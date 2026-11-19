//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_file_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    brainflow_file:// : a file BrainFlow wrote, replayed without BrainFlow.
 *
 * BrainFlow's file streamer (add_streamer("file://<path>:w")) and DataFilter.write_file write a sample per line,
 * the board's rows separated by tabs. The target is the file, board= the board that recorded it, whose layout gives
 * the channels of brainflow://. The replay is paced in real time or as fast as it is read, once or in a loop;
 * markers become events and the timestamp row is the clock.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflow_stream.h"

#include "../acq_devices.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

#include <atomic>
#include <charconv>
#include <cmath>
#include <format>
#include <fstream>
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

using Clock = std::chrono::steady_clock;

constexpr std::chrono::milliseconds kPoll(20);

struct Config
{
    std::string path;
    std::string board;
    bool realtime = true;
    bool loop = false;
    double speed = 1.0;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    const auto realtime = paramBool(uri, "realtime", true);
    const auto loop = paramBool(uri, "loop", false);
    const auto speed = paramDouble(uri, "speed", 1.0, 0.01, 1e4);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {realtime.error(), loop.error(), speed.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.path = uri.target;
    if (c.path.empty()) {
        return invalid("brainflow_file:// needs the file, e.g. brainflow_file:///data/session.csv?board=cyton");
    }
    const auto board = uri.param("board");
    if (!board) {
        return invalid("brainflow_file:// needs board=, the BrainFlow board that recorded the file, e.g. board=cyton");
    }
    c.board = std::string(*board);
    c.realtime = *realtime;
    c.loop = *loop;
    c.speed = *speed;
    c.thread = std::move(*thread);
    return c;
}

/** @return The samples of @p path, @p rows doubles each, or DeviceNotFound / UnsupportedFormat naming the line. */
AcqExpected<std::vector<double>> readSamples(const std::string& path, std::size_t rows)
{
    std::ifstream file(path);
    if (!file) {
        return AcqError(AcqErrorCode::DeviceNotFound, "cannot open " + path);
    }
    std::vector<double> samples;
    std::string line;
    for (std::size_t number = 1; std::getline(file, line); ++number) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        std::size_t values = 0;
        const char* at = line.data();
        const char* end = line.data() + line.size();
        while (at < end) {
            double value = 0.0;
            const auto [next, error] = std::from_chars(at, end, value);
            if (error != std::errc() || (next != end && *next != '\t')) {
                return AcqError(AcqErrorCode::UnsupportedFormat, std::format("{} line {}: not tab-separated numbers", path, number));
            }
            samples.push_back(value);
            ++values;
            at = next == end ? end : next + 1;
        }
        if (values != rows) {
            return AcqError(AcqErrorCode::UnsupportedFormat,
                            std::format("{} line {}: {} values, but the board has {} rows; is board= the board that recorded it?", path,
                                        number, values, rows));
        }
    }
    if (samples.empty()) {
        return AcqError(AcqErrorCode::UnsupportedFormat, path + " holds no samples");
    }
    return samples;
}

class BrainFlowFileBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        auto board = brainflow_board(config->board);
        if (!board) {
            return board.error();
        }
        auto& [id, description] = *board;
        if (description.sampling_rate <= 0 || description.rows == 0) {
            return AcqError(AcqErrorCode::InvalidParameter, std::format("BrainFlow board {} describes no samples", id));
        }
        auto samples = readSamples(config->path, description.rows);
        if (!samples) {
            return samples.error();
        }
        std::unique_ptr<BrainFlowFileBackend> backend(new BrainFlowFileBackend(std::move(*config), std::move(*samples), description, uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4 * description.sampling_rate, 1024));
        auto writer = backend->addInputStream(brainflow_stream_info(description, std::format("brainflow_file:{}", id)), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = BrainFlowRowWriter(**writer, description);
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~BrainFlowFileBackend() override
    {
        stop();
    }

    BrainFlowFileBackend(const BrainFlowFileBackend&) = delete;
    BrainFlowFileBackend& operator=(const BrainFlowFileBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"brainflow_file", "", std::format("{} recorded by BrainFlow {}", m_config.path, m_name), m_uri};
    }

    AcqError start() override
    {
        m_stop.store(false, std::memory_order_relaxed);
        m_writer.restart();
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
    BrainFlowFileBackend(Config config, std::vector<double> samples, const BrainFlowDescription& description, std::string uri)
    : m_config(std::move(config))
    , m_samples(std::move(samples))
    , m_name(description.name)
    , m_uri(std::move(uri))
    , m_rows(description.rows)
    , m_sfreq(description.sampling_rate)
    , m_block(static_cast<std::size_t>(std::max(1.0, std::round(m_sfreq / 100.0))))
    {
    }

    void run()
    {
        const auto stopping = [this] {
            return m_stop.load(std::memory_order_acquire);
        };
        const std::size_t length = m_samples.size() / m_rows;
        const Clock::time_point begin = Clock::now();
        std::uint64_t written = 0;
        while (!stopping()) {
            if (m_position == length) {
                if (!m_config.loop) {
                    m_writer.finish();
                    return;
                }
                m_position = 0;
            }
            const std::size_t n = std::min(m_block, length - m_position);
            if (m_config.realtime) {
                const auto due = begin + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(written + n) / (m_sfreq * m_config.speed)));
                while (!stopping() && Clock::now() < due) {
                    std::this_thread::sleep_until(std::min(due, Clock::now() + kPoll));
                }
            }
            const AcqError error = m_writer.write(m_samples.data() + m_position * m_rows, n, m_rows, 1, stopping);
            if (error.code() == AcqErrorCode::DeviceStopped) {
                return;
            }
            if (!error.ok()) {
                reportError(error);
                return;
            }
            m_position += n;
            written += n;
        }
    }

    Config m_config;
    std::vector<double> m_samples; // the file, sample after sample
    std::string m_name;
    std::string m_uri;
    std::size_t m_rows = 0;
    double m_sfreq = 0.0;
    std::size_t m_block = 1;
    std::size_t m_position = 0; // next sample of the file
    BrainFlowRowWriter m_writer;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "brainflow_file";
    info.description = "Replays a file BrainFlow wrote (its file streamer or DataFilter.write_file: a sample per line, rows "
                       "separated by tabs) without BrainFlow; board= names the board that recorded it.";
    info.exampleUri = "brainflow_file:///data/session.csv?board=synthetic";
    info.parameters = {
        {"board", "string", "", "BrainFlow board id or name that recorded the file, e.g. synthetic, cyton, ganglion, cytondaisy or -1."},
        {"realtime", "bool", "1", "Pace at the board's sampling rate; 0 replays as fast as the samples are read."},
        {"speed", "double", "1", "Pacing factor of realtime, e.g. 10 replays ten times faster."},
        {"loop", "bool", "0", "Restart at the first sample at the end; 0 ends the stream there."},
        {"cpus", "string", "", "Comma-separated CPUs for the replay thread."},
        {"priority", "string", "normal", "Replay thread priority: normal, high or realtime."},
    };
    return info;
}

/** Registers brainflow_file:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_brainflow_file_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_brainflow_file_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &BrainFlowFileBackend::open);
    return result;
}
