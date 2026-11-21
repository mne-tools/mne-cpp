//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May, 2026
 * @brief    polhemus:// : Polhemus Fastrak / FastSCAN digitizers on a serial port, or their simulator.
 *
 * One input stream "poses" with a frame per update of all stations: for each station S its position "S X",
 * "S Y", "S Z" in metres and its orientation quaternion "S QW", "S QX", "S QY", "S QZ". A station not seen
 * yet reads NaN. The digitizer stops its continuous output while the stylus button is pressed; a pause of
 * the stream becomes a frame that repeats the last poses with event code 1, and the output is started again.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "polhemus_transport.h"
#include "../acq_devices.h"
#include "../transport_error.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"
#include <com/serial_port.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;
using mne::list_serial_ports;
using mne::SerialPort;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::chrono::milliseconds kWait(50);
constexpr std::chrono::microseconds kPoll(500);
constexpr std::size_t kValuesPerStation = 7;
constexpr std::int32_t kButtonCode = 1;

struct Config
{
    bool simulate = false;
    std::string port; // empty = the first USB-serial adapter
    int baud = 115200;
    FastrakUnits units = FastrakUnits::Inches;
    std::vector<int> stations{1, 2};
    std::optional<std::array<double, 3>> hemisphere;
    double rate = 120.0;
    std::chrono::milliseconds pause{300};
    PolhemusSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<std::vector<int>> parseStations(std::string_view text)
{
    std::array<bool, 5> chosen{};
    std::size_t begin = 0;
    while (begin <= text.size()) {
        const std::size_t end = std::min(text.find(',', begin), text.size());
        const std::string_view token = text.substr(begin, end - begin);
        int station = 0;
        const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), station);
        if (ec != std::errc() || ptr != token.data() + token.size() || station < 1 || station > 4 || chosen[static_cast<std::size_t>(station)]) {
            return invalid(std::format("stations={}: distinct stations 1..4 such as 1,2", text));
        }
        chosen[static_cast<std::size_t>(station)] = true;
        begin = end + 1;
    }
    std::vector<int> stations;
    for (int s = 1; s <= 4; ++s) {
        if (chosen[static_cast<std::size_t>(s)]) {
            stations.push_back(s);
        }
    }
    return stations;
}

AcqExpected<std::array<double, 3>> parseHemisphere(std::string_view text)
{
    std::string spaced(text);
    std::replace(spaced.begin(), spaced.end(), ',', ' ');
    std::istringstream stream(spaced);
    stream.imbue(std::locale::classic());
    std::array<double, 3> h{};
    if (!(stream >> h[0] >> h[1] >> h[2]) || !(stream >> std::ws).eof() || (h[0] == 0.0 && h[1] == 0.0 && h[2] == 0.0)) {
        return invalid(std::format("hemisphere={}: a direction x,y,z such as 0,0,1", text));
    }
    return h;
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    c.port = uri.target;
    auto stations = parseStations(uri.param("stations").value_or("1,2"));
    const auto simulate = paramBool(uri, "simulate", false);
    const auto baud = paramInt(uri, "baud", 115200, 1200, 230400);
    const auto rate = paramDouble(uri, "rate", 120.0, 1.0, 1000.0);
    const auto pause = paramInt(uri, "pause", 300, 10, 10000);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto button = paramDouble(uri, "button", 0.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {stations.error(), simulate.error(), baud.error(), rate.error(), pause.error(), seed.error(), realtime.error(),
                                  duration.error(), button.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    const std::string_view units = uri.param("units").value_or("in");
    if (units != "in" && units != "cm") {
        return invalid(std::format("units={}: in or cm", units));
    }
    if (const auto text = uri.param("hemisphere")) {
        auto hemisphere = parseHemisphere(*text);
        if (!hemisphere) {
            return hemisphere.error();
        }
        c.hemisphere = *hemisphere;
    }
    c.simulate = *simulate;
    if (c.simulate && !c.port.empty()) {
        return invalid("target '" + c.port + "': the simulator takes no serial port");
    }
    c.baud = static_cast<int>(*baud);
    c.units = units == "in" ? FastrakUnits::Inches : FastrakUnits::Centimetres;
    c.stations = std::move(*stations);
    c.rate = *rate;
    c.pause = std::chrono::milliseconds(*pause);
    c.simulation = {c.stations, c.units, c.rate, static_cast<std::uint64_t>(*seed), *realtime, *duration, *button};
    c.thread = std::move(*thread);
    return c;
}

/** A digitizer on a serial port. */
class PolhemusSerial final : public PolhemusTransport
{
public:
    PolhemusSerial(SerialPort port, std::string name)
    : m_port(std::move(port))
    , m_name(std::move(name))
    {
    }

    std::string description() const override
    {
        return "Fastrak on " + m_name;
    }

    AcqError write(std::string_view command) override
    {
        return transport_error(m_port.write(command), AcqErrorCode::DeviceIo, "cannot write to " + m_name);
    }

    AcqExpected<std::size_t> read(std::span<char> buffer, std::chrono::milliseconds timeout) override
    {
        auto got = m_port.read(buffer, timeout);
        if (!got) {
            return transport_error(got.error(), AcqErrorCode::DeviceIo, "cannot read " + m_name);
        }
        return *got;
    }

private:
    SerialPort m_port;
    std::string m_name;
};

AcqExpected<std::unique_ptr<PolhemusTransport>> openSerial(const Config& config)
{
    std::string name = config.port;
    if (name.empty()) {
        const std::vector<std::string> ports = list_serial_ports();
        if (ports.empty()) {
            return AcqError(AcqErrorCode::DeviceNotFound, "no serial port; give the digitizer's port as target or use simulate=1");
        }
        name = ports.front();
    }
    auto port = SerialPort::open(name, config.baud);
    if (!port) {
        return transport_error(port.error(), AcqErrorCode::DeviceIo, std::format("cannot open serial port {} at {} baud", name, config.baud));
    }
    return std::unique_ptr<PolhemusTransport>(std::make_unique<PolhemusSerial>(std::move(*port), std::move(name)));
}

/** @return The commands that enable the stations, select position and Euler angles, and set the hemisphere. */
std::string setupCommands(const Config& config)
{
    std::string commands;
    for (int s = 1; s <= 4; ++s) {
        const bool used = std::find(config.stations.begin(), config.stations.end(), s) != config.stations.end();
        commands += std::format("l{},{}\r", s, used ? 1 : 0);
    }
    for (const int s : config.stations) {
        commands += std::format("O{},2,4,1\r", s);
        if (config.hemisphere) {
            commands += std::format("H{},{},{},{}\r", s, (*config.hemisphere)[0], (*config.hemisphere)[1], (*config.hemisphere)[2]);
        }
    }
    return commands;
}

class PolhemusBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<PolhemusTransport> transport;
        if (config->simulate) {
            transport = make_polhemus_simulator(config->simulation);
        } else {
            auto serial = openSerial(*config);
            if (!serial) {
                return serial.error();
            }
            transport = std::move(*serial);
        }
        if (AcqError e = transport->write("P\r" + setupCommands(*config)); !e.ok()) {
            return e;
        }

        AcqStreamInfo info;
        info.name = "poses";
        info.sourceId = transport->description();
        info.format = AcqSampleFormat::Float32;
        info.layout = AcqLayout::Interleaved;
        info.sfreq = config->rate / static_cast<double>(config->stations.size());
        for (const int s : config->stations) {
            for (const char* axis : {"X", "Y", "Z"}) {
                AcqChannelInfo& channel = info.channels.emplace_back();
                channel.name = std::format("{} {}", s, axis);
                channel.kind = AcqChannelKind::Position;
                channel.unit = AcqUnit::Meter;
            }
            for (const char* component : {"QW", "QX", "QY", "QZ"}) {
                AcqChannelInfo& channel = info.channels.emplace_back();
                channel.name = std::format("{} {}", s, component);
                channel.kind = AcqChannelKind::Orientation;
                channel.unit = AcqUnit::None;
            }
        }

        std::unique_ptr<PolhemusBackend> backend(new PolhemusBackend(std::move(*config), std::move(transport), uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = 4096;
        auto writer = backend->addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~PolhemusBackend() override
    {
        stop();
    }

    PolhemusBackend(const PolhemusBackend&) = delete;
    PolhemusBackend& operator=(const PolhemusBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"polhemus", "", "Polhemus " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->write("C\r"); !e.ok()) {
            return e;
        }
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_config.rate / static_cast<double>(m_config.stations.size());
        clock.anchorSample = m_writer->writePosition();
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);
        m_stop.store(false, std::memory_order_relaxed);
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            static_cast<void>(m_transport->write("P\r"));
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
            static_cast<void>(m_transport->write("P\r"));
        }
    }

private:
    PolhemusBackend(Config config, std::unique_ptr<PolhemusTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    , m_frame(m_config.stations.size() * kValuesPerStation, std::numeric_limits<float>::quiet_NaN())
    {
    }

    /** @return false if the frame could not be written. */
    bool commitFrame()
    {
        AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
        while (!view && view.error() == AcqErrorCode::Timeout) {
            if (m_stop.load(std::memory_order_acquire)) {
                return false;
            }
            view = m_writer->beginWrite(1, kPoll); // a gating reader may not have freed the space yet
        }
        if (!view) {
            reportError(view.error());
            return false;
        }
        std::memcpy(view->part(0).at(0, 0), m_frame.data(), m_frame.size() * sizeof(float));
        m_writer->commit(1);
        return true;
    }

    /** Decodes the complete lines in m_line; @return false on a write error. */
    bool decodeLines()
    {
        std::size_t begin = 0;
        for (std::size_t end = m_line.find('\n'); end != std::string::npos; end = m_line.find('\n', begin)) {
            std::string_view line(m_line.data() + begin, end - begin);
            begin = end + 1;
            if (!line.empty() && line.back() == '\r') {
                line.remove_suffix(1);
            }
            const auto record = parse_fastrak_record(line, m_config.units);
            if (!record) {
                continue; // blank lines and command echoes
            }
            const auto station = std::find(m_config.stations.begin(), m_config.stations.end(), record->station);
            if (station == m_config.stations.end()) {
                continue;
            }
            float* values = m_frame.data() + static_cast<std::size_t>(station - m_config.stations.begin()) * kValuesPerStation;
            std::copy(record->position.begin(), record->position.end(), values);
            std::copy(record->orientation.begin(), record->orientation.end(), values + 3);
            if (station + 1 == m_config.stations.end() && !commitFrame()) {
                return false;
            }
        }
        m_line.erase(0, begin);
        return true;
    }

    void run()
    {
        std::array<char, 1024> chunk{};
        auto last = std::chrono::steady_clock::now();
        bool streaming = false;
        while (!m_stop.load(std::memory_order_acquire)) {
            auto got = m_transport->read(chunk, std::min(kWait, m_config.pause));
            if (!got) {
                if (got.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                    return;
                }
                if (got.error() != AcqErrorCode::Timeout) {
                    reportError(got.error());
                    return;
                }
                if (streaming && std::chrono::steady_clock::now() - last >= m_config.pause) {
                    // The stylus button paused the output: a frame of its own repeats the pose it was pressed at
                    streaming = false;
                    if (AcqError e = m_writer->pushEvent({m_writer->writePosition(), 0, kButtonCode}); !e.ok()) {
                        reportError(e);
                        return;
                    }
                    if (!commitFrame()) {
                        return;
                    }
                    if (AcqError e = m_transport->write("C\r"); !e.ok()) {
                        reportError(e);
                        return;
                    }
                }
                continue;
            }
            streaming = true;
            last = std::chrono::steady_clock::now();
            m_line.append(chunk.data(), *got);
            if (!decodeLines()) {
                return;
            }
        }
    }

    Config m_config;
    std::unique_ptr<PolhemusTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::vector<float> m_frame; // owned by the acquisition thread while it runs
    std::string m_line;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "polhemus";
    info.description = "Polhemus Fastrak and FastSCAN digitizers on a serial port (the target, e.g. /dev/cu.usbserial-1 or COM3; "
                       "empty for the first USB-serial adapter): positions in metres and orientation quaternions per station, "
                       "stylus button presses as event code 1. simulate=1 sweeps a stylus over a head instead.";
    info.exampleUri = "polhemus://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the digitizer instead of opening a serial port."},
        {"stations", "string", "1,2", "Stations with a sensor, e.g. 1,2 for stylus and tracker."},
        {"units", "enum:in|cm", "in", "Unit of the digitizer's positions: in (Fastrak default) or cm (FastSCAN)."},
        {"baud", "int", "115200", "Bits per second of the serial port."},
        {"hemisphere", "string", "", "Tracking hemisphere x,y,z sent to every station, e.g. 0,0,1; empty keeps the digitizer's."},
        {"rate", "double", "120", "Records per second the digitizer sends, shared by its stations."},
        {"pause", "int", "300", "Milliseconds without records that count as a stylus button press."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"seed", "int", "0", "Simulation: phase of the stylus sweep."},
        {"realtime", "bool", "1", "Simulation: deliver records at the digitizer's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"button", "double", "0", "Simulation: seconds between stylus button presses; 0 never."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds /*timeout*/)
{
    std::vector<AcqDiscoveredDevice> found;
    for (const std::string& port : list_serial_ports()) {
        found.push_back({"polhemus://" + port, "Polhemus", "polhemus", "serial port " + port, true});
    }
    return found;
}

/** Registers polhemus:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_polhemus_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_polhemus_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &PolhemusBackend::open, &discover);
    return result;
}
