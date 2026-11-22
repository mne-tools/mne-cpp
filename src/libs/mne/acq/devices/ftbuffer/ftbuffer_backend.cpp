//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffer_backend.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    ftbuffer:// : a FieldTrip buffer server, e.g. the one neuromag2ft fills on a Neuromag system.
 *
 * One input stream "raw" in the buffer's sample format, from the samples put after start(). With the Neuromag
 * header chunk (neuromag2ft), the channels and the measurement info, including the digitizer points of the
 * isotrak chunk, are those of the acquisition, its floats physical values, and changes of STI 014 (or STI101)
 * events. Otherwise the channels are named after the channel-name chunk, or numbered. Events put in the buffer
 * with a numeric value become events with that code. A new header (a new measurement) ends the stream.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_transport.h"
#include "../acq_devices.h"
#include "../neuromag/neuromag_fiff.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstring>
#include <deque>
#include <format>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::chrono::milliseconds kWait(100);
constexpr std::chrono::milliseconds kAnswer(5000); // beyond the wait the request asks for
constexpr std::chrono::microseconds kPoll(500);
constexpr std::uint16_t kDefaultPort = 1972;
constexpr std::uint32_t kAll = 0xFFFFFFFFU;

struct Config
{
    bool simulate = false;
    std::string host = "localhost";
    std::uint16_t port = kDefaultPort;
    std::chrono::milliseconds timeout{10000};
    FtBufferSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

template<typename T>
T get(std::span<const std::byte> data, std::size_t offset)
{
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof(T));
    return value;
}

template<typename T>
void put(std::vector<std::byte>& out, T value)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    const auto simulate = paramBool(uri, "simulate", false);
    const auto timeout = paramDouble(uri, "timeout", 10.0, 0.1, 3600.0);
    const auto meg = paramInt(uri, "meg", 306, 0, 4096);
    const auto eeg = paramInt(uri, "eeg", 0, 0, 4096);
    const auto rate = paramDouble(uri, "rate", 1000.0, 1.0, 1e5);
    const auto block = paramInt(uri, "block", 50, 1, 100000);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), timeout.error(), meg.error(), eeg.error(), rate.error(), block.error(), seed.error(),
                                  realtime.error(), duration.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    if (!uri.target.empty()) {
        if (c.simulate) {
            return invalid("target '" + uri.target + "': the simulator takes no server");
        }
        const std::size_t colon = uri.target.rfind(':');
        c.host = uri.target.substr(0, colon);
        if (colon != std::string::npos) {
            const std::string_view port = std::string_view(uri.target).substr(colon + 1);
            unsigned value = 0;
            const auto [end, ec] = std::from_chars(port.data(), port.data() + port.size(), value);
            if (ec != std::errc() || end != port.data() + port.size() || value == 0 || value > 65535) {
                return invalid("target '" + uri.target + "': host:port with a port 1..65535");
            }
            c.port = static_cast<std::uint16_t>(value);
        }
        if (c.host.empty()) {
            return invalid("target '" + uri.target + "': host:port");
        }
    }
    if (*meg + *eeg == 0) {
        return invalid("meg=0&eeg=0: the simulated system needs channels");
    }
    c.timeout = std::chrono::milliseconds(static_cast<std::int64_t>(*timeout * 1000.0));
    c.simulation = {static_cast<std::size_t>(*meg), static_cast<std::size_t>(*eeg), *rate, static_cast<std::size_t>(*block),
                    static_cast<std::uint64_t>(*seed), *realtime, *duration};
    c.thread = std::move(*thread);
    return c;
}

/** @return The sample format of a FieldTrip data type, if the stream can hold it as it is. */
std::optional<AcqSampleFormat> format_of(std::uint32_t type)
{
    switch (type) {
        case ft::kTypeInt16:
            return AcqSampleFormat::Int16;
        case ft::kTypeInt32:
            return AcqSampleFormat::Int32;
        case ft::kTypeFloat32:
            return AcqSampleFormat::Float32;
        case ft::kTypeFloat64:
            return AcqSampleFormat::Float64;
        default:
            return std::nullopt;
    }
}

std::size_t width_of(AcqSampleFormat format)
{
    return format == AcqSampleFormat::Int16 ? 2 : format == AcqSampleFormat::Float64 ? 8
                                                                                     : 4;
}

/** @return The value at @p p as a number. */
double value_at(const std::byte* p, AcqSampleFormat format)
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return get<std::int16_t>({p, 2}, 0);
        case AcqSampleFormat::Int32:
            return get<std::int32_t>({p, 4}, 0);
        case AcqSampleFormat::Float64:
            return get<double>({p, 8}, 0);
        default:
            return get<float>({p, 4}, 0);
    }
}

/** @return The first value of an event's value field as an event code, if it is an integral number. */
std::optional<std::int32_t> event_code(std::uint32_t type, std::span<const std::byte> value)
{
    const auto integral = [](double v) -> std::optional<std::int32_t> {
        if (!std::isfinite(v) || v != std::trunc(v) || std::abs(v) > 2147483647.0) {
            return std::nullopt;
        }
        return static_cast<std::int32_t>(v);
    };
    const auto need = [&](std::size_t bytes) {
        return value.size() >= bytes;
    };
    switch (type) {
        case ft::kTypeUint8:
            return need(1) ? std::optional(static_cast<std::int32_t>(get<std::uint8_t>(value, 0))) : std::nullopt;
        case ft::kTypeInt8:
            return need(1) ? std::optional(static_cast<std::int32_t>(get<std::int8_t>(value, 0))) : std::nullopt;
        case ft::kTypeUint16:
            return need(2) ? std::optional(static_cast<std::int32_t>(get<std::uint16_t>(value, 0))) : std::nullopt;
        case ft::kTypeInt16:
            return need(2) ? std::optional(static_cast<std::int32_t>(get<std::int16_t>(value, 0))) : std::nullopt;
        case ft::kTypeUint32:
            return need(4) ? integral(static_cast<double>(get<std::uint32_t>(value, 0))) : std::nullopt;
        case ft::kTypeInt32:
            return need(4) ? std::optional(get<std::int32_t>(value, 0)) : std::nullopt;
        case ft::kTypeUint64:
            return need(8) ? integral(static_cast<double>(get<std::uint64_t>(value, 0))) : std::nullopt;
        case ft::kTypeInt64:
            return need(8) ? integral(static_cast<double>(get<std::int64_t>(value, 0))) : std::nullopt;
        case ft::kTypeFloat32:
            return need(4) ? integral(get<float>(value, 0)) : std::nullopt;
        case ft::kTypeFloat64:
            return need(8) ? integral(get<double>(value, 0)) : std::nullopt;
        default:
            return std::nullopt; // strings
    }
}

/** @return Bytes per element of a FieldTrip type. */
std::size_t type_size(std::uint32_t type)
{
    constexpr std::array<std::size_t, 11> kSizes = {1, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8};
    return type < kSizes.size() ? kSizes[type] : 0;
}

/** The buffer's header: counts, rate, sample format and chunks. */
struct Header
{
    std::uint32_t nchans = 0;
    std::uint32_t nsamples = 0;
    std::uint32_t nevents = 0;
    float fsample = 0.0F;
    std::uint32_t data_type = 0;
    std::vector<std::pair<std::uint32_t, std::span<const std::byte>>> chunks;
    std::vector<std::byte> bytes; // owns the chunk data

    [[nodiscard]] std::span<const std::byte> chunk(std::uint32_t type) const
    {
        const auto it = std::find_if(chunks.begin(), chunks.end(), [type](const auto& c) { return c.first == type; });
        return it == chunks.end() ? std::span<const std::byte>() : it->second;
    }
};

AcqExpected<Header> parse_header(std::vector<std::byte> payload)
{
    if (payload.size() < ft::kHeaderDefBytes) {
        return AcqError(AcqErrorCode::DeviceIo, "the FieldTrip buffer sent a truncated header");
    }
    Header h;
    h.nchans = get<std::uint32_t>(payload, 0);
    h.nsamples = get<std::uint32_t>(payload, 4);
    h.nevents = get<std::uint32_t>(payload, 8);
    h.fsample = get<float>(payload, 12);
    h.data_type = get<std::uint32_t>(payload, 16);
    h.bytes = std::move(payload);
    const std::span<const std::byte> all(h.bytes);
    const std::size_t size = std::min<std::size_t>(get<std::uint32_t>(all, 20), all.size() - ft::kHeaderDefBytes);
    std::span<const std::byte> rest = all.subspan(ft::kHeaderDefBytes, size);
    while (rest.size() >= 8) {
        const auto type = get<std::uint32_t>(rest, 0);
        const std::size_t length = std::min<std::size_t>(get<std::uint32_t>(rest, 4), rest.size() - 8);
        h.chunks.emplace_back(type, rest.subspan(8, length));
        rest = rest.subspan(8 + length);
    }
    return h;
}

/** @return The stream of the buffer's header. */
AcqExpected<AcqStreamInfo> stream_of(const Header& h, const std::string& source)
{
    const auto format = format_of(h.data_type);
    if (!format) {
        return AcqError(AcqErrorCode::UnsupportedFormat, std::format("FieldTrip data type {} (int16, int32, float32 and float64 are read)", h.data_type));
    }
    if (h.nchans == 0 || !(h.fsample > 0.0F)) {
        return AcqError(AcqErrorCode::UnsupportedFormat, std::format("the FieldTrip header has {} channels at {} Hz", h.nchans, h.fsample));
    }
    if (const auto neuromag = h.chunk(ft::kChunkNeuromagHeader); !neuromag.empty()) {
        // neuromag2ft writes the acquisition's tags in its own byte order, the isotrak file as a FIFF file
        std::vector<NeuromagTag> meas_info = block_of(parse_tags(neuromag, false), fiff::kBlockMeasInfo);
        if (const auto isotrak = h.chunk(ft::kChunkNeuromagIsotrak); !isotrak.empty() && !meas_info.empty()) {
            const std::vector<NeuromagTag> dig = block_of(parse_tags(isotrak, true), fiff::kBlockIsotrak);
            meas_info.insert(meas_info.end() - 1, dig.begin(), dig.end());
        }
        auto info = stream_info(meas_info, source, *format);
        if (!info) {
            return info.error();
        }
        if (info->channelCount() != h.nchans) {
            return AcqError(AcqErrorCode::UnsupportedFormat,
                            std::format("the Neuromag header describes {} channels, the FieldTrip header {}", info->channelCount(), h.nchans));
        }
        info->sfreq = h.fsample;
        return info;
    }
    AcqStreamInfo info;
    info.name = "raw";
    info.sourceId = source;
    info.format = *format;
    info.layout = AcqLayout::Interleaved;
    info.sfreq = h.fsample;
    std::span<const std::byte> names = h.chunk(ft::kChunkChannelNames);
    for (std::uint32_t c = 0; c < h.nchans; ++c) {
        AcqChannelInfo& channel = info.channels.emplace_back();
        const auto* begin = reinterpret_cast<const char*>(names.data());
        const auto length = static_cast<std::size_t>(std::find(names.begin(), names.end(), std::byte{0}) - names.begin());
        channel.name = length > 0 ? std::string(begin, length) : std::format("CH {:03}", c + 1);
        names = names.subspan(std::min(names.size(), length + 1));
        channel.kind = AcqChannelKind::Misc;
    }
    return info;
}

class FtBufferBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<FtBufferTransport> transport;
        if (config->simulate) {
            transport = make_ftbuffer_simulator(config->simulation);
        } else {
            auto client = connect_ftbuffer(config->host, config->port, std::min(config->timeout, std::chrono::milliseconds(5000)));
            if (!client) {
                return client.error();
            }
            transport = std::move(*client);
        }
        auto header = read_header(*transport, config->timeout);
        if (!header) {
            return header.error();
        }
        const bool neuromag = !header->chunk(ft::kChunkNeuromagHeader).empty();
        auto info = stream_of(*header, transport->description());
        if (!info) {
            return info.error();
        }

        std::unique_ptr<FtBufferBackend> backend(new FtBufferBackend(std::move(*config), std::move(transport), uri.toString()));
        backend->m_format = info->format;
        backend->m_width = info->channelCount();
        backend->m_trigger_channel = neuromag ? trigger_channel(*info) : std::nullopt;
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4.0 * info->sfreq, 4096.0));
        backend->m_chunk = options.capacitySamples / 4;
        backend->m_rate = info->sfreq;
        auto writer = backend->addInputStream(std::move(*info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~FtBufferBackend() override
    {
        stop();
    }

    FtBufferBackend(const FtBufferBackend&) = delete;
    FtBufferBackend& operator=(const FtBufferBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"ftbuffer", "", "FieldTrip buffer " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        // Samples and events already in the buffer belong to the time before the stream
        auto counts = wait({kAll, kAll}, std::chrono::milliseconds(0));
        if (!counts) {
            return counts.error();
        }
        m_offset = m_next = counts->first;
        m_next_event = counts->second;
        m_pending.clear();
        m_last_event = 0;
        m_trigger.reset();
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_rate;
        clock.anchorSample = m_writer->writePosition();
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);
        m_stop.store(false, std::memory_order_relaxed);
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            return thread.error();
        }
        m_thread = std::move(*thread);
        return {};
    }

    void stop() override
    {
        m_stop.store(true, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    FtBufferBackend(Config config, std::unique_ptr<FtBufferTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    {
    }

    /** @return The header, once a producer has put one. */
    static AcqExpected<Header> read_header(FtBufferTransport& transport, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true) {
            auto answer = transport.request(ft::kGetHdr, {}, kAnswer);
            if (!answer) {
                return answer.error();
            }
            if (answer->command == ft::kGetOk) {
                return parse_header(std::move(answer->payload));
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                return AcqError(AcqErrorCode::Timeout, std::format("{} has no header; is the producer running?", transport.description()));
            }
            std::this_thread::sleep_for(kWait);
        }
    }

    /** @return The numbers of samples and events in the buffer once they exceed @p threshold, or after @p timeout. */
    AcqExpected<std::pair<std::uint32_t, std::uint32_t>> wait(std::pair<std::uint32_t, std::uint32_t> threshold, std::chrono::milliseconds timeout)
    {
        std::vector<std::byte> payload;
        put(payload, threshold.first);
        put(payload, threshold.second);
        put(payload, static_cast<std::uint32_t>(timeout.count()));
        auto answer = m_transport->request(ft::kWaitDat, payload, timeout + kAnswer);
        if (!answer) {
            return answer.error();
        }
        if (answer->command != ft::kWaitOk || answer->payload.size() < 8) {
            return AcqError(AcqErrorCode::DeviceIo, "the FieldTrip buffer refuses to wait for data; has its header gone?");
        }
        return std::pair{get<std::uint32_t>(answer->payload, 0), get<std::uint32_t>(answer->payload, 4)};
    }

    /** Fetches the events [m_next_event, @p count) into m_pending. */
    AcqError fetch_events(std::uint32_t count)
    {
        std::vector<std::byte> payload;
        put(payload, static_cast<std::int32_t>(m_next_event));
        put(payload, static_cast<std::int32_t>(count - 1));
        auto answer = m_transport->request(ft::kGetEvt, payload, kAnswer);
        if (!answer) {
            return answer.error();
        }
        m_next_event = count;
        if (answer->command != ft::kGetOk) {
            return {}; // overwritten before they were read
        }
        std::span<const std::byte> rest(answer->payload);
        while (rest.size() >= ft::kEventDefBytes) {
            const auto type_type = get<std::uint32_t>(rest, 0);
            const auto type_numel = get<std::uint32_t>(rest, 4);
            const auto value_type = get<std::uint32_t>(rest, 8);
            const auto sample = get<std::int32_t>(rest, 16);
            const std::size_t size = std::min<std::size_t>(get<std::uint32_t>(rest, 28), rest.size() - ft::kEventDefBytes);
            const std::size_t skip = std::min(size, std::size_t{type_numel} * type_size(type_type));
            const auto code = event_code(value_type, rest.subspan(ft::kEventDefBytes + skip, size - skip));
            if (code && sample >= 0 && static_cast<std::uint64_t>(sample) >= m_offset) {
                m_pending.push_back({static_cast<std::uint64_t>(sample) - m_offset, 0, *code});
            }
            rest = rest.subspan(ft::kEventDefBytes + size);
        }
        std::stable_sort(m_pending.begin(), m_pending.end(), [](const AcqEvent& a, const AcqEvent& b) { return a.sample < b.sample; });
        return {};
    }

    /** Pushes @p event unless it lies before an event already pushed. */
    AcqError push(const AcqEvent& event)
    {
        if (event.sample < m_last_event) {
            return {};
        }
        m_last_event = event.sample;
        return m_writer->pushEvent(event);
    }

    /** Reads the samples [m_next, m_next + @p count) into the ring. */
    AcqError fetch_samples(std::uint32_t count)
    {
        std::vector<std::byte> payload;
        put(payload, m_next);
        put(payload, m_next + count - 1);
        auto answer = m_transport->request(ft::kGetDat, payload, kAnswer);
        if (!answer) {
            return answer.error();
        }
        const std::size_t value = width_of(m_format);
        const std::size_t frame = m_width * value;
        const std::span<const std::byte> data(answer->payload);
        if (answer->command != ft::kGetOk) {
            return {AcqErrorCode::DeviceIo, "the FieldTrip buffer overwrote samples before they were read; read faster or enlarge it"};
        }
        if (data.size() < ft::kDataDefBytes + count * frame || get<std::uint32_t>(data, 0) != m_width || get<std::uint32_t>(data, 4) != count ||
            format_of(get<std::uint32_t>(data, 8)) != m_format) {
            return {AcqErrorCode::DeviceIo, "the FieldTrip buffer sent samples that do not match its header"};
        }
        AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
        while (!view && view.error() == AcqErrorCode::Timeout) {
            if (m_stop.load(std::memory_order_acquire)) {
                return {};
            }
            view = m_writer->beginWrite(count, kPoll); // a gating reader may not have freed the space yet
        }
        if (!view) {
            return view.error();
        }
        const std::uint64_t first = m_writer->writePosition();
        const std::byte* source = data.data() + ft::kDataDefBytes;
        std::size_t s = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                const std::byte* values = source + s * frame;
                std::memcpy(part.at(0, i), values, frame);
                const std::uint64_t at = first + s;
                while (!m_pending.empty() && m_pending.front().sample <= at) {
                    if (AcqError e = push(m_pending.front()); !e.ok()) {
                        return e;
                    }
                    m_pending.pop_front();
                }
                if (m_trigger_channel) {
                    const auto code = static_cast<std::int32_t>(std::lround(value_at(values + *m_trigger_channel * value, m_format)));
                    if (m_trigger.changed(code)) {
                        if (AcqError e = push({at, 0, code}); !e.ok()) {
                            return e;
                        }
                    }
                }
            }
        }
        m_writer->commit(count);
        m_next += count;
        return {};
    }

    void run()
    {
        while (!m_stop.load(std::memory_order_acquire)) {
            auto counts = wait({m_next, m_next_event}, kWait);
            if (!counts) {
                if (counts.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(counts.error());
                }
                return;
            }
            const auto [samples, events] = *counts;
            if (samples < m_next || events < m_next_event) {
                m_writer->finish(); // a new header: the measurement this stream describes is over
                return;
            }
            if (events > m_next_event) {
                if (AcqError e = fetch_events(events); !e.ok()) {
                    reportError(e);
                    return;
                }
            }
            while (m_next < samples && !m_stop.load(std::memory_order_acquire)) {
                if (AcqError e = fetch_samples(static_cast<std::uint32_t>(std::min<std::size_t>(samples - m_next, m_chunk))); !e.ok()) {
                    reportError(e);
                    return;
                }
            }
        }
    }

    Config m_config;
    std::unique_ptr<FtBufferTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    AcqSampleFormat m_format = AcqSampleFormat::Float32;
    std::size_t m_width = 0;
    std::size_t m_chunk = 1024; // samples per data request
    double m_rate = 0.0;
    std::optional<std::size_t> m_trigger_channel;
    // Owned by the acquisition thread while it runs
    std::uint32_t m_offset = 0;     // buffer sample of stream sample 0
    std::uint32_t m_next = 0;       // next buffer sample to read
    std::uint32_t m_next_event = 0; // next buffer event to read
    std::deque<AcqEvent> m_pending; // buffer events whose samples have not arrived
    std::uint64_t m_last_event = 0;
    TriggerWord m_trigger;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "ftbuffer";
    info.description = "A FieldTrip buffer server (the target, host:port, default localhost:1972), e.g. the one neuromag2ft fills on a "
                       "Neuromag acquisition computer; with its Neuromag header the channels and measurement info of the acquisition. "
                       "simulate=1 relays a simulated Neuromag acquisition instead.";
    info.exampleUri = "ftbuffer://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate neuromag2ft and its buffer instead of connecting to a server."},
        {"timeout", "double", "10", "Seconds to wait for the connection and for the producer's header."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"meg", "int", "306", "Simulation: magnetometer channels."},
        {"eeg", "int", "0", "Simulation: EEG channels."},
        {"rate", "double", "1000", "Simulation: sampling rate in Hz."},
        {"block", "int", "50", "Simulation: samples per data put."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"realtime", "bool", "1", "Simulation: put samples at the acquisition's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
    };
    return info;
}

/** Registers ftbuffer:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_ftbuffer_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_ftbuffer_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &FtBufferBackend::open);
    return result;
}
