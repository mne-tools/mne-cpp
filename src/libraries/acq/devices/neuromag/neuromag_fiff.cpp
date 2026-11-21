//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     neuromag_fiff.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    FIFF tags of the Neuromag acquisition stream without the FIFF library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_fiff.h"

#include "../../detail/acq_fiff_codes.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr int kTypeByte = 1;
constexpr int kTypeShort = 2;
constexpr int kTypeDouble = 5;
constexpr int kTypeJulian = 6;
constexpr int kTypeUshort = 7;
constexpr int kTypeUint = 8;
constexpr int kTypeDauPack16 = 16;
constexpr int kTypeComplexFloat = 20;
constexpr int kTypeComplexDouble = 21;
constexpr int kTypeOldPack = 23;
constexpr int kTypeDirEntry = 32;
constexpr int kTypeDigPoint = 33;
constexpr int kTypeCoordTrans = 35;
constexpr int kTypeDigString = 36;
constexpr int kBaseMask = 0x00000FFF;
constexpr int kMatrix = 0x40000000;
constexpr int kSparseMask = 0x00FF0000;
constexpr std::size_t kNameOffset = 80; // the channel name follows 20 words
constexpr std::int32_t kFileVersion = (1 << 16) | 3;

std::int32_t byteswap(std::int32_t value) noexcept
{
    auto bits = static_cast<std::uint32_t>(value);
    bits = (bits >> 24) | ((bits >> 8) & 0x0000FF00U) | ((bits << 8) & 0x00FF0000U) | (bits << 24);
    return static_cast<std::int32_t>(bits);
}

void swap_words(std::span<std::byte> data, std::size_t width) noexcept
{
    for (std::size_t i = 0; i + width <= data.size(); i += width) {
        std::reverse(data.begin() + static_cast<std::ptrdiff_t>(i), data.begin() + static_cast<std::ptrdiff_t>(i + width));
    }
}

/** @return Bytes per element of a base type, 0 for byte strings and types without a known layout. */
std::size_t element_width(int base) noexcept
{
    switch (base) {
        case kTypeShort:
        case kTypeUshort:
        case kTypeDauPack16:
            return 2;
        case fiff::kTypeInt:
        case fiff::kTypeFloat:
        case kTypeJulian:
        case kTypeUint:
        case kTypeComplexFloat:
        case fiff::kTypeId:
        case kTypeDirEntry:
        case kTypeDigPoint:
        case kTypeCoordTrans:
        case kTypeDigString:
            return 4;
        case kTypeDouble:
        case kTypeComplexDouble:
            return 8;
        default:
            return 0;
    }
}

template<typename T>
T load(std::span<const std::byte> data, std::size_t offset) noexcept
{
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof(T));
    return value;
}

template<typename T>
void store(std::vector<std::byte>& out, T value)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

constexpr bool kNativeBig = std::endian::native == std::endian::big;

/** Appends a tag in big-endian byte order, or little-endian if @p big_endian is false. */
void append_tag(std::vector<std::byte>& file, int kind, int type, std::span<const std::byte> native, int next = 0, bool big_endian = true)
{
    std::vector<std::byte> data(native.begin(), native.end());
    const bool swap = big_endian != kNativeBig;
    if (swap) {
        swap_tag_bytes(type, data, kNativeBig);
    }
    for (std::int32_t word : {kind, type, static_cast<std::int32_t>(data.size()), next}) {
        store(file, swap ? byteswap(word) : word);
    }
    file.insert(file.end(), data.begin(), data.end());
}

std::vector<std::byte> file_id()
{
    std::vector<std::byte> id;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (const std::int32_t word : {kFileVersion, 0, 0, static_cast<std::int32_t>(seconds), 0}) {
        store(id, word);
    }
    return id;
}

std::vector<std::byte> int_data(std::int32_t value)
{
    std::vector<std::byte> data;
    store(data, value);
    return data;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<std::int32_t> NeuromagTag::as_int() const noexcept
{
    if (data.size() < sizeof(std::int32_t)) {
        return std::nullopt;
    }
    return load<std::int32_t>(data, 0);
}

//=============================================================================================================

std::optional<float> NeuromagTag::as_float() const noexcept
{
    if (data.size() < sizeof(float)) {
        return std::nullopt;
    }
    return load<float>(data, 0);
}

//=============================================================================================================

std::optional<NeuromagChannel> NeuromagChannel::parse(std::span<const std::byte> data)
{
    if (data.size() < kSize) {
        return std::nullopt;
    }
    NeuromagChannel ch;
    ch.scan_no = load<std::int32_t>(data, 0);
    ch.log_no = load<std::int32_t>(data, 4);
    ch.kind = load<std::int32_t>(data, 8);
    ch.range = load<float>(data, 12);
    ch.cal = load<float>(data, 16);
    ch.coil_type = load<std::int32_t>(data, 20);
    for (std::size_t i = 0; i < ch.loc.size(); ++i) {
        ch.loc[i] = load<float>(data, 24 + 4 * i);
    }
    ch.unit = load<std::int32_t>(data, 72);
    ch.unit_mul = load<std::int32_t>(data, 76);
    const auto* name = reinterpret_cast<const char*>(data.data() + kNameOffset);
    ch.name.assign(name, strnlen(name, kSize - kNameOffset));
    return ch;
}

//=============================================================================================================

std::vector<std::byte> NeuromagChannel::serialize() const
{
    std::vector<std::byte> data;
    data.reserve(kSize);
    store(data, scan_no);
    store(data, log_no);
    store(data, kind);
    store(data, range);
    store(data, cal);
    store(data, coil_type);
    for (const float value : loc) {
        store(data, value);
    }
    store(data, unit);
    store(data, unit_mul);
    std::array<char, kSize - kNameOffset> chars{};
    std::copy_n(name.begin(), std::min(name.size(), chars.size() - 1), chars.begin());
    const auto* bytes = reinterpret_cast<const std::byte*>(chars.data());
    data.insert(data.end(), bytes, bytes + chars.size());
    return data;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

void mne::detail::swap_tag_bytes(int type, std::span<std::byte> data, bool from_big) noexcept
{
    const int base = type & kBaseMask;
    if ((type & kMatrix) != 0) {
        const std::size_t width = element_width(base);
        if (width == 4 || (type & kSparseMask) != 0) {
            swap_words(data, 4); // elements, indices and dimensions are words
            return;
        }
        if (width != 8 || data.size() < 4) {
            return;
        }
        // Dense matrices of doubles end with their dimensions and then their number
        std::int32_t ndim = load<std::int32_t>(data, data.size() - 4);
        const bool native_big = std::endian::native == std::endian::big;
        if (from_big != native_big) {
            ndim = byteswap(ndim);
        }
        const std::size_t trailer = 4 * (static_cast<std::size_t>(std::max(ndim, 0)) + 1);
        if (trailer > data.size()) {
            return;
        }
        swap_words(data.first(data.size() - trailer), 8);
        swap_words(data.last(trailer), 4);
        return;
    }
    switch (base) {
        case fiff::kTypeChInfo:
            swap_words(data.first(std::min(data.size(), kNameOffset)), 4);
            return;
        case kTypeOldPack:
            swap_words(data.first(std::min<std::size_t>(data.size(), 8)), 4);
            if (data.size() > 8) {
                swap_words(data.subspan(8), 2);
            }
            return;
        case kTypeByte:
        case fiff::kTypeString:
        case fiff::kTypeVoid:
            return;
        default:
            if (const std::size_t width = element_width(base); width > 1) {
                swap_words(data, width);
            }
            return;
    }
}

//=============================================================================================================

void mne::detail::big_to_native(int type, std::span<std::byte> data) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        swap_tag_bytes(type, data, true);
    }
}

//=============================================================================================================

std::vector<std::byte> mne::detail::meas_info_file(std::span<const NeuromagTag> meas_info)
{
    std::vector<std::byte> file;
    append_tag(file, fiff::kFileId, fiff::kTypeId, file_id());
    append_tag(file, fiff::kDirPointer, fiff::kTypeInt, int_data(fiff::kNextNone));
    append_tag(file, fiff::kBlockStart, fiff::kTypeInt, int_data(fiff::kBlockMeas));
    for (const NeuromagTag& tag : meas_info) {
        append_tag(file, tag.kind, tag.type, tag.data);
    }
    append_tag(file, fiff::kBlockEnd, fiff::kTypeInt, int_data(fiff::kBlockMeas));
    append_tag(file, fiff::kNop, fiff::kTypeVoid, {}, fiff::kNextNone);
    return file;
}

//=============================================================================================================

std::vector<NeuromagTag> mne::detail::parse_tags(std::span<const std::byte> file, bool big_endian)
{
    constexpr std::size_t kHeader = 4 * sizeof(std::int32_t);
    const bool swap = big_endian != kNativeBig;
    std::vector<NeuromagTag> tags;
    std::size_t at = 0;
    while (at + kHeader <= file.size()) {
        std::array<std::int32_t, 3> words{};
        for (std::size_t w = 0; w < words.size(); ++w) {
            const auto word = load<std::int32_t>(file, at + 4 * w);
            words[w] = swap ? byteswap(word) : word;
        }
        const auto [kind, type, size] = words;
        if (size < 0 || static_cast<std::size_t>(size) > file.size() - at - kHeader) {
            break;
        }
        const auto data = file.subspan(at + kHeader, static_cast<std::size_t>(size));
        NeuromagTag& tag = tags.emplace_back(NeuromagTag{kind, type, {data.begin(), data.end()}});
        if (swap) {
            swap_tag_bytes(type, tag.data, big_endian);
        }
        at += kHeader + static_cast<std::size_t>(size);
    }
    return tags;
}

//=============================================================================================================

std::vector<std::byte> mne::detail::tag_file(std::span<const NeuromagTag> tags, bool big_endian)
{
    std::vector<std::byte> file;
    append_tag(file, fiff::kFileId, fiff::kTypeId, file_id(), 0, big_endian);
    append_tag(file, fiff::kDirPointer, fiff::kTypeInt, int_data(fiff::kNextNone), 0, big_endian);
    for (const NeuromagTag& tag : tags) {
        append_tag(file, tag.kind, tag.type, tag.data, 0, big_endian);
    }
    return file;
}

//=============================================================================================================

std::vector<NeuromagTag> mne::detail::block_of(std::span<const NeuromagTag> tags, int block)
{
    const auto is = [block](const NeuromagTag& tag, int edge) {
        return tag.kind == edge && tag.as_int() == block;
    };
    const auto begin = std::find_if(tags.begin(), tags.end(), [&](const NeuromagTag& t) { return is(t, fiff::kBlockStart); });
    const auto end = std::find_if(begin, tags.end(), [&](const NeuromagTag& t) { return is(t, fiff::kBlockEnd); });
    if (end == tags.end()) {
        return {};
    }
    return {begin, end + 1};
}

//=============================================================================================================

mne::AcqExpected<mne::AcqStreamInfo> mne::detail::stream_info(std::span<const NeuromagTag> meas_info, const std::string& source,
                                                              AcqSampleFormat format)
{
    AcqStreamInfo info;
    info.name = "raw";
    info.sourceId = source;
    info.format = format;
    info.layout = AcqLayout::Interleaved;
    std::optional<std::int32_t> nchan;
    int depth = 0; // only tags of the measurement info itself, not of its sub-blocks
    for (const NeuromagTag& tag : meas_info) {
        if (tag.kind == fiff::kBlockStart) {
            ++depth;
        } else if (tag.kind == fiff::kBlockEnd) {
            --depth;
        } else if (depth == 1 && tag.kind == fiff::kNchan) {
            nchan = tag.as_int();
        } else if (depth == 1 && tag.kind == fiff::kSfreq) {
            info.sfreq = tag.as_float().value_or(0.0F);
        } else if (depth == 1 && tag.kind == fiff::kChInfo) {
            const auto ch = NeuromagChannel::parse(tag.data);
            if (!ch) {
                return AcqError(AcqErrorCode::UnsupportedFormat, "the measurement info holds a truncated channel record");
            }
            AcqChannelInfo channel;
            channel.name = ch->name;
            channel.kind = acq_kind_of(ch->kind);
            channel.unit = acq_unit_of(ch->unit);
            channel.unitExponent = ch->unit_mul;
            channel.scale = format == AcqSampleFormat::Int32 ? static_cast<double>(ch->cal) * static_cast<double>(ch->range) : 1.0;
            AcqFiffChannel fiff;
            fiff.kind = ch->kind;
            fiff.coilType = ch->coil_type;
            fiff.logicalNo = ch->log_no;
            fiff.scannerNo = ch->scan_no;
            fiff.unit = ch->unit;
            fiff.unitMul = ch->unit_mul;
            fiff.coordFrame = fiff_coord_frame_of(ch->kind);
            fiff.cal = ch->cal;
            fiff.range = ch->range;
            fiff.loc = ch->loc;
            channel.fiff = fiff;
            info.channels.push_back(std::move(channel));
        }
    }
    if (info.channels.empty() || (nchan && static_cast<std::size_t>(*nchan) != info.channels.size())) {
        return AcqError(AcqErrorCode::UnsupportedFormat,
                        std::format("the measurement info describes {} of {} channels", info.channels.size(), nchan.value_or(0)));
    }
    if (info.sfreq <= 0.0) {
        return AcqError(AcqErrorCode::UnsupportedFormat, "the measurement info has no sampling rate");
    }
    info.extensions.push_back({kMeasInfoSchema, meas_info_file(meas_info)});
    return info;
}

//=============================================================================================================

std::optional<std::size_t> mne::detail::trigger_channel(const AcqStreamInfo& info)
{
    for (const char* name : {"STI 014", "STI101"}) {
        const auto it = std::find_if(info.channels.begin(), info.channels.end(), [&](const AcqChannelInfo& c) { return c.name == name; });
        if (it != info.channels.end()) {
            return static_cast<std::size_t>(it - info.channels.begin());
        }
    }
    const auto it = std::find_if(info.channels.begin(), info.channels.end(), [](const AcqChannelInfo& c) { return c.kind == AcqChannelKind::Stim; });
    return it == info.channels.end() ? std::nullopt : std::optional<std::size_t>(static_cast<std::size_t>(it - info.channels.begin()));
}
