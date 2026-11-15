//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
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

#include <acq/acq.h>
#include <acq/devices/acq_devices.h>
#include <acq/devices/brainflow/brainflow_stream.h>
#include <acq/fiff/acq_fiff_replay.h>
#include <acq/fiff/acq_recorder.h>
#include <acq/lsl/acq_lsl.h>
#if defined(MNE_ACQ_VIEW)
#include "acq_view.h"

#include <QApplication>
#endif

#include <lsl/lsl_stream_info.h>
#include <lsl/lsl_stream_outlet.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <string_view>
#include <optional>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

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

/** Standard output; flushed when the program ends. */
QTextStream& out()
{
    static QTextStream stream(stdout);
    return stream;
}

/** Standard error; flushed when the program ends. */
QTextStream& err()
{
    static QTextStream stream(stderr);
    return stream;
}

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
AcqExpected<AcqDevice> openSource(const QString& uri)
{
    registerSchemes();
    auto device = AcqDevice::open(uri.toStdString());
    if (!device) {
        err() << QString::asprintf("mne_acq: %s: %s\n", qPrintable(uri), device.error().text().c_str());
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
    out() << QString::asprintf("Schemes:\n");
    for (const AcqSchemeInfo& scheme : registry.schemes()) {
        out() << QString::asprintf("  %-14s %s\n", (scheme.scheme + "://").c_str(), scheme.description.c_str());
    }
    out() << QString::asprintf("Sources (searched for %.1f s):\n", timeout);
    const auto found = registry.discover(std::chrono::milliseconds(static_cast<long long>(timeout * 1000.0)));
    if (found.empty()) {
        out() << QString::asprintf("  none\n");
    }
    for (const AcqDiscoveredDevice& device : found) {
        out() << QString::asprintf("  %s  %s (%s)%s\n", device.uri.c_str(), device.label.c_str(), device.detail.c_str(),
                                   device.available ? "" : " [unavailable]");
    }
    return 0;
}

//=============================================================================================================

int info(const QString& uri)
{
    auto device = openSource(uri);
    if (!device) {
        return kRunError;
    }
    const AcqDeviceDescription description = device->description();
    out() << QString::asprintf("%s\n  backend   %s%s\n  device    %s\n", description.uri.c_str(), description.backend.c_str(),
                               description.backendVersion.empty() ? "" : (" (" + description.backendVersion + ")").c_str(),
                               description.device.c_str());
    for (const AcqStream& stream : device->streams()) {
        const auto streamInfo = stream.info();
        out() << QString::asprintf("Stream \"%s\": %zu channels, %s, %g Hz%s%s\n", streamInfo->name.c_str(), streamInfo->channels.size(),
                                   formatName(streamInfo->format), streamInfo->sfreq,
                                   streamInfo->clockDomain.empty() ? "" : ", clock ", streamInfo->clockDomain.c_str());
        for (const AcqChannelInfo& channel : streamInfo->channels) {
            out() << QString::asprintf("  %-16s %-12s %s\n", channel.name.c_str(), kindName(channel.kind), unitName(channel).c_str());
        }
    }
    device->close();
    return 0;
}

//=============================================================================================================

int record(const QString& uri, const QString& path, double duration)
{
    auto device = openSource(uri);
    if (!device) {
        return kRunError;
    }
    AcqRecorderOptions options;
    options.device = device->description();
    auto recorder = AcqRecorder::start(device->stream(), path.toStdString(), options);
    if (!recorder) {
        err() << QString::asprintf("mne_acq: %s: %s\n", qPrintable(path), recorder.error().text().c_str());
        return kRunError;
    }
    if (const AcqError started = device->start(); !started.ok()) {
        err() << QString::asprintf("mne_acq: %s\n", started.text().c_str());
        return kRunError;
    }
    waitUntil(duration, [&] { return !recorder->isRecording(); });
    device->close();
    const AcqError stopped = recorder->stop();
    const AcqRecorderStats stats = recorder->stats();
    out() << QString::asprintf("Recorded %llu samples (%llu lost) and %llu events to %zu file(s):\n",
                               static_cast<unsigned long long>(stats.samples), static_cast<unsigned long long>(stats.lost),
                               static_cast<unsigned long long>(stats.events), recorder->files().size());
    for (const std::string& file : recorder->files()) {
        out() << QString::asprintf("  %s\n", file.c_str());
    }
    if (!stopped.ok()) {
        err() << QString::asprintf("mne_acq: %s\n", stopped.text().c_str());
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
        err() << QString::asprintf("mne_acq: %s\n", started.text().c_str());
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
            err() << QString::asprintf("mne_acq: %s\n", decoded.text().c_str());
            return kRunError;
        }
        sink(block);
        published += block.samples;
    }
    device.close();
    out() << QString::asprintf("Published %llu samples\n", static_cast<unsigned long long>(published));
    return 0;
}

//=============================================================================================================

int streamLsl(AcqDevice& device, AcqReader& reader, const QString& name, double duration)
{
    const auto streamInfo = reader.stream().info();
    const std::size_t channels = streamInfo->channels.size();
    const std::string type = channels > 0 ? kindName(streamInfo->channels.front().kind) : "unknown";
    mne::stream_outlet outlet(mne::stream_info(name.isEmpty() ? streamInfo->name : name.toStdString(), type,
                                               static_cast<int>(channels), streamInfo->sfreq,
                                               mne::ChannelFormat::Float32, device.description().uri));
    out() << QString::asprintf("Publishing %s as LSL stream \"%s\" (%zu channels, %g Hz, float32)\n",
                               device.description().uri.c_str(), outlet.info().name().c_str(), channels,
                               streamInfo->sfreq);
    out().flush();
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

int streamBrainFlow(AcqDevice& device, AcqReader& reader, const QString& target, int board, double duration)
{
    const std::string address = target.section(QLatin1Char(':'), 0, 0).toStdString();
    bool portOk = false;
    const auto port = target.section(QLatin1Char(':'), 1).toUShort(&portOk);
    bool firstOk = false;
    const int first = QString::fromStdString(address).section(QLatin1Char('.'), 0, 0).toInt(&firstOk);
    if (!firstOk || first < 224 || first > 239 || !portOk || port == 0) {
        err() << QString::asprintf("mne_acq: --brainflow-multicast needs a multicast address:port, e.g. 225.1.1.1:6677\n");
        return kUsageError;
    }
    auto layout = detail::brainflow_board(std::to_string(board));
    if (!layout) {
        err() << QString::asprintf("mne_acq: %s\n", layout.error().text().c_str());
        return kRunError;
    }
    const detail::BrainFlowDescription& description = layout->second;
    // STREAMING_BOARD reads packets of BRAINFLOW_BATCH_SIZE samples (default 3), so both sides must agree.
    const int batchEnv = qEnvironmentVariableIntValue("BRAINFLOW_BATCH_SIZE");
    const std::size_t batch = batchEnv > 1 && batchEnv < 100 ? static_cast<std::size_t>(batchEnv) : 3;
    auto streamer = detail::BrainFlowStreamer::open(description, address, port, batch);
    if (!streamer) {
        err() << QString::asprintf("mne_acq: %s\n", streamer.error().text().c_str());
        return kRunError;
    }

    const auto streamInfo = reader.stream().info();
    const std::size_t channels = streamInfo->channels.size();
    const std::size_t mapped = std::min(channels, streamer->eeg_rows().size());
    out() << QString::asprintf("Publishing %s in BrainFlow's format to %s as board %d (%s): channels 1-%zu of %zu on "
                               "its EEG rows, volts as microvolts, %zu samples per packet\n",
                               device.description().uri.c_str(), qPrintable(target), board, description.name.c_str(), mapped,
                               channels, batch);
    out().flush();

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
#if defined(MNE_ACQ_VIEW)
    // Only view needs a GUI; the other commands also run without a display.
    bool gui = false;
    for (int i = 1; i < argc && !gui; ++i) {
        gui = std::string_view(argv[i]) == "view";
    }
    std::unique_ptr<QCoreApplication> app = gui ? std::make_unique<QApplication>(argc, argv)
                                                : std::make_unique<QCoreApplication>(argc, argv);
#else
    auto app = std::make_unique<QCoreApplication>(argc, argv);
#endif
    QCoreApplication::setApplicationName("mne_acq");
    QCoreApplication::setApplicationVersion(MNE_CPP_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "List, inspect, record and re-publish acquisition sources.\n\n"
        "  mne_acq list [--timeout s]\n"
        "  mne_acq info <uri>\n"
        "  mne_acq record <uri> <file.fif> [--duration s]\n"
        "  mne_acq stream <uri> --lsl [--name n] [--duration s]\n"
        "  mne_acq stream <uri> --brainflow-multicast 225.1.1.1:6677 [--master-board id] [--duration s]\n"
        "  mne_acq view <uri> [--window s] [--duration s] [--screenshot file.png]\n\n"
        "URIs: synthetic://?eeg=32&sfreq=1000, fiff:///data/raw.fif?loop=1, lsl://NAME, ... (see mne_acq list).");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command", "list, info, record, stream or view.");
    parser.addPositionalArgument("uri", "Source URI.", "[uri]");
    parser.addPositionalArgument("file", "Output file of record.", "[file]");
    const QCommandLineOption timeoutOpt("timeout", "Seconds to search for sources (list, default 1).", "s", "1");
    const QCommandLineOption durationOpt("duration", "Seconds to run (record, stream); 0 until the source ends or Ctrl+C.", "s", "0");
    const QCommandLineOption lslOpt("lsl", "Publish as a Lab Streaming Layer outlet (stream).");
    const QCommandLineOption nameOpt("name", "Name of the published stream (stream; default: the stream's name).", "n");
    const QCommandLineOption brainFlowOpt("brainflow-multicast",
                                          "Publish in BrainFlow's multicast format for its STREAMING_BOARD (stream).",
                                          "address:port");
    const QCommandLineOption boardOpt("master-board",
                                      "BrainFlow board whose row layout is sent; consumers pass it as master_board "
                                      "(default -1, the synthetic board).",
                                      "id", "-1");
    const QCommandLineOption windowOpt("window", "Visible time window of view in seconds (default 10).", "s", "10");
    const QCommandLineOption screenshotOpt("screenshot", "PNG of the view window when it closes (view).", "file");
    parser.addOptions({timeoutOpt, durationOpt, lslOpt, nameOpt, brainFlowOpt, boardOpt, windowOpt, screenshotOpt});
    parser.process(*app);

    const QStringList args = parser.positionalArguments();
    const QString command = args.value(0);
    bool ok = true;
    const double timeout = parser.value(timeoutOpt).toDouble(&ok);
    bool durationOk = true;
    const double duration = parser.value(durationOpt).toDouble(&durationOk);
    if (!ok || timeout < 0.0 || !durationOk || duration < 0.0) {
        err() << QString::asprintf("mne_acq: --timeout and --duration need non-negative seconds\n");
        return kUsageError;
    }
    std::signal(SIGINT, onInterrupt);
    std::signal(SIGTERM, onInterrupt);

    if (command == QLatin1String("list") && args.size() == 1) {
        return list(timeout);
    }
    if (command == QLatin1String("info") && args.size() == 2) {
        return info(args[1]);
    }
    if (command == QLatin1String("record") && args.size() == 3) {
        return record(args[1], args[2], duration);
    }
    if (command == QLatin1String("stream") && args.size() == 2) {
        if (parser.isSet(lslOpt) == parser.isSet(brainFlowOpt)) {
            err() << QString::asprintf("mne_acq: stream needs one output, --lsl or --brainflow-multicast\n");
            return kUsageError;
        }
        bool boardOk = false;
        const int board = parser.value(boardOpt).toInt(&boardOk);
        if (!boardOk) {
            err() << QString::asprintf("mne_acq: --master-board needs a BrainFlow board id\n");
            return kUsageError;
        }
        auto device = openSource(args[1]);
        if (!device) {
            return kRunError;
        }
        auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
        if (!reader) {
            err() << QString::asprintf("mne_acq: %s\n", reader.error().text().c_str());
            return kRunError;
        }
        return parser.isSet(lslOpt)
            ? streamLsl(*device, *reader, parser.value(nameOpt), duration)
            : streamBrainFlow(*device, *reader, parser.value(brainFlowOpt), board, duration);
    }
    if (command == QLatin1String("view") && args.size() == 2) {
        bool windowOk = false;
        const double window = parser.value(windowOpt).toDouble(&windowOk);
        if (!windowOk || window <= 0.0) {
            err() << QString::asprintf("mne_acq: --window needs positive seconds\n");
            return kUsageError;
        }
#if defined(MNE_ACQ_VIEW)
        auto device = openSource(args[1]);
        if (!device) {
            return kRunError;
        }
        err().flush();
        return runAcqView(*device, window, duration, parser.value(screenshotOpt));
#else
        err() << QString::asprintf("mne_acq: view needs a build with the display library (mne_disp)\n");
        return kRunError;
#endif
    }
    err() << QString::asprintf("%s\n", qPrintable(parser.helpText()));
    return kUsageError;
}
