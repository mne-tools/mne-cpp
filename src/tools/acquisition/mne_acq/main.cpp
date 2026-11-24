//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    mne_acq: list, inspect, record and re-publish acquisition sources from the command line.
 *
 *   mne_acq list [--timeout s]                         schemes and reachable sources with ready-to-use URIs
 *   mne_acq info <uri>                                 device, streams and channels of a source
 *   mne_acq record <uri> <file.fif> [--duration s]     record the first stream to FIFF
 *   mne_acq stream <uri> --lsl [--name n] [--duration s]   publish the first stream as an LSL outlet
 *   mne_acq stream <uri> --brainflow-multicast a:p [--master-board id]   publish it for BrainFlow's STREAMING_BOARD
 *   mne_acq view <uri> [--window s] [--duration s] [--screenshot file.png]   live traces in a sweep display
 *
 * record and stream run until the duration has passed, the source ends (e.g. a file replay without loop) or
 * Ctrl+C. Exit codes: 0 success, 1 usage error, 2 the source or the output failed.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq.h>
#include <mne/acq/devices/acq_devices.h>
#include <mne/acq/devices/brainflow/brainflow_stream.h>
#include <mne/acq/fiff/acq_fiff_replay.h>
#include <mne/acq/fiff/acq_recorder.h>
#include <mne/acq/lsl/acq_lsl.h>
#if defined(MNE_ACQ_VIEW)
#include "acq_view.h"
#endif
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>
#include <mne/lsl/lsl_stream_info.h>
#include <mne/lsl/lsl_stream_outlet.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

constexpr int kUsageError = 1;
constexpr int kRunError = 2;

std::atomic<bool> g_interrupted{false};

extern "C" void onInterrupt(int /*signal*/)
{
    g_interrupted.store(true);
}

const char* formatName(AcqSampleFormat format)
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return "int16";
        case AcqSampleFormat::Int24Packed:
            return "int24";
        case AcqSampleFormat::Int32:
            return "int32";
        case AcqSampleFormat::Float32:
            return "float32";
        case AcqSampleFormat::Float64:
            return "float64";
        case AcqSampleFormat::ComplexInt16:
            return "complex int16";
        case AcqSampleFormat::ComplexFloat32:
            return "complex float32";
    }
    return "?";
}

const char* kindName(AcqChannelKind kind)
{
    static constexpr const char* kNames[] = {"unknown", "EEG", "MEG", "OPM", "ECG", "EOG", "ERG", "EMG",
                                             "sEEG", "ECoG", "DBS", "fNIRS", "respiration", "GSR", "stim",
                                             "accelerometer", "gyroscope", "temperature", "timestamp", "marker"};
    const auto index = static_cast<std::size_t>(kind);
    return index < std::size(kNames) ? kNames[index] : "other";
}

std::string unitName(const AcqChannelInfo& channel)
{
    static constexpr const char* kUnits[] = {"?", "", "V", "A", "T", "T/m", "Am", "m/s^2", "rad/s", "degC",
                                             "s", "Ohm", "W", "Pa", "M"};
    const auto index = static_cast<std::size_t>(channel.unit);
    std::string unit = index < std::size(kUnits) ? kUnits[index] : "?";
    if (channel.unitExponent != 0 && !unit.empty()) {
        unit = "1e" + std::to_string(channel.unitExponent) + " " + unit;
    }
    return unit;
}

/** Registers the schemes of the bridge and device libraries, which a link may otherwise drop. */
void registerSchemes()
{
    registerFiffScheme();
    registerLslScheme();
    static_cast<void>(register_device_schemes());
}

/** Opens @p uri, printing the error; all schemes are registered first. */
AcqExpected<AcqDevice> openSource(const std::string& uri)
{
    registerSchemes();
    auto device = AcqDevice::open(uri);
    if (!device) {
        std::print(stderr, "mne_acq: {}: {}\n", uri, device.error().text());
    }
    return device;
}

/** Waits until @p done(), the duration (0 = none) has passed or Ctrl+C. */
template<typename Done>
void waitUntil(double seconds, Done done)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (!done() && !g_interrupted.load() && (seconds <= 0.0 || std::chrono::steady_clock::now() < deadline)) {
        std::this_thread::sleep_for(20ms);
    }
}

//=============================================================================================================

int list(double timeout)
{
    registerSchemes();
    AcqDeviceRegistry& registry = AcqDeviceRegistry::instance();
    std::print("Schemes:\n");
    for (const AcqSchemeInfo& scheme : registry.schemes()) {
        std::print("  {:<14} {}\n", scheme.scheme + "://", scheme.description);
    }
    std::print("Sources (searched for {:.1f} s):\n", timeout);
    const auto found = registry.discover(std::chrono::milliseconds(static_cast<long long>(timeout * 1000.0)));
    if (found.empty()) {
        std::print("  none\n");
    }
    for (const AcqDiscoveredDevice& device : found) {
        std::print("  {}  {} ({}){}\n", device.uri, device.label, device.detail, device.available ? "" : " [unavailable]");
    }
    return 0;
}

//=============================================================================================================

int info(const std::string& uri)
{
    auto device = openSource(uri);
    if (!device) {
        return kRunError;
    }
    const AcqDeviceDescription description = device->description();
    std::print("{}\n  backend   {}{}\n  device    {}\n", description.uri, description.backend,
               description.backendVersion.empty() ? std::string() : " (" + description.backendVersion + ")", description.device);
    for (const AcqStream& stream : device->streams()) {
        const auto streamInfo = stream.info();
        std::print("Stream \"{}\": {} channels, {}, {:g} Hz{}{}\n", streamInfo->name, streamInfo->channels.size(),
                   formatName(streamInfo->format), streamInfo->sfreq, streamInfo->clockDomain.empty() ? "" : ", clock ",
                   streamInfo->clockDomain);
        for (const AcqChannelInfo& channel : streamInfo->channels) {
            std::print("  {:<16} {:<12} {}\n", channel.name, kindName(channel.kind), unitName(channel));
        }
    }
    device->close();
    return 0;
}

//=============================================================================================================

int record(const std::string& uri, const std::string& path, double duration)
{
    auto device = openSource(uri);
    if (!device) {
        return kRunError;
    }
    AcqRecorderOptions options;
    options.device = device->description();
    auto recorder = AcqRecorder::start(device->stream(), path, options);
    if (!recorder) {
        std::print(stderr, "mne_acq: {}: {}\n", path, recorder.error().text());
        return kRunError;
    }
    if (const AcqError started = device->start(); !started.ok()) {
        std::print(stderr, "mne_acq: {}\n", started.text());
        return kRunError;
    }
    waitUntil(duration, [&] { return !recorder->isRecording(); });
    device->close();
    const AcqError stopped = recorder->stop();
    const AcqRecorderStats stats = recorder->stats();
    std::print("Recorded {} samples ({} lost) and {} events to {} file(s):\n", stats.samples, stats.lost, stats.events,
               recorder->files().size());
    for (const std::string& file : recorder->files()) {
        std::print("  {}\n", file);
    }
    if (!stopped.ok()) {
        std::print(stderr, "mne_acq: {}\n", stopped.text());
        return kRunError;
    }
    return 0;
}

//=============================================================================================================

/** One decoded block: physical units, column-major channels x samples. */
struct Block
{
    std::vector<double> values;
    std::size_t samples = 0;
    std::uint64_t first = 0;
};

/** Hands every block of @p reader to @p sink until the duration (0: none), the end of the source or Ctrl+C. */
template<typename Sink>
int pump(AcqDevice& device, AcqReader& reader, double duration, Sink sink)
{
    if (const AcqError started = device.start(); !started.ok()) {
        std::print(stderr, "mne_acq: {}\n", started.text());
        return kRunError;
    }
    const std::size_t channels = reader.channelCount();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(duration);
    Block block;
    std::uint64_t published = 0;
    bool ended = false;
    while (!ended && !g_interrupted.load() && (duration <= 0.0 || std::chrono::steady_clock::now() < deadline)) {
        auto view = reader.acquire(1, 100ms);
        if (!view) {
            ended = view.error().code() != AcqErrorCode::Timeout;
            continue;
        }
        block.samples = view->sampleCount();
        block.first = view->firstSample();
        block.values.resize(block.samples * channels);
        const AcqError decoded = decode(*view, AcqMatrixRef<double>::columnMajor(block.values.data(), channels, block.samples));
        static_cast<void>(reader.release(*view));
        if (!decoded.ok()) {
            std::print(stderr, "mne_acq: {}\n", decoded.text());
            return kRunError;
        }
        sink(block);
        published += block.samples;
    }
    device.close();
    std::print("Published {} samples\n", published);
    return 0;
}

//=============================================================================================================

int streamLsl(AcqDevice& device, AcqReader& reader, const std::string& name, double duration)
{
    const auto streamInfo = reader.stream().info();
    const std::size_t channels = streamInfo->channels.size();
    const std::string type = channels > 0 ? kindName(streamInfo->channels.front().kind) : "unknown";
    mne::stream_outlet outlet(mne::stream_info(name.empty() ? streamInfo->name : name, type,
                                               static_cast<int>(channels), streamInfo->sfreq,
                                               mne::ChannelFormat::Float32, device.description().uri));
    std::print("Publishing {} as LSL stream \"{}\" ({} channels, {:g} Hz, float32)\n", device.description().uri,
               outlet.info().name(), channels, streamInfo->sfreq);
    std::fflush(stdout);
    std::vector<std::vector<float>> chunk;
    return pump(device, reader, duration, [&](const Block& block) {
        chunk.assign(block.samples, std::vector<float>(channels));
        for (std::size_t s = 0; s < block.samples; ++s) {
            for (std::size_t c = 0; c < channels; ++c) {
                chunk[s][c] = static_cast<float>(block.values[s * channels + c]);
            }
        }
        outlet.push_chunk(chunk);
    });
}

//=============================================================================================================

int streamBrainFlow(AcqDevice& device, AcqReader& reader, const std::string& target, int board, double duration)
{
    const std::size_t colon = target.find(':');
    const std::string address = target.substr(0, colon);
    // 0 for a missing or invalid number, which both checks reject.
    const int port = colon == std::string::npos ? 0 : to_int(std::string_view(target).substr(colon + 1)).value_or(0);
    const int first = to_int(address.substr(0, address.find('.'))).value_or(0);
    if (first < 224 || first > 239 || port <= 0 || port > 65535) {
        std::print(stderr, "mne_acq: --brainflow-multicast needs a multicast address:port, e.g. 225.1.1.1:6677\n");
        return kUsageError;
    }
    auto layout = detail::brainflow_board(std::to_string(board));
    if (!layout) {
        std::print(stderr, "mne_acq: {}\n", layout.error().text());
        return kRunError;
    }
    const detail::BrainFlowDescription& description = layout->second;
    // STREAMING_BOARD reads packets of BRAINFLOW_BATCH_SIZE samples (default 3), so both sides must agree.
    const char* batchText = std::getenv("BRAINFLOW_BATCH_SIZE");
    const int batchEnv = batchText != nullptr ? to_int(batchText).value_or(0) : 0;
    const std::size_t batch = batchEnv > 1 && batchEnv < 100 ? static_cast<std::size_t>(batchEnv) : 3;
    auto streamer = detail::BrainFlowStreamer::open(description, address, static_cast<std::uint16_t>(port), batch);
    if (!streamer) {
        std::print(stderr, "mne_acq: {}\n", streamer.error().text());
        return kRunError;
    }

    const auto streamInfo = reader.stream().info();
    const std::size_t channels = streamInfo->channels.size();
    const std::size_t mapped = std::min(channels, streamer->eeg_rows().size());
    std::print("Publishing {} in BrainFlow's format to {} as board {} ({}): channels 1-{} of {} on its EEG rows, volts "
               "as microvolts, {} samples per packet\n",
               device.description().uri, target, board, description.name, mapped, channels, batch);
    std::fflush(stdout);

    std::vector<double> scale(mapped, 1.0);
    for (std::size_t c = 0; c < mapped; ++c) {
        if (streamInfo->channels[c].unit == AcqUnit::Volt) {
            scale[c] = 1e6;
        }
    }
    std::vector<double> eeg(mapped);
    double t0 = -1.0;
    return pump(device, reader, duration, [&](const Block& block) {
        if (t0 < 0.0) {
            t0 = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count() -
                static_cast<double>(block.first) / streamInfo->sfreq;
        }
        for (std::size_t s = 0; s < block.samples; ++s) {
            for (std::size_t c = 0; c < mapped; ++c) {
                eeg[c] = block.values[s * channels + c] * scale[c];
            }
            static_cast<void>(streamer->push(eeg, 0.0, t0 + static_cast<double>(block.first + s) / streamInfo->sfreq));
        }
    });
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "List, inspect, record and re-publish acquisition sources.\n\n"
        "  mne_acq list [--timeout s]\n"
        "  mne_acq info <uri>\n"
        "  mne_acq record <uri> <file.fif> [--duration s]\n"
        "  mne_acq stream <uri> --lsl [--name n] [--duration s]\n"
        "  mne_acq stream <uri> --brainflow-multicast 225.1.1.1:6677 [--master-board id] [--duration s]\n"
        "  mne_acq view <uri> [--window s] [--duration s] [--screenshot file.png]\n\n"
        "URIs: synthetic://?eeg=32&sfreq=1000, fiff:///data/raw.fif?loop=1, lsl://NAME, ... (see mne_acq list).");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_positional_argument("command", "list, info, record, stream or view.");
    parser.add_positional_argument("[uri]", "Source URI.");
    parser.add_positional_argument("[file]", "Output file of record.");
    parser.add_option({{"timeout"}, "Seconds to search for sources (list, default 1).", "s", "1"});
    parser.add_option({{"duration"}, "Seconds to run (record, stream); 0 until the source ends or Ctrl+C.", "s", "0"});
    parser.add_option({{"lsl"}, "Publish as a Lab Streaming Layer outlet (stream).", "", ""});
    parser.add_option({{"name"}, "Name of the published stream (stream; default: the stream's name).", "n", ""});
    parser.add_option({{"brainflow-multicast"}, "Publish in BrainFlow's multicast format for its STREAMING_BOARD (stream).", "address:port", ""});
    parser.add_option({{"master-board"},
                       "BrainFlow board whose row layout is sent; consumers pass it as master_board "
                       "(default -1, the synthetic board).",
                       "id",
                       "-1"});
    parser.add_option({{"window"}, "Visible time window of view in seconds (default 10).", "s", "10"});
    parser.add_option({{"screenshot"}, "PNG of the view window when it closes (view).", "file", ""});
    parser.process(argc, argv, "mne_acq", MNE_CPP_VERSION);

    const std::vector<std::string>& args = parser.positional_arguments();
    const std::string command = args.empty() ? std::string() : args[0];
    const std::optional<double> timeout = to_double(parser.value("timeout"));
    const std::optional<double> duration = to_double(parser.value("duration"));
    if (!timeout || *timeout < 0.0 || !duration || *duration < 0.0) {
        std::print(stderr, "mne_acq: --timeout and --duration need non-negative seconds\n");
        return kUsageError;
    }
    std::signal(SIGINT, onInterrupt);
    std::signal(SIGTERM, onInterrupt);

    if (command == "list" && args.size() == 1) {
        return list(*timeout);
    }
    if (command == "info" && args.size() == 2) {
        return info(args[1]);
    }
    if (command == "record" && args.size() == 3) {
        return record(args[1], args[2], *duration);
    }
    if (command == "stream" && args.size() == 2) {
        if (parser.is_set("lsl") == parser.is_set("brainflow-multicast")) {
            std::print(stderr, "mne_acq: stream needs one output, --lsl or --brainflow-multicast\n");
            return kUsageError;
        }
        const std::optional<int> board = to_int(parser.value("master-board"));
        if (!board) {
            std::print(stderr, "mne_acq: --master-board needs a BrainFlow board id\n");
            return kUsageError;
        }
        auto device = openSource(args[1]);
        if (!device) {
            return kRunError;
        }
        auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
        if (!reader) {
            std::print(stderr, "mne_acq: {}\n", reader.error().text());
            return kRunError;
        }
        return parser.is_set("lsl") ? streamLsl(*device, *reader, parser.value("name"), *duration)
                                    : streamBrainFlow(*device, *reader, parser.value("brainflow-multicast"), *board, *duration);
    }
    if (command == "view" && args.size() == 2) {
        const std::optional<double> window = to_double(parser.value("window"));
        if (!window || *window <= 0.0) {
            std::print(stderr, "mne_acq: --window needs positive seconds\n");
            return kUsageError;
        }
#if defined(MNE_ACQ_VIEW)
        auto device = openSource(args[1]);
        if (!device) {
            return kRunError;
        }
        std::fflush(stderr);
        return runAcqView(argc, argv, *device, *window, *duration, parser.value("screenshot"));
#else
        std::print(stderr, "mne_acq: view needs a build with the display library (mne_disp)\n");
        return kRunError;
#endif
    }
    std::print(stderr, "{}\n", parser.help_text("mne_acq"));
    return kUsageError;
}
