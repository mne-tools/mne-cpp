//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     streaming_board_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    streaming_board:// : the samples a BrainFlow session streams with add_streamer, without BrainFlow.
 *
 * BrainFlow's streamer ("streaming_board://<address>:<port>") sends a board's samples as UDP datagrams of whole
 * samples, num_rows doubles each; BrainFlow's STREAMING_BOARD reads them. This backend reads them too: the target is
 * the address and port, board= the board behind the stream, whose layout comes from a table generated from BrainFlow
 * or, for other boards, from BrainFlow when it is installed. The stream is that of brainflow://, and the sender's
 * marker row becomes events. simulate=1 streams a synthetic:// session in the board's layout to itself.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflow_stream.h"

#include "../acq_devices.h"
#include "../../acq_backend.h"
#include "../../acq_device.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

#include <com/udp_socket.h>

#include <atomic>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstring>
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

static_assert(std::endian::native == std::endian::little, "BrainFlow's streamer sends doubles in the sender's byte order");

constexpr std::size_t kMaxDatagram = 65536;
constexpr std::size_t kSimulatedBatch = 3; // BrainFlow's BRAINFLOW_BATCH_SIZE default
constexpr std::chrono::milliseconds kWait(200);

struct Config
{
    bool simulate = false;
    std::string address = "127.0.0.1";
    std::uint16_t port = 0;
    std::string board = "synthetic";
    std::int64_t seed = 0;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    const auto simulate = paramBool(uri, "simulate", false);
    const auto seed = paramInt(uri, "seed", 0, 0, INT32_MAX);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), seed.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    c.seed = *seed;
    c.thread = std::move(*thread);
    if (const auto board = uri.param("board")) {
        c.board = std::string(*board);
    } else if (!c.simulate) {
        return invalid("streaming_board:// needs board=, the BrainFlow board behind the stream, e.g. board=synthetic");
    }
    if (c.simulate) {
        if (!uri.target.empty()) {
            return invalid("target '" + uri.target + "': the simulator streams to itself");
        }
        return c;
    }
    const std::size_t colon = uri.target.rfind(':');
    int port = 0;
    if (colon != std::string::npos) {
        const char* first = uri.target.data() + colon + 1;
        const char* last = uri.target.data() + uri.target.size();
        const auto [end, error] = std::from_chars(first, last, port);
        if (error != std::errc() || end != last) {
            port = 0;
        }
    }
    if (colon == std::string::npos || port <= 0 || port > 65535) {
        return invalid("streaming_board:// needs <address>:<port> as BrainFlow's add_streamer has it, e.g. "
                       "streaming_board://225.1.1.1:6677?board=synthetic");
    }
    c.address = uri.target.substr(0, colon);
    c.port = static_cast<std::uint16_t>(port);
    return c;
}

/** Streams a synthetic:// session in a board's layout, as a BrainFlow session with add_streamer would. */
class Simulator
{
public:
    static AcqExpected<std::unique_ptr<Simulator>> start(const BrainFlowDescription& description, std::uint16_t port, std::int64_t seed)
    {
        auto streamer = BrainFlowStreamer::open(description, "127.0.0.1", port, kSimulatedBatch);
        if (!streamer) {
            return streamer.error();
        }
        const std::size_t eeg = streamer->eeg_rows().size();
        auto session = AcqDevice::open(std::format("synthetic://?sfreq={}&eeg={}&stim=1&seed={}&realtime=1", description.sampling_rate,
                                                   std::max<std::size_t>(eeg, 1), seed));
        if (!session) {
            return session.error();
        }
        AcqReaderOptions gating;
        gating.gating = true;
        auto reader = session->stream().reader(AcqReader::ReadMode::Lossless, gating);
        if (!reader) {
            return reader.error();
        }
        std::unique_ptr<Simulator> simulator(new Simulator(std::move(*streamer), std::move(*session), std::move(*reader), description.sampling_rate));
        if (AcqError e = simulator->m_session.start(); !e.ok()) {
            return e;
        }
        simulator->m_thread = std::thread([s = simulator.get()] { s->run(); });
        return simulator;
    }

    ~Simulator()
    {
        m_stop.store(true, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
        m_session.close();
    }

    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

private:
    Simulator(BrainFlowStreamer streamer, AcqDevice session, AcqReader reader, double sfreq)
    : m_streamer(std::move(streamer))
    , m_session(std::move(session))
    , m_reader(std::move(reader))
    , m_sfreq(sfreq)
    {
    }

    void run()
    {
        const std::size_t channels = m_reader.stream().info()->channelCount(); // EEG, then the trigger code
        std::vector<double> eeg(channels - 1);
        const double t0 = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::uint64_t sample = 0;
        float previous = 0.0F;
        while (!m_stop.load(std::memory_order_acquire)) {
            auto view = m_reader.acquire(1, kWait);
            if (!view) {
                if (view.error() == AcqErrorCode::Timeout) {
                    continue;
                }
                return;
            }
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqBlockPart& part = view->part(p);
                for (std::size_t s = 0; s < part.samples; ++s, ++sample) {
                    for (std::size_t c = 0; c + 1 < channels; ++c) {
                        float volts = 0.0F;
                        std::memcpy(&volts, part.at(c, s), sizeof(volts));
                        eeg[c] = static_cast<double>(volts) * 1e6;
                    }
                    float code = 0.0F;
                    std::memcpy(&code, part.at(channels - 1, s), sizeof(code));
                    const double marker = previous == 0.0F ? code : 0.0; // a marker at the onset of the trigger pulse
                    previous = code;
                    if (!m_streamer.push(eeg, marker, t0 + static_cast<double>(sample) / m_sfreq).ok()) {
                        static_cast<void>(m_reader.release(*view));
                        return;
                    }
                }
            }
            if (!m_reader.release(*view).ok()) {
                return;
            }
        }
    }

    BrainFlowStreamer m_streamer;
    AcqDevice m_session;
    AcqReader m_reader;
    double m_sfreq = 0.0;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

class StreamingBoardBackend final : public AcqBackend
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
        auto socket = mne::UdpSocket::bind(config->address, config->port);
        if (!socket) {
            const AcqErrorCode code = socket.error() == std::errc::invalid_argument ? AcqErrorCode::InvalidParameter : AcqErrorCode::DeviceIo;
            return AcqError(code, std::format("cannot receive on {}:{}: {}", config->address, config->port, socket.error().message()));
        }
        std::unique_ptr<StreamingBoardBackend> backend(new StreamingBoardBackend(std::move(*config), std::move(*socket), id, description, uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4 * description.sampling_rate, 1024));
        auto writer = backend->addInputStream(brainflow_stream_info(description, std::format("streaming_board:{}", id)), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = BrainFlowRowWriter(**writer, description);
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~StreamingBoardBackend() override
    {
        stop();
    }

    StreamingBoardBackend(const StreamingBoardBackend&) = delete;
    StreamingBoardBackend& operator=(const StreamingBoardBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        const std::string from = m_config.simulate ? std::string("its simulator") : std::format("{}:{}", m_config.address, m_config.port);
        return {"streaming_board", "", std::format("BrainFlow stream of {} (board {}) from {}", m_description.name, m_board, from), m_uri};
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
        if (m_config.simulate) {
            auto simulator = Simulator::start(m_description, m_socket.port(), m_config.seed);
            if (!simulator) {
                stop();
                return simulator.error();
            }
            m_simulator = std::move(*simulator);
        }
        return {};
    }

    void stop() override
    {
        m_simulator.reset();
        m_stop.store(true, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    StreamingBoardBackend(Config config, mne::UdpSocket socket, int board, const BrainFlowDescription& description, std::string uri)
    : m_config(std::move(config))
    , m_socket(std::move(socket))
    , m_board(board)
    , m_description(description)
    , m_uri(std::move(uri))
    , m_datagram(kMaxDatagram / sizeof(double))
    {
    }

    void run()
    {
        const std::size_t rows = m_description.rows;
        const std::size_t frame = rows * sizeof(double);
        const auto stopping = [this] {
            return m_stop.load(std::memory_order_acquire);
        };
        while (!stopping()) {
            const auto got = m_socket.receive(std::as_writable_bytes(std::span(m_datagram)), kWait);
            if (!got) {
                if (got.error() == std::errc::timed_out) {
                    continue;
                }
                reportError({AcqErrorCode::DeviceIo, "receiving: " + got.error().message()});
                return;
            }
            if (*got == 0 || *got % frame != 0) {
                continue; // not whole samples of this board, as BrainFlow's STREAMING_BOARD drops them
            }
            const AcqError written = m_writer.write(m_datagram.data(), *got / frame, rows, 1, stopping);
            if (written.code() == AcqErrorCode::DeviceStopped) {
                return;
            }
            if (!written.ok()) {
                reportError(written);
                return;
            }
        }
    }

    Config m_config;
    mne::UdpSocket m_socket;
    int m_board = 0;
    BrainFlowDescription m_description;
    std::string m_uri;
    std::vector<double> m_datagram;
    BrainFlowRowWriter m_writer;
    std::unique_ptr<Simulator> m_simulator;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "streaming_board";
    info.description = "The samples a BrainFlow session streams with add_streamer(\"streaming_board://<address>:<port>\"), "
                       "received without BrainFlow; the target is that address and port, board= the board behind the stream.";
    info.exampleUri = "streaming_board://225.1.1.1:6677?board=synthetic";
    info.parameters = {
        {"board", "string", "", "BrainFlow board id or name behind the stream, e.g. synthetic, cyton, ganglion, cytondaisy or -1."},
        {"simulate", "bool", "0", "Stream a synthetic session in the board's layout to itself instead of receiving one."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"cpus", "string", "", "Comma-separated CPUs for the receiving thread."},
        {"priority", "string", "normal", "Receiving thread priority: normal, high or realtime."},
    };
    return info;
}

/** Registers streaming_board:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_streaming_board_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_streaming_board_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &StreamingBoardBackend::open);
    return result;
}
