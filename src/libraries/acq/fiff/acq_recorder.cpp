//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_recorder.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqRecorder: crash-safe FIFF recording of a stream with aligned, double-buffered direct I/O.
 *
 * Every block written to a file is a sequence of FIFF tags, a FIFF_NOP padding them to a page boundary, and a
 * page holding only a terminating FIFF_NOP whose next field is FIFFV_NEXT_NONE, so readers stop there. After
 * the following block is durable, the writer rewrites that terminator page with FIFFV_NEXT_SEQ, linking the
 * new block; a page with samples is never rewritten. Whatever is cut off, the file ends at a NOP that says
 * "no more tags".
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_recorder.h"
#include "acq_fiff.h"
#include "detail/acq_direct_file.h"

#include "../acq_reader.h"
#include "../acq_timebase.h"

#include <base/string_utils.h>
#include <bids/bids_channel.h>
#include <bids/bids_event.h>
#include <bids/bids_path.h>
#include <bids/bids_raw_data.h>

#include <fiff/fiff_annotations.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_stream.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBuffer>
#include <QByteArray>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL
//=============================================================================================================

namespace
{

constexpr std::size_t kPage = AcqDirectFile::kBlockBytes;
constexpr std::size_t kTagHeader = 16;
constexpr std::uint64_t kMaxSplitBytes = 2147483648ULL;
constexpr std::uint64_t kTrailerReserve = 2 * kPage;
constexpr std::uint64_t kAnnotationReserve = 1048576; // room left in the first part for its annotations, as MNE-Python
constexpr std::size_t kDataBlocks = 2;                // double buffering: one filled while the other is written
constexpr const char* kStimName = "STI 101";
constexpr const char* kSkipDescription = "BAD_ACQ_SKIP";

std::size_t alignUp(std::size_t bytes)
{
    return (bytes + kPage - 1) / kPage * kPage;
}

void putBe32(std::byte* out, std::uint32_t value)
{
    out[0] = static_cast<std::byte>(value >> 24U);
    out[1] = static_cast<std::byte>(value >> 16U);
    out[2] = static_cast<std::byte>(value >> 8U);
    out[3] = static_cast<std::byte>(value);
}

void putTagHeader(std::byte* out, int kind, int type, std::size_t size, int next)
{
    putBe32(out, static_cast<std::uint32_t>(kind));
    putBe32(out + 4, static_cast<std::uint32_t>(type));
    putBe32(out + 8, static_cast<std::uint32_t>(size));
    putBe32(out + 12, static_cast<std::uint32_t>(next));
}

template<typename T>
void storeBe(std::byte* out, T value)
{
    using U = std::conditional_t<sizeof(T) == 2, std::uint16_t, std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>>;
    const U bits = std::bit_cast<U>(value);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        out[i] = static_cast<std::byte>(bits >> (8U * (sizeof(T) - 1 - i)));
    }
}

/** How samples are stored: the FIFF type of the data buffers. */
enum class Pack : std::uint8_t
{
    Short,
    Int,
    Float,
    Double
};

int fiffType(Pack pack)
{
    switch (pack) {
        case Pack::Short:
            return FIFFT_DAU_PACK16;
        case Pack::Int:
            return FIFFT_INT;
        case Pack::Float:
            return FIFFT_FLOAT;
        case Pack::Double:
            return FIFFT_DOUBLE;
    }
    return FIFFT_DOUBLE;
}

std::size_t packBytes(Pack pack)
{
    return pack == Pack::Short ? 2U : (pack == Pack::Double ? 8U : 4U);
}

/** Native storage of each sample format; complex formats never get here. */
Pack nativePack(AcqSampleFormat format)
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return Pack::Short;
        case AcqSampleFormat::Int24Packed:
        case AcqSampleFormat::Int32:
            return Pack::Int;
        case AcqSampleFormat::Float32:
            return Pack::Float;
        default:
            return Pack::Double;
    }
}

template<typename Src>
auto readSample(const std::byte* at)
{
    Src value;
    std::memcpy(&value, at, sizeof(Src));
    if constexpr (std::is_same_v<Src, AcqInt24>) {
        return value.value();
    } else {
        return value;
    }
}

/** Big-endian copy of @p count frames of @p part, starting at frame @p from, into frames of @p outChannels. */
template<typename Src, typename Dst>
void packFrames(const AcqBlockPart& part, std::size_t from, std::size_t count, std::size_t channels,
                std::size_t outChannels, std::byte* out)
{
    for (std::size_t s = 0; s < count; ++s) {
        std::byte* frame = out + s * outChannels * sizeof(Dst);
        for (std::size_t c = 0; c < channels; ++c) {
            storeBe(frame + c * sizeof(Dst), static_cast<Dst>(readSample<Src>(part.at(c, from + s))));
        }
    }
}

using PackFunction = void (*)(const AcqBlockPart&, std::size_t, std::size_t, std::size_t, std::size_t, std::byte*);

PackFunction packFunction(AcqSampleFormat format)
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return &packFrames<std::int16_t, std::int16_t>;
        case AcqSampleFormat::Int24Packed:
            return &packFrames<AcqInt24, std::int32_t>;
        case AcqSampleFormat::Int32:
            return &packFrames<std::int32_t, std::int32_t>;
        case AcqSampleFormat::Float32:
            return &packFrames<float, float>;
        case AcqSampleFormat::Float64:
            return &packFrames<double, double>;
        default:
            return nullptr;
    }
}

/** memcpy for memory another thread may be writing: byte-wise relaxed atomic loads, word-wise where aligned. */
void racyCopy(std::byte* dst, const std::byte* src, std::size_t bytes)
{
    auto* from = const_cast<std::byte*>(src); // atomic_ref<const T> is C++26; the loads never write
    std::size_t i = 0;
    for (; i < bytes && reinterpret_cast<std::uintptr_t>(from + i) % alignof(std::uint64_t) != 0; ++i) {
        dst[i] = std::atomic_ref<std::byte>(from[i]).load(std::memory_order_relaxed);
    }
    for (; i + sizeof(std::uint64_t) <= bytes; i += sizeof(std::uint64_t)) {
        const std::uint64_t word =
            std::atomic_ref<std::uint64_t>(*reinterpret_cast<std::uint64_t*>(from + i)).load(std::memory_order_relaxed);
        std::memcpy(dst + i, &word, sizeof(word));
    }
    for (; i < bytes; ++i) {
        dst[i] = std::atomic_ref<std::byte>(from[i]).load(std::memory_order_relaxed);
    }
}

/** Numeric value of one real sample; for integers with @p digital bits on top, the analog part as decode() gives it. */
double rawValue(AcqSampleFormat format, const std::byte* at, int digital)
{
    std::int64_t word = 0;
    int bits = 0;
    switch (format) {
        case AcqSampleFormat::Int16:
            word = readSample<std::int16_t>(at);
            bits = 16;
            break;
        case AcqSampleFormat::Int24Packed:
            word = readSample<AcqInt24>(at);
            bits = 24;
            break;
        case AcqSampleFormat::Int32:
            word = readSample<std::int32_t>(at);
            bits = 32;
            break;
        case AcqSampleFormat::Float32:
            return static_cast<double>(readSample<float>(at));
        default:
            return readSample<double>(at);
    }
    if (digital == 0) {
        return static_cast<double>(word);
    }
    // signExtend(low bits - digital bits) << digital
    const int analog = bits - digital;
    const auto low = static_cast<std::uint64_t>(word) & ((std::uint64_t(1) << analog) - 1);
    const std::uint64_t sign = std::uint64_t(1) << (analog - 1);
    return static_cast<double>((static_cast<std::int64_t>(low ^ sign) - static_cast<std::int64_t>(sign)) *
                               (std::int64_t(1) << digital));
}

/** Writes @p value into the stim column of one frame in the stored type. */
void putStim(std::byte* at, Pack pack, std::int32_t value)
{
    switch (pack) {
        case Pack::Short:
            storeBe(at, static_cast<std::int16_t>(std::clamp<std::int32_t>(value, std::numeric_limits<std::int16_t>::min(), std::numeric_limits<std::int16_t>::max())));
            break;
        case Pack::Int:
            storeBe(at, value);
            break;
        case Pack::Float:
            storeBe(at, static_cast<float>(value));
            break;
        case Pack::Double:
            storeBe(at, static_cast<double>(value));
            break;
    }
}

/** FIFF tags written by FiffStream into memory. */
template<typename Write>
QByteArray fiffTags(Write&& write)
{
    auto bytes = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *bytes;
    FiffStream stream(std::move(bytes));
    write(stream);
    return QByteArray(reinterpret_cast<const char*>(written.data().data()), static_cast<qsizetype>(written.size()));
}

std::string lockName(AcqLockState lock)
{
    switch (lock) {
        case AcqLockState::Free:
            return "Free";
        case AcqLockState::Declared:
            return "Declared";
        case AcqLockState::FrequencyLocked:
            return "FrequencyLocked";
        case AcqLockState::PhaseLocked:
            return "PhaseLocked";
    }
    return "Free";
}

std::string clockSourceName(AcqClockSource source)
{
    switch (source) {
        case AcqClockSource::Nominal:
            return "Nominal";
        case AcqClockSource::Host:
            return "Host";
        case AcqClockSource::Device:
            return "Device";
        case AcqClockSource::Lsl:
            return "Lsl";
        case AcqClockSource::External:
            return "External";
    }
    return "Nominal";
}

bool isLabel(const std::string& value)
{
    return !value.empty() && mne::BIDSPath::is_valid_entity_value(value);
}

/** One unit of work for the writer thread: whole pages ending in a NOP that terminates the file. */
struct Block
{
    std::unique_ptr<AcqAlignedBuffer> buffer;
    std::size_t bytes = 0;     // written length, whole pages
    std::size_t padTag = 0;    // offset of the terminator page
    std::uint64_t samples = 0; // data samples, 0 for metadata blocks
    std::size_t file = 0;      // part index
    bool opensFile = false;    // open part `file` before writing
    bool closesFile = false;   // close the part after writing
    bool appendsFirst = false; // append to the closed first part (its annotations)
    bool continues = false;    // closes a part that a further part follows
    bool pooled = false;       // a data block, returned to the pool
};

/** Bytes of a block holding @p used bytes of tags: padding to a page boundary plus the terminator page. */
std::size_t sealedBytes(std::size_t used)
{
    return alignUp(used + kTagHeader) + kPage;
}

/** Pads @p used bytes of tags in @p block to a page boundary and appends the terminator page. */
void seal(Block& block, std::size_t used)
{
    const std::size_t pad = alignUp(used + kTagHeader);
    std::byte* data = block.buffer->data();
    putTagHeader(data + used, FIFF_NOP, FIFFT_VOID, pad - used - kTagHeader, FIFFV_NEXT_SEQ);
    std::memset(data + used + kTagHeader, 0, pad - used - kTagHeader);
    putTagHeader(data + pad, FIFF_NOP, FIFFT_VOID, kPage - kTagHeader, FIFFV_NEXT_NONE);
    std::memset(data + pad + kTagHeader, 0, kPage - kTagHeader);
    block.bytes = pad + kPage;
    block.padTag = pad;
}

std::unique_ptr<Block> metadataBlock(const QByteArray& tags)
{
    auto block = std::make_unique<Block>();
    block->buffer = std::make_unique<AcqAlignedBuffer>(sealedBytes(static_cast<std::size_t>(tags.size())));
    std::memcpy(block->buffer->data(), tags.constData(), static_cast<std::size_t>(tags.size()));
    seal(*block, static_cast<std::size_t>(tags.size()));
    return block;
}

/** The terminator page of the last written block, which links the following block once that is durable. */
struct Link
{
    bool valid = false;
    std::uint64_t pageOffset = 0;
    std::unique_ptr<AcqAlignedBuffer> page = std::make_unique<AcqAlignedBuffer>(kPage);
};

struct RecordedEvent
{
    std::uint64_t sample = 0;
    std::uint32_t duration = 0;
    std::int32_t code = 0;
};

struct Skip
{
    std::uint64_t first = 0;
    std::uint64_t count = 0;
};

} // namespace

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

struct AcqRecorder::Impl
{
    // Fixed at start.
    AcqStream stream;
    AcqReader reader;
    AcqRecorderOptions options;
    std::optional<AcqBidsPath> bids;
    FiffInfo fiffInfo;
    std::shared_ptr<const AcqStreamInfo> info;
    std::string firstPath; // part 0 before a BIDS rename
    Pack pack = Pack::Float;
    PackFunction packer = nullptr;
    bool physical = false;
    bool addStim = false;
    std::size_t channels = 0;
    std::size_t outChannels = 0;
    std::size_t frameBytes = 0;
    std::size_t bufferSamples = 0;
    std::uint64_t firstSample = 0;
    std::uint64_t origin = 0; // stream sample at meas_date: 0, or the first sample if FIFF's int32 cannot hold it
    fiff_int_t measDate[2] = {-1, 0};
    double sfreq = 0.0;

    // Reader thread.
    std::thread readerThread;
    std::vector<RecordedEvent> events;
    std::vector<Skip> skips;
    std::size_t part = 0;
    std::uint64_t partBytes = 0;
    std::int32_t stimCode = 0;
    std::uint64_t stimEnd = 0;
    std::vector<std::unique_ptr<Block>> pool;
    std::size_t allocated = 0;
    std::vector<std::byte> snapshot; // race-free copy of the view being encoded


    // Writer thread.
    std::thread writerThread;
    AcqDirectFile file;
    std::uint64_t offset = 0;
    Link link;
    std::uint64_t firstEnd = 0; // end of the closed first part
    Link firstLink;

    // Shared.
    std::mutex mutex;
    std::condition_variable queued;   // a block for the writer, or the end
    std::condition_variable returned; // a pooled block is free again
    std::deque<std::unique_ptr<Block>> queue;
    bool writerDone = false;
    std::vector<std::string> paths;
    AcqError error;
    std::atomic<bool> stopRequested{false};
    std::atomic<std::uint64_t> stopAt{0};
    std::atomic<bool> failed{false};
    std::atomic<bool> recording{false};
    bool stopped = false;

    std::atomic<std::uint64_t> statSamples{0};
    std::atomic<std::uint64_t> statLost{0};
    std::atomic<std::uint64_t> statEvents{0};
    std::atomic<std::uint64_t> statBuffers{0};
    std::atomic<std::uint64_t> statBytes{0};
    std::atomic<std::size_t> statFiles{0};
    std::atomic<bool> statDirect{false};

    //=========================================================================================================

    void fail(const AcqError& e)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (error.ok()) {
            error = e;
        }
        failed.store(true, std::memory_order_release);
        returned.notify_all();
    }

    /** Path of part @p index as it is created; the first part keeps the given name until a split. */
    std::string partPath(std::size_t index) const
    {
        const std::filesystem::path first(firstPath);
        if (index == 0) {
            return firstPath;
        }
        if (bids) {
            return splitPath(index);
        }
        return sibling(first.stem().string() + "-" + std::to_string(index) + first.extension().string());
    }

    /** @p name in the directory of the first part, spelled as the caller gave it (no separator rewrite on Windows). */
    std::string sibling(const std::string& name) const
    {
        const std::string first = std::filesystem::path(firstPath).filename().string();
        return firstPath.substr(0, firstPath.size() - first.size()) + name;
    }

    /** BIDS name of part @p index of a split recording: sub-01_task-x_meg.fif -> sub-01_task-x_split-02_meg.fif. */
    std::string splitPath(std::size_t index) const
    {
        const std::filesystem::path first(firstPath);
        const std::string name = first.filename().string();
        const std::size_t suffix = name.rfind("_meg.fif");
        char split[16];
        std::snprintf(split, sizeof(split), "_split-%02zu", index + 1);
        return sibling(name.substr(0, suffix) + split + name.substr(suffix));
    }

    /** Name of part @p index once the recording is complete, as the links between parts give it. */
    std::string finalName(std::size_t index) const
    {
        return std::filesystem::path(bids ? splitPath(index) : partPath(index)).filename().string();
    }

    //=========================================================================================================
    // Metadata blocks (reader thread and start()).

    std::unique_ptr<Block> header(std::size_t index, std::uint64_t firstTag) const
    {
        auto memory = std::make_unique<mne::MemoryByteStream>();
        const mne::MemoryByteStream& written = *memory;
        Eigen::RowVectorXd cals;
        auto tags = FiffStream::start_writing_raw(std::move(memory), fiffInfo, cals, defaultMatrixXi, false,
                                                  fiffType(pack));
        if (firstTag != 0) {
            const auto value = static_cast<fiff_int_t>(firstTag);
            tags->write_int(FIFF_FIRST_SAMPLE, &value);
        }
        if (index > 0) {
            tags->start_block(FIFFB_REF);
            const fiff_int_t role = FIFFV_ROLE_PREV_FILE;
            tags->write_int(FIFF_REF_ROLE, &role);
            tags->write_string(FIFF_REF_FILE_NAME, finalName(index - 1));
            const auto number = static_cast<fiff_int_t>(index - 1);
            tags->write_int(FIFF_REF_FILE_NUM, &number);
            tags->end_block(FIFFB_REF);
        }
        auto block = metadataBlock(QByteArray(reinterpret_cast<const char*>(written.data().data()), static_cast<qsizetype>(written.size())));
        block->file = index;
        block->opensFile = true;
        return block;
    }

    FiffAnnotations annotations() const
    {
        FiffAnnotations list;
        for (const RecordedEvent& e : events) {
            const auto code = static_cast<std::size_t>(e.code);
            const bool labelled = e.code >= 0 && code < info->eventLabels.size() && !info->eventLabels[code].empty();
            list.append(static_cast<double>(e.sample - origin) / sfreq, static_cast<double>(e.duration) / sfreq,
                        labelled ? info->eventLabels[code] : std::to_string(e.code));
        }
        for (const Skip& s : skips) {
            list.append(static_cast<double>(s.first - origin) / sfreq, static_cast<double>(s.count) / sfreq,
                        kSkipDescription);
        }
        return list;
    }

    /** End of a part: link to the next part, end of the raw data, and for the last part the annotations. */
    std::unique_ptr<Block> trailer(bool last)
    {
        const QByteArray bytes = fiffTags([&](FiffStream& tags) {
            if (!last) {
                tags.start_block(FIFFB_REF);
                const fiff_int_t role = FIFFV_ROLE_NEXT_FILE;
                tags.write_int(FIFF_REF_ROLE, &role);
                tags.write_string(FIFF_REF_FILE_NAME, finalName(part + 1));
                const auto number = static_cast<fiff_int_t>(part + 1);
                tags.write_int(FIFF_REF_FILE_NUM, &number);
                tags.end_block(FIFFB_REF);
            }
            tags.end_block(FIFFB_RAW_DATA);
            // The first part ends its measurement block when the annotations are known, at the end.
            if (part == 0 && last) {
                tags.write_annotations(annotations(), measDate);
            }
            if (part != 0 || last) {
                tags.end_block(FIFFB_MEAS);
            }
        });
        auto block = metadataBlock(bytes);
        block->file = part;
        block->closesFile = true;
        block->continues = !last;
        return block;
    }

    std::unique_ptr<Block> firstPartAnnotations()
    {
        const QByteArray bytes = fiffTags([&](FiffStream& tags) {
            tags.write_annotations(annotations(), measDate);
            tags.end_block(FIFFB_MEAS);
        });
        auto block = metadataBlock(bytes);
        block->appendsFirst = true;
        return block;
    }

    //=========================================================================================================
    // Reader thread.

    std::unique_ptr<Block> takeDataBlock()
    {
        std::unique_lock<std::mutex> lock(mutex);
        returned.wait(lock, [&] { return !pool.empty() || allocated < kDataBlocks || failed.load(); });
        if (failed.load()) {
            return nullptr;
        }
        std::unique_ptr<Block> block;
        if (!pool.empty()) {
            block = std::move(pool.back());
            pool.pop_back();
        } else {
            ++allocated;
            lock.unlock();
            block = std::make_unique<Block>();
            block->buffer = std::make_unique<AcqAlignedBuffer>(sealedBytes(kTagHeader + bufferSamples * frameBytes));
            block->pooled = true;
        }
        block->samples = 0;
        block->opensFile = block->closesFile = block->appendsFirst = false;
        return block;
    }

    void submit(std::unique_ptr<Block> block)
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(block));
        queued.notify_one();
    }

    /** Room kept free at the end of the current part. */
    std::uint64_t reserve() const
    {
        return kTrailerReserve + (part == 0 ? kAnnotationReserve : 0);
    }

    /** Seals a filled data block and hands it to the writer (leaving @p block empty), splitting the file first if needed. */
    void submitData(std::unique_ptr<Block>& block, std::uint64_t blockFirst)
    {
        const std::size_t used = kTagHeader + static_cast<std::size_t>(block->samples) * frameBytes;
        putTagHeader(block->buffer->data(), FIFF_DATA_BUFFER, fiffType(pack), used - kTagHeader, FIFFV_NEXT_SEQ);
        seal(*block, used);
        if (partBytes + block->bytes + reserve() > options.splitBytes) {
            submit(trailer(false));
            ++part;
            auto next = header(part, blockFirst - origin);
            partBytes = next->bytes;
            submit(std::move(next));
        }
        block->file = part;
        partBytes += block->bytes;
        submit(std::move(block));
    }

    void readerLoop()
    {
        std::uint64_t position = firstSample; // next stream sample to store
        std::unique_ptr<Block> block;
        std::uint64_t blockFirst = position;

        const auto appendFrames = [&](std::size_t count, const auto& fill) -> bool {
            while (count > 0) {
                if (!block) {
                    block = takeDataBlock();
                    if (!block) {
                        return false;
                    }
                    blockFirst = position;
                }
                const std::size_t n = std::min<std::size_t>(count, bufferSamples - static_cast<std::size_t>(block->samples));
                fill(block->buffer->data() + kTagHeader + static_cast<std::size_t>(block->samples) * frameBytes, n);
                block->samples += n;
                position += n;
                count -= n;
                if (block->samples == bufferSamples) {
                    submitData(block, blockFirst);
                }
            }
            return true;
        };
        const auto zeros = [&](std::size_t count) {
            return appendFrames(count, [&](std::byte* out, std::size_t n) { std::memset(out, 0, n * frameBytes); });
        };
        const auto skip = [&](std::uint64_t first, std::uint64_t count) {
            if (!skips.empty() && skips.back().first + skips.back().count == first) {
                skips.back().count += count;
            } else {
                skips.push_back({first, count});
            }
            statLost.fetch_add(count, std::memory_order_relaxed);
        };

        while (!failed.load(std::memory_order_acquire)) {
            if (stopRequested.load(std::memory_order_acquire) && position >= stopAt.load(std::memory_order_acquire)) {
                break;
            }
            // A view never spans two data blocks, so an overrun can be zeroed in place.
            const std::size_t room = block ? bufferSamples - static_cast<std::size_t>(block->samples) : bufferSamples;
            auto lease = reader.lease(1, std::chrono::milliseconds(20), room);
            if (!lease) {
                const AcqErrorCode code = lease.error().code();
                if (code == AcqErrorCode::Timeout || code == AcqErrorCode::DeviceStopped) {
                    continue;
                }
                if (code != AcqErrorCode::EndOfStream) {
                    fail(lease.error());
                }
                break;
            }
            const AcqBlockView& view = **lease;
            if (view.lost() > 0) {
                skip(position, view.lost());
                if (!zeros(static_cast<std::size_t>(view.lost()))) {
                    break;
                }
            }
            const std::size_t frames = view.sampleCount();
            if (block && bufferSamples - static_cast<std::size_t>(block->samples) < frames) {
                // The zeros of a loss took the room the view was acquired for.
                submitData(block, blockFirst);
            }
            if (!block) {
                block = takeDataBlock();
                if (!block) {
                    break;
                }
                blockFirst = position;
            }
            const std::uint64_t viewFirst = view.firstSample();
            std::byte* out = block->buffer->data() + kTagHeader + static_cast<std::size_t>(block->samples) * frameBytes;
            encode(view, out);
            std::size_t next = 0;
            for (std::size_t i = 0; i < view.events().size(); ++i) {
                const AcqEvent& e = view.events()[i];
                events.push_back({e.sample, e.duration, e.code});
                statEvents.fetch_add(1, std::memory_order_relaxed);
                if (addStim) {
                    // The stim column of every frame up to the event, then the new code.
                    const std::size_t onset = static_cast<std::size_t>(std::clamp(e.sample, viewFirst, viewFirst + frames) - viewFirst);
                    for (; next < onset; ++next) {
                        stimFrame(out, next, viewFirst + next);
                    }
                    stimCode = e.code;
                    stimEnd = e.sample + std::max<std::uint32_t>(e.duration, 1U);
                }
            }
            if (addStim) {
                for (; next < frames; ++next) {
                    stimFrame(out, next, viewFirst + next);
                }
            }
            if (lease->release() == AcqErrorCode::Overrun) {
                // The producer overwrote the view while it was copied: store it as lost.
                std::memset(out, 0, frames * frameBytes);
                skip(viewFirst, frames);
            }
            block->samples += frames;
            position += frames;
            if (block->samples == bufferSamples) {
                submitData(block, blockFirst);
            }
        }
        if (block && block->samples > 0 && !failed.load()) {
            submitData(block, blockFirst);
        } else if (block) {
            std::lock_guard<std::mutex> lock(mutex);
            pool.push_back(std::move(block));
        }
        if (!failed.load()) {
            submit(trailer(true));
            if (part > 0) {
                submit(firstPartAnnotations());
            }
        }
        std::lock_guard<std::mutex> lock(mutex);
        writerDone = true;
        queued.notify_one();
    }

    void stimFrame(std::byte* out, std::size_t frame, std::uint64_t sample)
    {
        putStim(out + frame * frameBytes + channels * packBytes(pack), pack, sample < stimEnd ? stimCode : 0);
    }

    /**
     * Copies the samples of @p view into the block at @p out in the stored format. The ring is read through a
     * snapshot taken with relaxed atomic loads: the producer may overwrite the view meanwhile (release() then
     * reports Overrun and the samples are discarded), and that must not be a data race.
     */
    void encode(const AcqBlockView& view, std::byte* out)
    {
        std::size_t done = 0;
        for (std::size_t p = 0; p < view.partCount(); ++p) {
            const AcqBlockPart run = snapshotPart(view.part(p));
            if (physical) {
                packPhysical(run, view, out + done * frameBytes);
            } else {
                packer(run, 0, run.samples, channels, outChannels, out + done * frameBytes);
            }
            done += run.samples;
        }
    }

    /** Copies @p run into the snapshot buffer and returns a run with the same layout over the copy. */
    AcqBlockPart snapshotPart(const AcqBlockPart& run)
    {
        const std::size_t sampleBytes = bytesPerSample(info->format);
        const std::size_t bytes = run.samples * channels * sampleBytes;
        if (snapshot.size() < bytes) {
            snapshot.resize(bytes);
        }
        AcqBlockPart copy = run;
        copy.data = snapshot.data();
        if (run.sampleStride == static_cast<std::ptrdiff_t>(channels * sampleBytes)) {
            racyCopy(snapshot.data(), run.data, bytes); // interleaved: one block of frames
        } else {
            // planar: one contiguous row per channel
            const std::size_t rowBytes = run.samples * sampleBytes;
            for (std::size_t c = 0; c < channels; ++c) {
                racyCopy(snapshot.data() + c * rowBytes, run.at(c, 0), rowBytes);
            }
            copy.channelStride = static_cast<std::ptrdiff_t>(rowBytes);
        }
        return copy;
    }

    /** raw * scale + offset per channel as big-endian float64, the analog part of words with digital bits. */
    void packPhysical(const AcqBlockPart& run, const AcqBlockView& view, std::byte* out) const
    {
        const auto scales = view.scales();
        const auto offsets = view.offsets();
        const auto digital = view.digitalBits();
        for (std::size_t s = 0; s < run.samples; ++s) {
            for (std::size_t c = 0; c < channels; ++c) {
                double value = rawValue(info->format, run.at(c, s), digital.empty() ? 0 : digital[c]);
                value *= scales[c]; // as decode(): multiply, then add, never fused
                value += offsets[c];
                storeBe(out + (s * outChannels + c) * sizeof(double), value);
            }
        }
    }

    //=========================================================================================================
    // Writer thread.

    AcqError writeBlock(const Block& block, Link& previous)
    {
        if (auto e = file.writeAt(offset, block.buffer->data(), block.bytes); !e.ok()) {
            return e;
        }
        if (auto e = file.barrier(); !e.ok()) {
            return e;
        }
        if (previous.valid) {
            putBe32(previous.page->data() + 12, static_cast<std::uint32_t>(FIFFV_NEXT_SEQ));
            if (auto e = file.writeAt(previous.pageOffset, previous.page->data(), kPage); !e.ok()) {
                return e;
            }
        }
        previous.valid = true;
        previous.pageOffset = offset + block.padTag;
        std::memcpy(previous.page->data(), block.buffer->data() + block.padTag, kPage);
        offset += block.bytes;
        statBytes.fetch_add(block.bytes, std::memory_order_relaxed);
        statSamples.fetch_add(block.samples, std::memory_order_relaxed);
        if (block.samples > 0) {
            statBuffers.fetch_add(1, std::memory_order_relaxed);
        }
        return {};
    }

    AcqError process(Block& block)
    {
        if (block.appendsFirst) {
            // The first part was closed at a split; its annotations go after its last block.
            if (auto e = file.open(paths.front(), options.directIo, false); !e.ok()) {
                return e;
            }
            offset = firstEnd;
            if (auto e = writeBlock(block, firstLink); !e.ok()) {
                return e;
            }
            auto e = file.barrier();
            file.close();
            return e;
        }
        if (block.opensFile) {
            const std::string path = partPath(block.file);
            if (auto e = file.open(path, options.directIo); !e.ok()) {
                return e;
            }
            statDirect.store(file.isDirect(), std::memory_order_relaxed);
            offset = 0;
            link.valid = false;
            std::lock_guard<std::mutex> lock(mutex);
            paths.push_back(path);
            statFiles.store(paths.size(), std::memory_order_relaxed);
        }
        if (auto e = writeBlock(block, link); !e.ok()) {
            return e;
        }
        if (block.closesFile) {
            if (auto e = file.barrier(); !e.ok()) {
                return e;
            }
            file.close();
            if (block.file == 0) {
                firstEnd = offset;
                std::swap(firstLink, link);
                if (bids && block.continues) {
                    return renameFirst();
                }
            }
        }
        return {};
    }

    /** MNE-Python's BIDS naming: once a recording is split, the first part is split-01 as well. */
    AcqError renameFirst()
    {
        const std::string renamed = splitPath(0);
        std::error_code ec;
        std::filesystem::rename(paths.front(), renamed, ec);
        if (ec) {
            return AcqError(AcqErrorCode::DeviceIo, "cannot rename " + paths.front() + ": " + ec.message());
        }
        std::lock_guard<std::mutex> lock(mutex);
        paths.front() = renamed;
        return {};
    }

    void writerLoop()
    {
        for (;;) {
            std::unique_ptr<Block> block;
            {
                std::unique_lock<std::mutex> lock(mutex);
                queued.wait(lock, [&] { return !queue.empty() || writerDone; });
                if (queue.empty()) {
                    break;
                }
                block = std::move(queue.front());
                queue.pop_front();
            }
            if (!failed.load(std::memory_order_acquire)) {
                if (AcqError e = process(*block); !e.ok()) {
                    fail(e);
                    file.close();
                }
            }
            if (block->pooled) {
                std::lock_guard<std::mutex> lock(mutex);
                pool.push_back(std::move(block));
                returned.notify_one();
            }
        }
        recording.store(false, std::memory_order_release);
    }

    //=========================================================================================================
    // BIDS sidecars (stop()).

    AcqError writeSidecars()
    {
        mne::BidsRawData data;
        data.raw.info = fiffInfo;
        data.raw.first_samp = static_cast<fiff_int_t>(firstSample - origin);
        data.raw.last_samp = static_cast<fiff_int_t>(firstSample - origin + statSamples.load() - 1);
        data.recordingType = "continuous";
        data.manufacturerModelName = options.device.device;
        for (const RecordedEvent& e : events) {
            mne::BidsEvent event;
            event.sample = static_cast<int>(e.sample - firstSample);
            event.onset = static_cast<float>(static_cast<double>(event.sample) / sfreq);
            event.duration = static_cast<float>(static_cast<double>(e.duration) / sfreq);
            event.value = e.code;
            const auto code = static_cast<std::size_t>(e.code);
            event.trialType = e.code >= 0 && code < info->eventLabels.size() && !info->eventLabels[code].empty()
                ? info->eventLabels[code]
                : std::string("n/a");
            data.events.push_back(std::move(event));
        }
        mne::BIDSPath path(mne::path_from_utf8(bids->root), bids->subject, bids->session, bids->task, "meg", "meg", ".fif");
        path.set_run(bids->run);
        mne::BidsRawData::WriteOptions write;
        write.overwrite = true;
        write.copyData = false;
        write.datasetName = bids->datasetName;
        if (data.write(path, {}, write).root().empty()) {
            return AcqError(AcqErrorCode::DeviceIo, "cannot write the BIDS sidecars of " + firstPath);
        }
        return {};
    }

    //=========================================================================================================

    AcqError begin(const AcqStream& source, const std::string& path)
    {
        stream = source;
        info = source.info();
        if (!info) {
            return AcqError(AcqErrorCode::InvalidState, "the stream has no description");
        }
        if (source.memory().kind == AcqMemoryKind::Device) {
            return AcqErrorCode::DeviceMemory;
        }
        if (options.splitBytes > kMaxSplitBytes) {
            return AcqError(AcqErrorCode::InvalidParameter, "splitBytes above 2 GiB");
        }
        auto converted = toFiffInfo(*info);
        if (!converted) {
            return converted.error();
        }
        fiffInfo = *converted;
        sfreq = info->sfreq;
        if (!(sfreq > 0.0)) {
            return AcqError(AcqErrorCode::UnsupportedFormat, "FIFF raw data needs a sampling rate");
        }
        channels = info->channelCount();
        physical = std::any_of(info->channels.begin(), info->channels.end(),
                               [](const AcqChannelInfo& c) { return c.offset != 0.0 || c.digitalBits != 0; });
        pack = physical ? Pack::Double : nativePack(info->format);
        packer = packFunction(info->format);
        if (physical) {
            for (std::size_t c = 0; c < fiffInfo.chs.size(); ++c) {
                fiffInfo.chs[c].cal = static_cast<float>(std::pow(10.0, info->channels[c].unitExponent));
                fiffInfo.chs[c].range = 1.0f;
                fiffInfo.chs[c].unit_mul = FIFF_UNITM_NONE;
            }
        }
        addStim = std::none_of(info->channels.begin(), info->channels.end(),
                               [](const AcqChannelInfo& c) { return c.kind == AcqChannelKind::Stim; });
        if (addStim) {
            FiffChInfo stim;
            stim.ch_name = kStimName;
            stim.kind = FIFFV_STIM_CH;
            stim.unit = FIFF_UNIT_NONE;
            stim.unit_mul = FIFF_UNITM_NONE;
            stim.cal = 1.0f;
            stim.range = 1.0f;
            stim.scanNo = static_cast<fiff_int_t>(channels + 1);
            stim.logNo = stim.scanNo;
            stim.chpos.coil_type = FIFFV_COIL_NONE;
            stim.coord_frame = FIFFV_COORD_UNKNOWN;
            fiffInfo.chs.push_back(stim);
            fiffInfo.ch_names.push_back(stim.ch_name);
            fiffInfo.nchan = static_cast<fiff_int_t>(fiffInfo.chs.size());
        }
        outChannels = channels + (addStim ? 1U : 0U);
        frameBytes = outChannels * packBytes(pack);
        bufferSamples = options.bufferSamples > 0 ? options.bufferSamples
                                                  : std::max<std::size_t>(1, static_cast<std::size_t>(std::lround(sfreq)));
        if (kTagHeader + bufferSamples * frameBytes > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
            return AcqError(AcqErrorCode::InvalidParameter, "bufferSamples too large for a FIFF tag");
        }
        snapshot.resize(bufferSamples * channels * bytesPerSample(info->format)); // no allocation while recording

        auto attached = source.reader(AcqReader::ReadMode::Lossless);
        if (!attached) {
            return attached.error();
        }
        reader = std::move(*attached);
        firstSample = reader.stats().position;

        // Measurement date: UTC of stream sample 0; FIFF positions are int32, so a later start is rebased.
        const AcqClockModel clock = source.clock();
        const auto systemNow = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
        const auto steadyNow = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count();
        const std::int64_t utcOffset = clock.utcOffsetNs != 0 ? clock.utcOffsetNs : systemNow - steadyNow;
        std::int64_t utcSample0 = clock.sfreq > 0.0
            ? clock.physicalNs(0) + utcOffset
            : systemNow - static_cast<std::int64_t>(static_cast<double>(firstSample) * 1e9 / sfreq);
        if (firstSample > static_cast<std::uint64_t>(std::numeric_limits<fiff_int_t>::max())) {
            utcSample0 += static_cast<std::int64_t>(std::llround(static_cast<double>(firstSample) * 1e9 / sfreq));
            origin = firstSample;
        }
        const std::int64_t seconds = utcSample0 >= 0 ? utcSample0 / 1000000000 : (utcSample0 - 999999999) / 1000000000;
        measDate[0] = static_cast<fiff_int_t>(seconds);
        measDate[1] = static_cast<fiff_int_t>((utcSample0 - seconds * 1000000000) / 1000);
        fiffInfo.meas_date[0] = measDate[0];
        fiffInfo.meas_date[1] = measDate[1];

        fiffInfo.device_info.type = options.device.backend;
        fiffInfo.device_info.model = options.device.device;
        fiffInfo.device_info.serial = info->sourceId;
        const AcqTimebase timebase = source.timebase();
        std::ostringstream provenance;
        provenance << "mne_acq stream=" << info->name << " uri=" << options.device.uri
                   << " first_sample=" << firstSample << " clock_domain=" << info->clockDomain
                   << " domain=" << timebase.domain << " lock=" << lockName(timebase.lock)
                   << " ticks_per_second=" << timebase.ticksPerSecond << " ticks_per_sample=" << timebase.ticksPerSample
                   << " sample_alignment=" << timebase.sampleAlignment << " origin_tick=" << timebase.originTick
                   << " origin_uncertainty_ticks=" << timebase.originUncertaintyTicks
                   << " clock_source=" << clockSourceName(clock.source) << " rate_correction=" << clock.rateCorrection
                   << " clock_uncertainty_ns=" << clock.uncertaintyNs << " pipeline_delay_ns=" << clock.pipelineDelayNs;
        const std::string text = provenance.str();
        fiffInfo.description = fiffInfo.description.empty() ? text : fiffInfo.description + "; " + text;

        firstPath = path;
        std::error_code ec;
        const std::filesystem::path parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
        }

        // The measurement info is on disk before start() returns, so a bad path fails here.
        auto first = header(0, firstSample - origin);
        if (options.splitBytes < first->bytes + sealedBytes(kTagHeader + bufferSamples * frameBytes) + reserve()) {
            return AcqError(AcqErrorCode::InvalidParameter, "splitBytes smaller than the header and one buffer");
        }
        partBytes = first->bytes;
        if (AcqError e = process(*first); !e.ok()) {
            return e;
        }
        recording.store(true, std::memory_order_release);
        writerThread = std::thread([this] { writerLoop(); });
        auto started = start_thread(options.thread, [this] { readerLoop(); });
        if (!started) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                writerDone = true;
                queued.notify_one();
            }
            writerThread.join();
            stopped = true;
            return started.error();
        }
        readerThread = std::move(*started);
        return {};
    }

    AcqError finish()
    {
        if (stopped) {
            std::lock_guard<std::mutex> lock(mutex);
            return error;
        }
        stopped = true;
        stopAt.store(stream.stats().committed, std::memory_order_release);
        stopRequested.store(true, std::memory_order_release);
        readerThread.join();
        writerThread.join();
        file.close();
        recording.store(false, std::memory_order_release);
        if (bids && !failed.load()) {
            if (AcqError e = writeSidecars(); !e.ok()) {
                fail(e);
            }
        }
        std::lock_guard<std::mutex> lock(mutex);
        return error;
    }
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqExpected<AcqRecorder> AcqRecorder::start(const AcqStream& stream, const std::string& path,
                                            const AcqRecorderOptions& options)
{
    auto impl = std::make_unique<Impl>();
    impl->options = options;
    if (AcqError e = impl->begin(stream, path); !e.ok()) {
        return e;
    }
    return AcqRecorder(std::move(impl));
}

//=============================================================================================================

AcqExpected<AcqRecorder> AcqRecorder::start(const AcqStream& stream, const AcqBidsPath& bids,
                                            const AcqRecorderOptions& options)
{
    const auto optionalLabel = [](const std::string& value) {
        return value.empty() || isLabel(value);
    };
    if (bids.root.empty() || !isLabel(bids.subject) || !isLabel(bids.task) || !optionalLabel(bids.session) ||
        !optionalLabel(bids.run)) {
        return AcqError(AcqErrorCode::InvalidParameter, "BIDS needs a root, and alphanumeric subject and task labels");
    }
    mne::BIDSPath path(mne::path_from_utf8(bids.root), bids.subject, bids.session, bids.task, "meg", "meg", ".fif");
    path.set_run(bids.run);
    auto impl = std::make_unique<Impl>();
    impl->options = options;
    impl->bids = bids;
    if (AcqError e = impl->begin(stream, mne::utf8_from_path(path.file_path())); !e.ok()) {
        return e;
    }
    return AcqRecorder(std::move(impl));
}

//=============================================================================================================

AcqRecorder::AcqRecorder() noexcept = default;

//=============================================================================================================

AcqRecorder::AcqRecorder(std::unique_ptr<Impl> impl) noexcept
: m_pImpl(std::move(impl))
{
}

//=============================================================================================================

AcqRecorder::~AcqRecorder()
{
    stop();
}

//=============================================================================================================

AcqRecorder::AcqRecorder(AcqRecorder&& other) noexcept = default;

//=============================================================================================================

AcqRecorder& AcqRecorder::operator=(AcqRecorder&& other) noexcept
{
    if (this != &other) {
        stop();
        m_pImpl = std::move(other.m_pImpl);
    }
    return *this;
}

//=============================================================================================================

bool AcqRecorder::isRecording() const noexcept
{
    return m_pImpl && m_pImpl->recording.load(std::memory_order_acquire);
}

//=============================================================================================================

AcqError AcqRecorder::stop()
{
    return m_pImpl ? m_pImpl->finish() : AcqError();
}

//=============================================================================================================

AcqRecorderStats AcqRecorder::stats() const noexcept
{
    AcqRecorderStats s;
    if (m_pImpl) {
        s.firstSample = m_pImpl->firstSample;
        s.samples = m_pImpl->statSamples.load(std::memory_order_relaxed);
        s.lost = m_pImpl->statLost.load(std::memory_order_relaxed);
        s.events = m_pImpl->statEvents.load(std::memory_order_relaxed);
        s.buffers = m_pImpl->statBuffers.load(std::memory_order_relaxed);
        s.bytes = m_pImpl->statBytes.load(std::memory_order_relaxed);
        s.files = m_pImpl->statFiles.load(std::memory_order_relaxed);
        s.directIo = m_pImpl->statDirect.load(std::memory_order_relaxed);
    }
    return s;
}

//=============================================================================================================

std::vector<std::string> AcqRecorder::files() const
{
    if (!m_pImpl) {
        return {};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->paths;
}
