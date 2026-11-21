//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_devices.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the vendor device sources of mne_acq_devices, run against their simulators.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_convert.h>
#include <acq/acq_device.h>
#include <acq/acq_device_registry.h>
#include <acq/devices/acq_devices.h>
#include <com/udp_socket.h>
#if !defined(_WIN32)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

/** Everything a Lossless reader delivered until EndOfStream, in physical units, frame after frame. */
struct Recording
{
    std::size_t channels = 0;
    std::vector<double> samples;
    std::vector<AcqEvent> events;
    std::vector<double> scale; /**< Per channel. */

    [[nodiscard]] std::size_t frames() const
    {
        return channels == 0 ? 0 : samples.size() / channels;
    }

    [[nodiscard]] double at(std::size_t frame, std::size_t channel) const
    {
        return samples[frame * channels + channel];
    }
};

/** Everything a Lossless reader of @p device delivers from start() until EndOfStream. */
Recording record_device(AcqDevice& device)
{
    Recording out;
    const auto info = device.stream().info();
    out.channels = info->channelCount();
    for (const AcqChannelInfo& channel : info->channels) {
        out.scale.push_back(channel.scale);
    }
    // Replays as fast as they are read wait for the reader instead of overrunning it
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device.stream().reader(AcqReader::ReadMode::Lossless, gating);
    if (!reader || !device.start().ok()) {
        return out;
    }
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            if (view.error() != AcqErrorCode::EndOfStream) {
                qWarning("acquire: %s", view.error().text().c_str());
            }
            break;
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < out.channels; ++c) {
                    if (info->format == AcqSampleFormat::Int16) {
                        std::int16_t raw = 0;
                        std::memcpy(&raw, part.at(c, s), sizeof(raw));
                        out.samples.push_back(raw * out.scale[c] + info->channels[c].offset);
                    } else if (info->format == AcqSampleFormat::Int32) {
                        std::int32_t raw = 0;
                        std::memcpy(&raw, part.at(c, s), sizeof(raw));
                        out.samples.push_back(raw * out.scale[c] + info->channels[c].offset);
                    } else if (info->format == AcqSampleFormat::Float64) {
                        double v = 0.0;
                        std::memcpy(&v, part.at(c, s), sizeof(v));
                        out.samples.push_back(v * out.scale[c] + info->channels[c].offset);
                    } else {
                        float v = 0.0F;
                        std::memcpy(&v, part.at(c, s), sizeof(v));
                        out.samples.push_back(v * out.scale[c] + info->channels[c].offset);
                    }
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            out.events.push_back(view->events()[e]);
        }
        static_cast<void>(reader->release(*view));
    }
    return out;
}

Recording record(const std::string& uri)
{
    auto device = AcqDevice::open(uri);
    if (!device) {
        qWarning("open(%s): %s", uri.c_str(), device.error().text().c_str());
        return {};
    }
    return record_device(*device);
}

AcqErrorCode openError(const std::string& uri)
{
    auto device = AcqDevice::open(uri);
    return device ? AcqErrorCode::None : device.error().code();
}

#if !defined(_WIN32)
/**
 * A Neuromag data server and collector on this machine, enough for the client: the measurement info of three
 * channels (two MEG, STI 014) at 600 Hz on every connection, and on the second one a data buffer from the
 * measurement file (big-endian), one in a datagram, and the end of the file.
 */
class FakeDacq
{
public:
    FakeDacq()
    {
        char dir[] = "/tmp/nmXXXXXX"; // short: socket paths are limited to about 100 characters
        if (::mkdtemp(dir) == nullptr) {
            return;
        }
        m_root = dir;
        ::mkdir((m_root + "/sockets").c_str(), 0700);
        ::mkdir((m_root + "/raw").c_str(), 0700);
        write_file();

        m_server = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        const sockaddr_un address = unix_address(m_root + "/sockets/dacq_server");
        if (m_server < 0 || ::bind(m_server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
            return;
        }
        m_listener = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in tcp{};
        tcp.sin_family = AF_INET;
        tcp.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t length = sizeof(tcp);
        if (m_listener < 0 || ::bind(m_listener, reinterpret_cast<sockaddr*>(&tcp), sizeof(tcp)) != 0 || ::listen(m_listener, 4) != 0 ||
            ::getsockname(m_listener, reinterpret_cast<sockaddr*>(&tcp), &length) != 0) {
            return;
        }
        m_port = ntohs(tcp.sin_port);
        m_threads.emplace_back([this] { serve_data(); });
        m_threads.emplace_back([this] { serve_collector(); });
    }

    ~FakeDacq()
    {
        m_stop = true;
        ::shutdown(m_listener, SHUT_RDWR);
        ::close(m_listener);
        ::close(m_server);
        for (std::thread& thread : m_threads) {
            thread.join();
        }
        std::filesystem::remove_all(m_root);
    }

    FakeDacq(const FakeDacq&) = delete;
    FakeDacq& operator=(const FakeDacq&) = delete;

    [[nodiscard]] bool ok() const
    {
        return m_port > 0;
    }

    [[nodiscard]] const std::string& root() const
    {
        return m_root;
    }

    [[nodiscard]] int port() const
    {
        return m_port;
    }

    [[nodiscard]] std::vector<std::string> commands()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_commands;
    }

private:
    static sockaddr_un unix_address(const std::string& path)
    {
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
        return address;
    }

    static bool readable(int fd)
    {
        pollfd p{fd, POLLIN, 0};
        return ::poll(&p, 1, 50) > 0 && (p.revents & POLLIN) != 0;
    }

    static std::vector<std::byte> ints(std::initializer_list<std::int32_t> values)
    {
        std::vector<std::byte> data(values.size() * 4);
        std::memcpy(data.data(), std::data(values), data.size());
        return data;
    }

    static std::vector<std::byte> channel(int scan, const char* name, int kind, int coil, int unit, float cal, float range)
    {
        std::vector<std::byte> data(96);
        const std::int32_t head[3] = {scan, scan, kind};
        std::memcpy(data.data(), head, sizeof(head));
        std::memcpy(data.data() + 12, &range, 4);
        std::memcpy(data.data() + 16, &cal, 4);
        std::memcpy(data.data() + 20, &coil, 4);
        std::memcpy(data.data() + 72, &unit, 4);
        std::memcpy(data.data() + 80, name, std::strlen(name));
        return data;
    }

    /** Samples @p first..first+3 of the three channels. */
    static std::vector<std::int32_t> samples(int first)
    {
        std::vector<std::int32_t> values;
        for (int s = first; s < first + 4; ++s) {
            values.insert(values.end(), {100 + s, -s, s >= 5 ? 3 : 0});
        }
        return values;
    }

    void write_file() const
    {
        std::ofstream file(m_root + "/raw/meas.fif", std::ios::binary);
        for (std::int32_t value : samples(0)) {
            const auto bits = static_cast<std::uint32_t>(value);
            const std::array<char, 4> big = {static_cast<char>(bits >> 24), static_cast<char>(bits >> 16), static_cast<char>(bits >> 8),
                                             static_cast<char>(bits)};
            file.write(big.data(), 4);
        }
    }

    void send(const sockaddr_un& to, int kind, int type, const std::vector<std::byte>& data, int loc = -1)
    {
        const std::int32_t message[6] = {kind, type, static_cast<std::int32_t>(data.size()), loc, -1, -1};
        ::sendto(m_server, message, sizeof(message), 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
        if (loc < 0 && !data.empty()) {
            ::sendto(m_server, data.data(), data.size(), 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
        }
    }

    void serve_data()
    {
        int session = 0;
        while (!m_stop) {
            if (!readable(m_server)) {
                continue;
            }
            sockaddr_un from{};
            socklen_t length = sizeof(from);
            std::int32_t id = 0;
            if (::recvfrom(m_server, &id, sizeof(id), 0, reinterpret_cast<sockaddr*>(&from), &length) != sizeof(id)) {
                continue;
            }
            const std::int32_t ok = 0;
            ::sendto(m_server, &ok, sizeof(ok), 0, reinterpret_cast<const sockaddr*>(&from), length);
            if (id <= 0) {
                continue;
            }
            const float sfreq = 600.0F;
            std::vector<std::byte> rate(4);
            std::memcpy(rate.data(), &sfreq, 4);
            send(from, 104, 3, ints({101}));
            send(from, 200, 3, ints({3}));
            send(from, 201, 4, rate);
            send(from, 203, 30, channel(1, "MEG 0111", 1, 3024, 112, 1.0F, 1.0F));
            send(from, 203, 30, channel(2, "MEG 0112", 1, 3012, 201, 2.0F, 0.5F));
            send(from, 203, 30, channel(3, "STI 014", 3, 0, -1, 1.0F, 1.0F));
            send(from, 105, 3, ints({101}));
            if (++session == 2) {
                const std::string path = m_root + "/raw/meas.fif";
                std::vector<std::byte> name(path.size() + 1);
                std::memcpy(name.data(), path.c_str(), path.size());
                send(from, 1, 10, name);
                send(from, 300, 3, std::vector<std::byte>(48), 0);
                const std::vector<std::int32_t> second = samples(4);
                std::vector<std::byte> data(48);
                std::memcpy(data.data(), second.data(), data.size());
                send(from, 300, 3, data);
                send(from, 2, 0, {});
            }
        }
    }

    void serve_collector()
    {
        while (!m_stop) {
            const int fd = ::accept(m_listener, nullptr, nullptr);
            if (fd < 0) {
                return;
            }
            std::string pending;
            std::array<char, 256> buffer{};
            while (!m_stop) {
                if (!readable(fd)) {
                    continue;
                }
                const ssize_t got = ::recv(fd, buffer.data(), buffer.size(), 0);
                if (got <= 0) {
                    break;
                }
                pending.append(buffer.data(), static_cast<std::size_t>(got));
                for (std::size_t end = pending.find('\n'); end != std::string::npos; end = pending.find('\n')) {
                    std::string line = pending.substr(0, end);
                    pending.erase(0, end + 1);
                    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                        line.pop_back();
                    }
                    if (line.starts_with("pass ") || line.starts_with("name ")) {
                        line.resize(4);
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_commands.push_back(line);
                    }
                    ::send(fd, "OK\n", 3, 0);
                }
            }
            ::close(fd);
        }
    }

    std::string m_root;
    int m_server = -1;
    int m_listener = -1;
    int m_port = 0;
    std::atomic<bool> m_stop{false};
    std::mutex m_mutex;
    std::vector<std::string> m_commands;
    std::vector<std::thread> m_threads;
};

/** Appends @p value to @p out, big-endian if @p big. */
template<typename T>
void append_value(std::vector<std::byte>& out, T value, bool big = false)
{
    std::array<std::byte, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));
    if (big) {
        std::reverse(bytes.begin(), bytes.end());
    }
    for (const std::byte b : bytes) { // insert() of the range trips GCC 14's -Wstringop-overflow at -O3
        out.push_back(b);
    }
}

/** Appends a FIFF tag whose data are words (or a channel record's words and name). */
void append_fiff_tag(std::vector<std::byte>& out, std::int32_t kind, std::int32_t type, const std::vector<std::byte>& data, bool big)
{
    for (const std::int32_t word : {kind, type, static_cast<std::int32_t>(data.size()), 0}) {
        append_value(out, word, big);
    }
    out.insert(out.end(), data.begin(), data.end());
}

std::vector<std::byte> fiff_words(std::initializer_list<std::int32_t> words, bool big)
{
    std::vector<std::byte> out;
    for (const std::int32_t word : words) {
        append_value(out, word, big);
    }
    return out;
}

/** A channel record (fiffChInfoRec) in little-endian byte order. */
std::vector<std::byte> fiff_channel(std::int32_t kind, std::int32_t coil, std::int32_t unit, float cal, const char* name)
{
    std::vector<std::byte> out;
    for (const std::int32_t word : {1, 1, kind}) {
        append_value(out, word);
    }
    append_value(out, 1.0F); // range
    append_value(out, cal);
    append_value(out, coil);
    for (int i = 0; i < 12; ++i) {
        append_value(out, i == 11 ? 1.0F : 0.0F);
    }
    append_value(out, unit);
    append_value(out, std::int32_t{0});
    std::array<char, 16> chars{};
    std::strncpy(chars.data(), name, chars.size() - 1);
    for (const char c : chars) {
        out.push_back(static_cast<std::byte>(c));
    }
    return out;
}

/**
 * A FieldTrip buffer server as neuromag2ft fills it: a header with the channel names, the Neuromag measurement
 * info (little-endian tags) and an isotrak file (big-endian FIFF) for two MEG channels and STI 014 at 600 Hz,
 * float32. After the stream started it holds 8 samples and two events (one with a string value), then it gets
 * a new header.
 */
class FakeFtBuffer
{
public:
    FakeFtBuffer()
    {
        m_listener = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in tcp{};
        tcp.sin_family = AF_INET;
        tcp.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t length = sizeof(tcp);
        if (m_listener < 0 || ::bind(m_listener, reinterpret_cast<sockaddr*>(&tcp), sizeof(tcp)) != 0 || ::listen(m_listener, 4) != 0 ||
            ::getsockname(m_listener, reinterpret_cast<sockaddr*>(&tcp), &length) != 0) {
            return;
        }
        m_port = ntohs(tcp.sin_port);
        m_thread = std::thread([this] { serve(); });
    }

    ~FakeFtBuffer()
    {
        m_stop = true;
        ::shutdown(m_listener, SHUT_RDWR);
        ::close(m_listener);
        m_thread.join();
    }

    FakeFtBuffer(const FakeFtBuffer&) = delete;
    FakeFtBuffer& operator=(const FakeFtBuffer&) = delete;

    [[nodiscard]] int port() const
    {
        return m_port;
    }

    [[nodiscard]] std::vector<std::uint16_t> commands()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_commands;
    }

private:
    static std::vector<std::byte> header()
    {
        std::vector<std::byte> meas; // the acquisition's tags in its (little-endian) byte order
        append_fiff_tag(meas, 100, 31, std::vector<std::byte>(20), false);
        append_fiff_tag(meas, 101, 3, fiff_words({-1}, false), false);
        append_fiff_tag(meas, 104, 3, fiff_words({100}, false), false);
        append_fiff_tag(meas, 104, 3, fiff_words({101}, false), false);
        append_fiff_tag(meas, 200, 3, fiff_words({3}, false), false);
        std::vector<std::byte> sfreq;
        append_value(sfreq, 600.0F);
        append_fiff_tag(meas, 201, 4, sfreq, false);
        append_fiff_tag(meas, 203, 30, fiff_channel(1, 3022, 112, 1e-13F, "MEG 0111"), false);
        append_fiff_tag(meas, 203, 30, fiff_channel(1, 3012, 201, 2e-13F, "MEG 0112"), false);
        append_fiff_tag(meas, 203, 30, fiff_channel(3, 0, -1, 1.0F, "STI 014"), false);
        append_fiff_tag(meas, 105, 3, fiff_words({101}, false), false);
        append_fiff_tag(meas, 104, 3, fiff_words({102}, false), false);

        std::vector<std::byte> isotrak; // a FIFF file with one cardinal point
        append_fiff_tag(isotrak, 100, 31, std::vector<std::byte>(20), true);
        append_fiff_tag(isotrak, 104, 3, fiff_words({107}, true), true);
        std::vector<std::byte> point = fiff_words({1, 2}, true);
        for (const float r : {0.0F, 0.1F, 0.0F}) {
            append_value(point, r, true);
        }
        append_fiff_tag(isotrak, 213, 33, point, true);
        append_fiff_tag(isotrak, 105, 3, fiff_words({107}, true), true);

        const std::string names("MEG 0111\0MEG 0112\0STI 014\0", 26);
        std::vector<std::byte> chunks;
        const auto chunk = [&](std::uint32_t type, std::span<const std::byte> data) {
            append_value(chunks, type);
            append_value(chunks, static_cast<std::uint32_t>(data.size()));
            chunks.insert(chunks.end(), data.begin(), data.end());
        };
        chunk(1, std::as_bytes(std::span(names)));
        chunk(8, meas);
        chunk(9, isotrak);

        std::vector<std::byte> out;
        for (const std::uint32_t word : {3U, 0U, 0U}) {
            append_value(out, word);
        }
        append_value(out, 600.0F);
        append_value(out, std::uint32_t{9}); // float32
        append_value(out, static_cast<std::uint32_t>(chunks.size()));
        out.insert(out.end(), chunks.begin(), chunks.end());
        return out;
    }

    static std::vector<std::byte> events()
    {
        std::vector<std::byte> out;
        const auto event = [&](std::uint32_t value_type, std::span<const std::byte> value, std::int32_t sample) {
            const std::string type = "trigger";
            for (const std::uint32_t word : {0U, static_cast<std::uint32_t>(type.size()), value_type, 1U}) {
                append_value(out, word);
            }
            for (const std::int32_t word : {sample, 0, 0}) {
                append_value(out, word);
            }
            append_value(out, static_cast<std::uint32_t>(type.size() + value.size()));
            const auto chars = std::as_bytes(std::span(type));
            out.insert(out.end(), chars.begin(), chars.end());
            out.insert(out.end(), value.begin(), value.end());
        };
        const std::int32_t code = 7;
        event(7, std::as_bytes(std::span(&code, 1)), 2);
        const std::string text = "start";
        event(0, std::as_bytes(std::span(text)), 4);
        return out;
    }

    static std::vector<std::byte> samples()
    {
        std::vector<std::byte> out;
        for (const std::uint32_t word : {3U, 8U, 9U, 8U * 3U * 4U}) {
            append_value(out, word);
        }
        for (int s = 0; s < 8; ++s) {
            for (const float v : {1e-12F * static_cast<float>(100 + s), -1e-11F * static_cast<float>(s), s >= 5 ? 3.0F : 0.0F}) {
                append_value(out, v);
            }
        }
        return out;
    }

    static bool read_all(int fd, std::byte* data, std::size_t size)
    {
        while (size > 0) {
            const ssize_t got = ::recv(fd, data, size, 0);
            if (got <= 0) {
                return false;
            }
            data += got;
            size -= static_cast<std::size_t>(got);
        }
        return true;
    }

    void answer(int fd, std::uint16_t command, const std::vector<std::byte>& payload)
    {
        std::vector<std::byte> out;
        append_value(out, std::uint16_t{1});
        append_value(out, command);
        append_value(out, static_cast<std::uint32_t>(payload.size()));
        out.insert(out.end(), payload.begin(), payload.end());
        static_cast<void>(::send(fd, out.data(), out.size(), 0));
    }

    void serve()
    {
        const int fd = ::accept(m_listener, nullptr, nullptr);
        if (fd < 0) {
            return;
        }
        int waits = 0;
        while (!m_stop) {
            std::array<std::byte, 8> def{};
            if (!read_all(fd, def.data(), def.size())) {
                break;
            }
            std::uint16_t command = 0;
            std::uint32_t size = 0;
            std::memcpy(&command, def.data() + 2, 2);
            std::memcpy(&size, def.data() + 4, 4);
            std::vector<std::byte> payload(size);
            if (!read_all(fd, payload.data(), size)) {
                break;
            }
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_commands.push_back(command);
            }
            if (command == 0x201) {
                answer(fd, 0x204, header());
            } else if (command == 0x402) {
                // The stream starts empty; then the samples and events arrive; then a new header resets the counts
                const std::uint32_t samples = waits == 0 ? 0 : waits == 1 ? 8
                                                                          : 0;
                const std::uint32_t count = waits == 0 ? 0 : waits == 1 ? 2
                                                                        : 0;
                ++waits;
                std::vector<std::byte> counts;
                append_value(counts, samples);
                append_value(counts, count);
                answer(fd, 0x404, counts);
            } else if (command == 0x203) {
                answer(fd, 0x204, events());
            } else if (command == 0x202) {
                answer(fd, 0x204, samples());
            } else {
                answer(fd, 0x205, {});
            }
        }
        ::close(fd);
    }

    int m_listener = -1;
    int m_port = 0;
    std::atomic<bool> m_stop{false};
    std::mutex m_mutex;
    std::vector<std::uint16_t> m_commands;
    std::thread m_thread;
};

/**
 * An mne_rt_server on two consecutive ports: it records the commands of the command port and the alias, gives
 * the data client id 5, answers "measinfo 5" with the measurement info of two MEG channels and STI 014 at 600 Hz,
 * and "start 5" with a buffer left over from an earlier start, the raw-data block with 8 calibrated samples in
 * two buffers, and the end of the block, as when the server stops.
 */
class FakeRtServer
{
public:
    FakeRtServer()
    {
        for (int attempt = 0; attempt < 20 && m_port == 0; ++attempt) {
            const int commands = listen_on(0);
            const int port = commands < 0 ? 0 : bound_port(commands);
            const int data = port > 0 && port < 65535 ? listen_on(port + 1) : -1;
            if (data < 0) {
                ::close(commands);
                continue;
            }
            m_listeners = {commands, data};
            m_port = port;
        }
        if (m_port > 0) {
            m_thread = std::thread([this] { serve(); });
        }
    }

    ~FakeRtServer()
    {
        m_stop = true;
        for (const int fd : m_listeners) {
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    FakeRtServer(const FakeRtServer&) = delete;
    FakeRtServer& operator=(const FakeRtServer&) = delete;

    [[nodiscard]] int port() const
    {
        return m_port;
    }

    [[nodiscard]] std::vector<std::string> commands()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_commands;
    }

    [[nodiscard]] std::string alias()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_alias;
    }

private:
    static int listen_on(int port)
    {
        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in tcp{};
        tcp.sin_family = AF_INET;
        tcp.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        tcp.sin_port = htons(static_cast<std::uint16_t>(port));
        if (fd < 0 || ::bind(fd, reinterpret_cast<sockaddr*>(&tcp), sizeof(tcp)) != 0 || ::listen(fd, 4) != 0) {
            if (fd >= 0) {
                ::close(fd);
            }
            return -1;
        }
        return fd;
    }

    static int bound_port(int fd)
    {
        sockaddr_in tcp{};
        socklen_t length = sizeof(tcp);
        return ::getsockname(fd, reinterpret_cast<sockaddr*>(&tcp), &length) == 0 ? ntohs(tcp.sin_port) : 0;
    }

    static bool read_all(int fd, std::byte* data, std::size_t size)
    {
        while (size > 0) {
            const ssize_t got = ::recv(fd, data, size, 0);
            if (got <= 0) {
                return false;
            }
            data += got;
            size -= static_cast<std::size_t>(got);
        }
        return true;
    }

    static std::uint32_t big(std::span<const std::byte> data, std::size_t offset)
    {
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4; ++i) {
            value = (value << 8) | std::to_integer<std::uint32_t>(data[offset + i]);
        }
        return value;
    }

    /** A channel record in big-endian byte order. */
    static std::vector<std::byte> big_channel(std::int32_t kind, std::int32_t coil, std::int32_t unit, float cal, const char* name)
    {
        std::vector<std::byte> out = fiff_channel(kind, coil, unit, cal, name);
        for (std::size_t word = 0; word < 20; ++word) {
            std::reverse(out.begin() + static_cast<std::ptrdiff_t>(4 * word), out.begin() + static_cast<std::ptrdiff_t>(4 * word + 4));
        }
        return out;
    }

    static std::vector<std::byte> meas_info()
    {
        std::vector<std::byte> out;
        append_fiff_tag(out, 104, 3, fiff_words({100}, true), true);
        append_fiff_tag(out, 104, 3, fiff_words({101}, true), true);
        append_fiff_tag(out, 200, 3, fiff_words({3}, true), true);
        std::vector<std::byte> sfreq;
        append_value(sfreq, 600.0F, true);
        append_fiff_tag(out, 201, 4, sfreq, true);
        append_fiff_tag(out, 203, 30, big_channel(1, 3022, 112, 1e-13F, "MEG 0111"), true);
        append_fiff_tag(out, 203, 30, big_channel(1, 3012, 201, 2e-13F, "MEG 0112"), true);
        append_fiff_tag(out, 203, 30, big_channel(3, 0, -1, 1.0F, "STI 014"), true);
        append_fiff_tag(out, 105, 3, fiff_words({101}, true), true);
        return out;
    }

    static std::vector<std::byte> buffer(int first, int count)
    {
        std::vector<std::byte> values;
        for (int s = first; s < first + count; ++s) {
            for (const float v : {1e-12F * static_cast<float>(100 + s), -1e-11F * static_cast<float>(s), s >= 5 ? 3.0F : 0.0F}) {
                append_value(values, v, true);
            }
        }
        std::vector<std::byte> out;
        append_fiff_tag(out, 300, 4, values, true);
        return out;
    }

    static void send_all(int fd, const std::vector<std::byte>& data)
    {
        static_cast<void>(::send(fd, data.data(), data.size(), 0));
    }

    void reply(int fd, const std::string& text)
    {
        std::vector<std::byte> out;
        append_value(out, static_cast<std::uint16_t>(4 + 2 * text.size()), true);
        append_value(out, static_cast<std::uint32_t>(2 * text.size()), true);
        for (const char c : text) {
            append_value(out, static_cast<std::uint16_t>(c), true);
        }
        send_all(fd, out);
    }

    /** Handles a command; @return false when the client left. */
    bool command(int commands, int data)
    {
        std::array<std::byte, 6> head{};
        if (!read_all(commands, head.data(), head.size())) {
            return false;
        }
        std::vector<std::byte> text(big(head, 2));
        if (!read_all(commands, text.data(), text.size())) {
            return false;
        }
        std::string line;
        for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
            line += static_cast<char>(std::to_integer<int>(text[i + 1]));
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_commands.push_back(line);
        }
        if (line == "measinfo 5") {
            send_all(data, meas_info());
        } else if (line == "start 5") {
            std::vector<std::byte> out = buffer(-4, 4); // still on its way from an earlier start
            append_fiff_tag(out, 104, 3, fiff_words({102}, true), true);
            const std::vector<std::byte> first = buffer(0, 5);
            const std::vector<std::byte> second = buffer(5, 3);
            out.insert(out.end(), first.begin(), first.end());
            out.insert(out.end(), second.begin(), second.end());
            append_fiff_tag(out, 105, 3, fiff_words({102}, true), true);
            send_all(data, out);
        }
        if (line.starts_with("simfile ")) {
            reply(commands, line.ends_with(".fif") ? "New simulation file set succefully.\r\n" : "Simulation file not set.\r\n");
        } else {
            reply(commands, "done " + line + "\r\n");
        }
        return true;
    }

    /** Handles a command tag; @return false when the client left. */
    bool data_command(int data)
    {
        std::array<std::byte, 16> head{};
        if (!read_all(data, head.data(), head.size())) {
            return false;
        }
        std::vector<std::byte> payload(big(head, 8));
        if (!read_all(data, payload.data(), payload.size())) {
            return false;
        }
        if (big(head, 0) != 3700 || payload.size() < 4) {
            return true;
        }
        if (big(payload, 0) == 1) {
            std::vector<std::byte> out;
            append_fiff_tag(out, 3701, 3, fiff_words({5}, true), true);
            send_all(data, out);
        } else if (big(payload, 0) == 2) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_alias.assign(reinterpret_cast<const char*>(payload.data()) + 4, payload.size() - 4);
        }
        return true;
    }

    void serve()
    {
        const int commands = ::accept(m_listeners[0], nullptr, nullptr);
        const int data = commands < 0 ? -1 : ::accept(m_listeners[1], nullptr, nullptr);
        while (!m_stop && data >= 0) {
            std::array<pollfd, 2> fds{{{commands, POLLIN, 0}, {data, POLLIN, 0}}};
            if (::poll(fds.data(), fds.size(), 50) < 0) {
                break;
            }
            if ((fds[0].revents != 0 && !command(commands, data)) || (fds[1].revents != 0 && !data_command(data))) {
                break;
            }
        }
        for (const int fd : {commands, data}) {
            if (fd >= 0) {
                ::close(fd);
            }
        }
    }

    std::array<int, 2> m_listeners{-1, -1};
    int m_port = 0;
    std::atomic<bool> m_stop{false};
    std::mutex m_mutex;
    std::vector<std::string> m_commands;
    std::string m_alias;
    std::thread m_thread;
};

/**
 * A BabyMEG server: its command port answers INFO with MEG001 (cal 1e-13, gain 0.5), the trigger line TRG001 and
 * DTRG01 at 600 Hz in packages of four samples; its data port answers the first two DATA requests with a package
 * each (MEG001 counting 1 to 8, TRG001 high at samples 2 to 4 and 7), and QUIT with QUIT. It records the commands.
 */
class FakeBabyMeg
{
public:
    FakeBabyMeg()
    {
        for (auto [fd, port] : {std::pair{&m_data, &m_data_port}, std::pair{&m_command, &m_command_port}}) {
            *fd = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in tcp{};
            tcp.sin_family = AF_INET;
            tcp.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            socklen_t length = sizeof(tcp);
            if (*fd >= 0 && ::bind(*fd, reinterpret_cast<sockaddr*>(&tcp), sizeof(tcp)) == 0 && ::listen(*fd, 2) == 0 &&
                ::getsockname(*fd, reinterpret_cast<sockaddr*>(&tcp), &length) == 0) {
                *port = ntohs(tcp.sin_port);
            }
        }
        m_threads.emplace_back([this] { serve_commands(); });
        m_threads.emplace_back([this] { serve_data(); });
    }

    ~FakeBabyMeg()
    {
        for (const int fd : {m_data, m_command}) {
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
        for (std::thread& thread : m_threads) {
            thread.join();
        }
    }

    FakeBabyMeg(const FakeBabyMeg&) = delete;
    FakeBabyMeg& operator=(const FakeBabyMeg&) = delete;

    [[nodiscard]] int data_port() const
    {
        return m_data_port;
    }

    [[nodiscard]] int command_port() const
    {
        return m_command_port;
    }

    [[nodiscard]] std::vector<std::string> commands()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_commands;
    }

private:
    static void send_message(int fd, std::string_view command, const std::vector<std::byte>& body)
    {
        std::vector<std::byte> out;
        const auto chars = std::as_bytes(std::span(command));
        out.insert(out.end(), chars.begin(), chars.end());
        append_value(out, static_cast<std::uint32_t>(body.size()), true);
        out.insert(out.end(), body.begin(), body.end());
        static_cast<void>(::send(fd, out.data(), out.size(), 0));
    }

    /** @return The next 4-character command, empty when the client left. */
    std::string command(int fd)
    {
        std::array<char, 4> text{};
        std::size_t got = 0;
        while (got < text.size()) {
            const ssize_t n = ::recv(fd, text.data() + got, text.size() - got, 0);
            if (n <= 0) {
                return {};
            }
            got += static_cast<std::size_t>(n);
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_commands.emplace_back(text.data(), text.size());
    }

    void serve_commands()
    {
        const int fd = ::accept(m_command, nullptr, nullptr);
        if (fd < 0) {
            return;
        }
        if (command(fd) == "INFO") {
            const std::string text = "INFO:3:4:600:MEG001|1,0.07,0,0,1,0,0,0,1,0,0,0,1,7002,1e-13,0.5;"
                                     "TRG001|1,0,0,0,0,0,0,0,0,0,0,0,0,3,1,1;DTRG01|1,0,0,0,0,0,0,0,0,0,0,0,0,3,1,1;";
            const auto chars = std::as_bytes(std::span(text));
            send_message(fd, "INFO", std::vector<std::byte>(chars.begin(), chars.end()));
        }
        ::close(fd);
    }

    void serve_data()
    {
        const int fd = ::accept(m_data, nullptr, nullptr);
        if (fd < 0) {
            return;
        }
        int packages = 0;
        while (true) {
            const std::string request = command(fd);
            if (request.empty() || request == "QREL") {
                break;
            }
            if (request == "QUIT") {
                send_message(fd, "QUIT", {});
            } else if (request == "DATA" && packages < 2) {
                std::vector<std::byte> body = {std::byte{'4'}};
                for (int i = 0; i < 4; ++i) {
                    const int s = 4 * packages + i;
                    for (const float v : {static_cast<float>(s + 1), s == 2 || s == 3 || s == 4 || s == 7 ? 5.0F : 0.0F, 0.0F}) {
                        append_value(body, v, true);
                    }
                }
                send_message(fd, "DATR", body);
                ++packages;
            }
        }
        ::close(fd);
    }

    int m_data = -1;
    int m_command = -1;
    int m_data_port = 0;
    int m_command_port = 0;
    std::mutex m_mutex;
    std::vector<std::string> m_commands;
    std::vector<std::thread> m_threads;
};

/**
 * A Fastrak at the far end of a pseudo-terminal: it records the commands it receives, answers the first "C"
 * with two updates of stations 1 and 2 (between them a station that is not used and lines that are no
 * records), then falls silent like a pressed stylus button, and answers the next "C" with one more update.
 */
class FakeFastrak
{
public:
    FakeFastrak()
    {
        m_master = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (m_master < 0 || ::grantpt(m_master) != 0 || ::unlockpt(m_master) != 0 || ::ptsname(m_master) == nullptr) {
            return;
        }
        m_port = ::ptsname(m_master);
        m_thread = std::thread([this] { serve(); });
    }

    ~FakeFastrak()
    {
        m_stop.store(true);
        if (m_thread.joinable()) {
            m_thread.join();
        }
        if (m_master >= 0) {
            ::close(m_master);
        }
    }

    FakeFastrak(const FakeFastrak&) = delete;
    FakeFastrak& operator=(const FakeFastrak&) = delete;

    [[nodiscard]] const std::string& port() const
    {
        return m_port;
    }

    [[nodiscard]] std::vector<std::string> commands()
    {
        std::lock_guard lock(m_mutex);
        return m_commands;
    }

private:
    void send(std::string_view text) const
    {
        for (std::size_t done = 0; done < text.size();) {
            const ssize_t n = ::write(m_master, text.data() + done, text.size() - done);
            if (n <= 0) {
                return; // the device under test then sees no answer and the test fails on that
            }
            done += static_cast<std::size_t>(n);
        }
    }

    void serve()
    {
        std::string pending;
        int starts = 0;
        while (!m_stop.load()) {
            pollfd p{m_master, POLLIN, 0};
            if (::poll(&p, 1, 20) <= 0 || (p.revents & POLLIN) == 0) {
                std::this_thread::sleep_for(5ms); // no reader on the terminal yet
                continue;
            }
            std::array<char, 256> chunk{};
            const ssize_t got = ::read(m_master, chunk.data(), chunk.size());
            if (got <= 0) {
                continue;
            }
            pending.append(chunk.data(), static_cast<std::size_t>(got));
            for (std::size_t end = pending.find('\r'); end != std::string::npos; end = pending.find('\r')) {
                const std::string command = pending.substr(0, end);
                pending.erase(0, end + 1);
                {
                    std::lock_guard lock(m_mutex);
                    m_commands.push_back(command);
                }
                if (command != "C") {
                    continue;
                }
                if (++starts == 1) {
                    // Inches; station 1 with Euler angles, station 2 with a quaternion and fields run together
                    send("01   1.0000   2.0000  -3.0000   90.000    0.000    0.000\r\n"
                         "\r\n"
                         "03   9.0000   9.0000   9.0000    0.000    0.000    0.000\r\n"
                         "02 0.8829-0.3055-1.0000 0 0 0 2\r\n"
                         "bad record\r\n"
                         "01   0.0000   0.0000   1.0000\r\n"
                         "02   0.0000   1.0000   0.0000   0.000   0.000   0.000\r\n");
                } else {
                    send("01   2.0000   0.0000   0.0000    0.000    0.000    0.000\r\n"
                         "02   0.0000   2.0000   0.0000    0.000    0.000    0.000\r\n");
                }
            }
        }
    }

    int m_master = -1;
    std::string m_port;
    std::atomic<bool> m_stop{false};
    std::mutex m_mutex;
    std::vector<std::string> m_commands;
    std::thread m_thread;
};
#endif

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestAcqDevices
 *
 * @brief Vendor device sources against their simulators.
 */
class TestAcqDevices : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBrainAmpScheme();
    void testBrainAmpStreamLayout();
    void testBrainAmpMatchesSynthetic();
    void testBrainAmpWaitsForGatingReader();
    void testBrainAmpResolution();
    void testBrainAmpClipsAtFullScale();
    void testBrainAmpNeedsAmplifiers();
    void testBrainAmpRealtimePacing();
    void testBrainAmpInvalidParameters_data();
    void testBrainAmpInvalidParameters();
    void testBrainAmpHardwareUnavailable();
    void testTmsiStreamLayout();
    void testTmsiMatchesSynthetic();
    void testTmsiRate_data();
    void testTmsiRate();
    void testTmsiChannelSubset();
    void testTmsiImpedances();
    void testTmsiWaitsForGatingReader();
    void testTmsiInvalidParameters_data();
    void testTmsiInvalidParameters();
    void testTmsiHardwareUnavailable();
    void testGusbampStreamLayout();
    void testGusbampChainsUnits();
    void testGusbampMatchesSynthetic();
    void testGusbampImpedances();
    void testGusbampWaitsForGatingReader();
    void testGusbampInvalidParameters_data();
    void testGusbampInvalidParameters();
    void testGusbampHardwareUnavailable();
    void testEegoStreamLayout();
    void testEegoMatchesSynthetic();
    void testEegoImpedances();
    void testEegoWaitsForGatingReader();
    void testEegoInvalidParameters_data();
    void testEegoInvalidParameters();
    void testEegoHardwareUnavailable();
    void testEegoDriverReadsTheSdk();
    void testNeuromagStreamLayout();
    void testNeuromagMatchesSynthetic();
    void testNeuromagWaitsForGatingReader();
    void testNeuromagInvalidParameters_data();
    void testNeuromagInvalidParameters();
    void testNeuromagHardwareUnavailable();
    void testNeuromagReadsTheDataServer();
    void testPolhemusStreamLayout();
    void testPolhemusSimulatorSweepsTheStylus();
    void testPolhemusButtonPressIsAnEvent();
    void testPolhemusInvalidParameters_data();
    void testPolhemusInvalidParameters();
    void testPolhemusHardwareUnavailable();
    void testPolhemusReadsTheSerialPort();
    void testFtBufferStreamLayout();
    void testFtBufferMatchesNeuromag();
    void testFtBufferWaitsForGatingReader();
    void testFtBufferInvalidParameters_data();
    void testFtBufferInvalidParameters();
    void testFtBufferServerUnavailable();
    void testFtBufferReadsTheServer();
    void testRtServerStreamLayout();
    void testRtServerMatchesNeuromag();
    void testRtServerRestarts();
    void testRtServerInvalidParameters_data();
    void testRtServerInvalidParameters();
    void testRtServerServerUnavailable();
    void testRtServerReadsTheServer();
    void testNatusStreamLayout();
    void testNatusMatchesSynthetic();
    void testNatusWaitsForGatingReader();
    void testNatusInvalidParameters_data();
    void testNatusInvalidParameters();
    void testNatusReceivesDatagrams();
    void testBabyMegStreamLayout();
    void testBabyMegMatchesSynthetic();
    void testBabyMegInvalidParameters_data();
    void testBabyMegInvalidParameters();
    void testBabyMegServerUnavailable();
    void testBabyMegReadsTheServer();
    void testBrainFlowStreamLayout();
    void testBrainFlowReadsTheBoard();
    void testBrainFlowPassesInputParams();
    void testBrainFlowInvalidParameters_data();
    void testBrainFlowInvalidParameters();
    void testBrainFlowLibraryUnavailable();
    void testBrainFlowRefusesUnsupportedVersion();
    void testBrainFlowSyntheticBoard();
    void testStreamingBoardDecodesDatagrams();
    void testStreamingBoardSimulates();
    void testStreamingBoardInvalidParameters_data();
    void testStreamingBoardInvalidParameters();
    void testBrainFlowFileReplays();
    void testBrainFlowFileRefusesMalformedFiles_data();
    void testBrainFlowFileRefusesMalformedFiles();
    void testBrainFlowFileMatchesBrainFlow();
    void testStreamingBoardMatchesBrainFlow_data();
    void testStreamingBoardMatchesBrainFlow();
};

//=============================================================================================================

void TestAcqDevices::initTestCase()
{
    QVERIFY(registerBrainAmpScheme().ok());
    QVERIFY(registerTmsiScheme().ok());
    QVERIFY(register_gusbamp_scheme().ok());
    QVERIFY(register_eego_scheme().ok());
    QVERIFY(register_neuromag_scheme().ok());
    QVERIFY(register_polhemus_scheme().ok());
    QVERIFY(register_ftbuffer_scheme().ok());
    QVERIFY(register_rtserver_scheme().ok());
    QVERIFY(register_natus_scheme().ok());
    QVERIFY(register_babymeg_scheme().ok());
    QVERIFY(register_brainflow_scheme().ok());
    QVERIFY(register_streaming_board_scheme().ok());
    QVERIFY(register_brainflow_file_scheme().ok());
    QVERIFY(register_device_schemes().ok());
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpScheme()
{
    const auto schemes = AcqDeviceRegistry::instance().schemes();
    const auto it = std::find_if(schemes.begin(), schemes.end(), [](const AcqSchemeInfo& s) { return s.scheme == "brainamp"; });
    QVERIFY(it != schemes.end());
    QCOMPARE(it->exampleUri, std::string("brainamp://?simulate=1"));
    for (const char* name : {"simulate", "channels", "block", "resolution", "lowpass", "dc", "seed", "amplifiers", "realtime", "duration"}) {
        QVERIFY2(std::any_of(it->parameters.begin(), it->parameters.end(), [&](const AcqParameterInfo& p) { return p.name == name; }), name);
    }
    // Registering twice reports the first result
    QVERIFY(registerBrainAmpScheme().ok());
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpStreamLayout()
{
    auto device = AcqDevice::open("brainamp://?simulate=1&channels=40");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("brainamp"));
    QVERIFY(device->description().device.find("simulated") != std::string::npos);

    // The amplifier's wire format: 16-bit channels followed by the marker word, 5 kHz
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Int16);
    QCOMPARE(info->layout, AcqLayout::Interleaved);
    QCOMPARE(info->sfreq, 5000.0);
    QCOMPARE(info->channelCount(), std::size_t(41));
    QCOMPARE(info->channels.front().name, std::string("EEG 001"));
    QCOMPARE(info->channels[39].name, std::string("EEG 040"));
    QCOMPARE(info->channels.front().kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels.front().unit, AcqUnit::Volt);
    QCOMPARE(info->channels.front().scale, 100e-9);
    QCOMPARE(info->channels.back().name, std::string("STI 014"));
    QCOMPARE(info->channels.back().kind, AcqChannelKind::Stim);
    QCOMPARE(info->channels.back().scale, 1.0);
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpMatchesSynthetic()
{
    // The simulated amplifier digitises the synthetic session of the same seed
    const Recording amp = record("brainamp://?simulate=1&channels=8&seed=3&realtime=0&duration=2&block=100");
    const Recording ref = record("synthetic://?eeg=8&sfreq=5000&seed=3&realtime=0&duration=2&block=100");
    QCOMPARE(amp.channels, std::size_t(9));
    QCOMPARE(ref.channels, std::size_t(9));
    QCOMPARE(amp.frames(), std::size_t(10000));
    QCOMPARE(ref.frames(), amp.frames());

    const double step = 100e-9;
    double worst = 0.0;
    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 8; ++c) {
            worst = std::max(worst, std::abs(amp.at(s, c) - ref.at(s, c)));
        }
        QCOMPARE(amp.at(s, 8), ref.at(s, 8));
    }
    QVERIFY2(worst <= 0.5 * step + 1e-12, qPrintable(QString::number(worst)));

    // Marker onsets become events, as the trigger codes of the session
    QVERIFY(ref.events.size() >= 1);
    QCOMPARE(amp.events.size(), ref.events.size());
    for (std::size_t e = 0; e < ref.events.size(); ++e) {
        QCOMPARE(amp.events[e].sample, ref.events[e].sample);
        QCOMPARE(amp.events[e].code, ref.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpWaitsForGatingReader()
{
    // Four seconds are more than the ring holds; the replay waits while the reader is busy
    auto device = AcqDevice::open("brainamp://?simulate=1&channels=4&realtime=0&duration=4");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(20000));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpResolution()
{
    const Recording amp = record("brainamp://?simulate=1&channels=2&seed=1&realtime=0&duration=0.2&resolution=10uV");
    QCOMPARE(amp.scale.front(), 10e-6);
    QVERIFY(amp.frames() > 0);
    bool nonZero = false;
    for (std::size_t s = 0; s < amp.frames(); ++s) {
        const double steps = amp.at(s, 0) / 10e-6;
        QVERIFY(std::abs(steps - std::round(steps)) < 1e-6);
        nonZero |= amp.at(s, 0) != 0.0;
    }
    QVERIFY(nonZero);
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpClipsAtFullScale()
{
    // 100 nV steps cover +-3.28 mV; a session 1000 times stronger saturates instead of wrapping around
    const Recording amp = record("brainamp://?simulate=1&channels=4&seed=2&realtime=0&duration=0.5&gain=1000");
    QVERIFY(amp.frames() > 0);
    double low = 0.0;
    double high = 0.0;
    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 4; ++c) {
            low = std::min(low, amp.at(s, c));
            high = std::max(high, amp.at(s, c));
        }
    }
    QCOMPARE(low, -32768 * 100e-9);
    QCOMPARE(high, 32767 * 100e-9);
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpNeedsAmplifiers()
{
    // 64 channels need two 32-channel amplifiers
    auto device = AcqDevice::open("brainamp://?simulate=1&channels=64&amplifiers=1");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("2 amplifiers") != std::string::npos);
    QVERIFY(AcqDevice::open("brainamp://?simulate=1&channels=64&amplifiers=2"));
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpRealtimePacing()
{
    auto device = AcqDevice::open("brainamp://?simulate=1&channels=4&block=50");
    QVERIFY(device);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    const auto begin = std::chrono::steady_clock::now();
    std::size_t samples = 0;
    while (samples < 1000) {
        auto view = reader->acquire(1, 5s);
        QVERIFY(view);
        QCOMPARE(view->sampleCount() % 50, std::size_t(0)); // whole driver blocks
        samples += view->sampleCount();
        static_cast<void>(reader->release(*view));
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    QVERIFY(device->stop().ok());
    QVERIFY2(seconds >= 0.15, qPrintable(QString::number(seconds))); // 1000 samples at 5 kHz take 0.2 s
    QCOMPARE(reader->stats().lost, std::uint64_t(0));
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("no channels") << "brainamp://?simulate=1&channels=0";
    QTest::newRow("too many channels") << "brainamp://?simulate=1&channels=129";
    QTest::newRow("resolution") << "brainamp://?simulate=1&resolution=1mV";
    QTest::newRow("lowpass") << "brainamp://?simulate=1&lowpass=500";
    QTest::newRow("block") << "brainamp://?simulate=1&block=0";
    QTest::newRow("amplifiers") << "brainamp://?simulate=1&amplifiers=5";
    QTest::newRow("target") << "brainamp://usb?simulate=1";
}

void TestAcqDevices::testBrainAmpInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testBrainAmpHardwareUnavailable()
{
#if defined(_WIN32) && defined(MNE_ACQ_BRAINAMP_HARDWARE)
    QSKIP("built with the BrainAmp driver interface");
#else
    auto device = AcqDevice::open("brainamp://");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::BackendUnavailable);
    QVERIFY(device.error().text().find("simulate=1") != std::string::npos);
#endif
}

//=============================================================================================================

void TestAcqDevices::testTmsiStreamLayout()
{
    auto device = AcqDevice::open("tmsi://?simulate=1&exg=8");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("tmsi"));
    QVERIFY(device->description().device.find("simulated Refa") != std::string::npos);

    // The SDK's format: an int32 per channel; ExG, bipolar and auxiliary inputs, digital inputs, test ramp
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Int32);
    QCOMPARE(info->layout, AcqLayout::Interleaved);
    QCOMPARE(info->sfreq, 2048.0);
    QCOMPARE(info->channelCount(), std::size_t(18));
    const std::vector<std::pair<std::size_t, std::string>> names = {{0, "EEG 001"}, {7, "EEG 008"}, {8, "BIP 1"}, {11, "BIP 4"}, {12, "AUX 1"}, {15, "AUX 4"}, {16, "STI 014"}, {17, "SAW"}};
    for (const auto& [index, name] : names) {
        QCOMPARE(info->channels[index].name, name);
    }
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[0].scale, 0.0715256 * 1e-6);
    // Gain, offset and exponent: (raw * 1 + 100) * 1e-6 V
    QCOMPARE(info->channels[12].scale, 1e-6);
    QCOMPARE(info->channels[12].offset, 100e-6);
    QCOMPARE(info->channels[16].kind, AcqChannelKind::Stim);
    QCOMPARE(info->channels[16].unit, AcqUnit::None);
    QCOMPARE(info->channels[16].scale, 1.0);
    QCOMPARE(info->channels[17].kind, AcqChannelKind::Misc);
}

//=============================================================================================================

void TestAcqDevices::testTmsiMatchesSynthetic()
{
    // The simulated amplifier digitises the synthetic session of the same seed
    const Recording amp = record("tmsi://?simulate=1&exg=8&rate=1024&seed=3&realtime=0&duration=2");
    const Recording ref = record("synthetic://?eeg=16&sfreq=1024&seed=3&realtime=0&duration=2&block=8");
    QCOMPARE(amp.channels, std::size_t(18));
    QCOMPARE(ref.channels, std::size_t(17));
    QCOMPARE(amp.frames(), std::size_t(2048));
    QCOMPARE(ref.frames(), amp.frames());

    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 16; ++c) {
            QVERIFY2(std::abs(amp.at(s, c) - ref.at(s, c)) <= 0.5 * amp.scale[c] + 1e-12, qPrintable(QString::number(c)));
        }
        QCOMPARE(amp.at(s, 16), ref.at(s, 16));
        QCOMPARE(amp.at(s, 17), static_cast<double>(s)); // the test ramp counts samples
    }
    QVERIFY(ref.events.size() >= 1);
    QCOMPARE(amp.events.size(), ref.events.size());
    for (std::size_t e = 0; e < ref.events.size(); ++e) {
        QCOMPARE(amp.events[e].sample, ref.events[e].sample);
        QCOMPARE(amp.events[e].code, ref.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testTmsiRate_data()
{
    QTest::addColumn<double>("requested");
    QTest::addColumn<double>("rate");
    QTest::newRow("base") << 2048.0 << 2048.0;
    QTest::newRow("nearest above") << 1000.0 << 1024.0;
    QTest::newRow("nearest below") << 300.0 << 256.0;
    QTest::newRow("above the base") << 5000.0 << 2048.0;
    QTest::newRow("below the lowest") << 10.0 << 128.0;
}

void TestAcqDevices::testTmsiRate()
{
    // The amplifier divides its base rate by a power of two; the stream reports the rate it runs at
    QFETCH(double, requested);
    QFETCH(double, rate);
    auto device = AcqDevice::open(std::format("tmsi://?simulate=1&exg=4&rate={}", requested));
    QVERIFY(device);
    QCOMPARE(device->stream().info()->sfreq, rate);
}

//=============================================================================================================

void TestAcqDevices::testTmsiChannelSubset()
{
    const Recording amp = record("tmsi://?simulate=1&exg=8&channels=3&realtime=0&duration=0.5&seed=3");
    QCOMPARE(amp.channels, std::size_t(3));
    QCOMPARE(amp.frames(), std::size_t(1024));
    QVERIFY(amp.events.empty()); // the digital inputs are not delivered

    auto tooMany = AcqDevice::open("tmsi://?simulate=1&exg=8&channels=19");
    QVERIFY(!tooMany);
    QCOMPARE(tooMany.error().code(), AcqErrorCode::InvalidParameter);
    QVERIFY(tooMany.error().text().find("has 18") != std::string::npos);
}

//=============================================================================================================

void TestAcqDevices::testTmsiImpedances()
{
    auto device = AcqDevice::open("tmsi://?simulate=1&exg=8&seed=5");
    QVERIFY(device);
    auto impedances = device->measureImpedances();
    QVERIFY(impedances);
    QCOMPARE(impedances->size(), std::size_t(8));
    for (std::size_t i = 0; i < impedances->size(); ++i) {
        QCOMPARE((*impedances)[i].stream, std::size_t(0));
        QCOMPARE((*impedances)[i].channel, i);
        QVERIFY((*impedances)[i].ohms >= 1e3 && (*impedances)[i].ohms <= 20e3);
    }

    // Only the delivered ExG channels, and not while streaming
    auto subset = AcqDevice::open("tmsi://?simulate=1&exg=8&seed=5&channels=3");
    QVERIFY(subset);
    auto three = subset->measureImpedances();
    QVERIFY(three);
    QCOMPARE(three->size(), std::size_t(3));
    QCOMPARE(three->back().ohms, (*impedances)[2].ohms);
    QVERIFY(subset->start().ok());
    QCOMPARE(subset->measureImpedances().error().code(), AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqDevices::testTmsiWaitsForGatingReader()
{
    // Four seconds are more than the ring holds; the replay waits while the reader is busy
    auto device = AcqDevice::open("tmsi://?simulate=1&exg=4&realtime=0&duration=4");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(8192));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testTmsiInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("rate") << "tmsi://?simulate=1&rate=0";
    QTest::newRow("block") << "tmsi://?simulate=1&block=0";
    QTest::newRow("exg") << "tmsi://?simulate=1&exg=0";
    QTest::newRow("average") << "tmsi://?simulate=1&average=maybe";
    QTest::newRow("target") << "tmsi://usb?simulate=1";
}

void TestAcqDevices::testTmsiInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testTmsiHardwareUnavailable()
{
#if defined(_WIN32)
    QSKIP("Windows looks for the TMSi SDK");
#else
    auto device = AcqDevice::open("tmsi://");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::BackendUnavailable);
    QVERIFY(device.error().text().find("simulate=1") != std::string::npos);
#endif
}

//=============================================================================================================

void TestAcqDevices::testGusbampStreamLayout()
{
    auto device = AcqDevice::open("gusbamp://?simulate=1&inputs=1-4&trigger=1");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("gusbamp"));
    QVERIFY(device->description().device.find("simulated g.USBamp") != std::string::npos);

    // The API's format: a float per channel in microvolts, the master's trigger line last
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->layout, AcqLayout::Interleaved);
    QCOMPARE(info->sfreq, 1200.0);
    QCOMPARE(info->channelCount(), std::size_t(5));
    QCOMPARE(info->channels[0].name, std::string("EEG 001"));
    QCOMPARE(info->channels[3].name, std::string("EEG 004"));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[0].scale, 1e-6);
    QCOMPARE(info->channels[4].name, std::string("STI 014"));
    QCOMPARE(info->channels[4].kind, AcqChannelKind::Stim);
    QCOMPARE(info->channels[4].scale, 1.0);
}

//=============================================================================================================

void TestAcqDevices::testGusbampChainsUnits()
{
    // The inputs of the master come first, then those of each slave
    auto device = AcqDevice::open("gusbamp://UB-1,UB-2,UB-3?simulate=1&inputs=2,5");
    QVERIFY(device);
    QVERIFY(device->description().device.find("UB-1 (master), UB-2, UB-3") != std::string::npos);
    const auto info = device->stream().info();
    QCOMPARE(info->channelCount(), std::size_t(6));
    QCOMPARE(info->channels.back().name, std::string("EEG 006"));
}

//=============================================================================================================

void TestAcqDevices::testGusbampMatchesSynthetic()
{
    // Two simulated units digitise the synthetic session of the same seed; the transfer headers are skipped
    const Recording amp = record("gusbamp://UB-1,UB-2?simulate=1&inputs=1-4&trigger=1&rate=256&seed=3&realtime=0&duration=2");
    const Recording ref = record("synthetic://?eeg=8&sfreq=256&seed=3&realtime=0&duration=2&block=16");
    QCOMPARE(amp.channels, std::size_t(9));
    QCOMPARE(ref.channels, std::size_t(9));
    QCOMPARE(amp.frames(), std::size_t(512));
    QCOMPARE(ref.frames(), amp.frames());

    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 9; ++c) {
            QVERIFY2(std::abs(amp.at(s, c) - ref.at(s, c)) <= 1e-6 * std::abs(ref.at(s, c)) + 1e-15, qPrintable(QString::number(c)));
        }
    }
    QVERIFY(ref.events.size() >= 1);
    QCOMPARE(amp.events.size(), ref.events.size());
    for (std::size_t e = 0; e < ref.events.size(); ++e) {
        QCOMPARE(amp.events[e].sample, ref.events[e].sample);
        QCOMPARE(amp.events[e].code, ref.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testGusbampImpedances()
{
    auto device = AcqDevice::open("gusbamp://UB-1,UB-2?simulate=1&seed=5");
    QVERIFY(device);
    auto impedances = device->measureImpedances();
    QVERIFY(impedances);
    QCOMPARE(impedances->size(), std::size_t(32));
    for (std::size_t i = 0; i < impedances->size(); ++i) {
        QCOMPARE((*impedances)[i].channel, i);
        QVERIFY((*impedances)[i].ohms >= 1e3 && (*impedances)[i].ohms <= 20e3);
    }

    // Each unit measures the set-up inputs, and not while streaming
    auto subset = AcqDevice::open("gusbamp://UB-1,UB-2?simulate=1&seed=5&inputs=3");
    QVERIFY(subset);
    auto two = subset->measureImpedances();
    QVERIFY(two);
    QCOMPARE(two->size(), std::size_t(2));
    QCOMPARE((*two)[0].ohms, (*impedances)[2].ohms);
    QCOMPARE((*two)[1].ohms, (*impedances)[18].ohms);
    QVERIFY(subset->start().ok());
    QCOMPARE(subset->measureImpedances().error().code(), AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqDevices::testGusbampWaitsForGatingReader()
{
    // Four seconds are more than the ring holds; the replay waits while the reader is busy
    auto device = AcqDevice::open("gusbamp://?simulate=1&inputs=1-4&rate=256&realtime=0&duration=4");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(1024));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testGusbampInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("rate") << "gusbamp://?simulate=1&rate=1000";
    QTest::newRow("input 0") << "gusbamp://?simulate=1&inputs=0";
    QTest::newRow("input 17") << "gusbamp://?simulate=1&inputs=1-17";
    QTest::newRow("descending range") << "gusbamp://?simulate=1&inputs=5-3";
    QTest::newRow("not a number") << "gusbamp://?simulate=1&inputs=x";
    QTest::newRow("empty inputs") << "gusbamp://?simulate=1&inputs=";
    QTest::newRow("trigger") << "gusbamp://?simulate=1&trigger=maybe";
    QTest::newRow("five units") << "gusbamp://A,B,C,D,E?simulate=1";
    QTest::newRow("twice the same unit") << "gusbamp://A,B,A?simulate=1";
    QTest::newRow("empty serial") << "gusbamp://A,,B?simulate=1";
}

void TestAcqDevices::testGusbampInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testGusbampHardwareUnavailable()
{
#if defined(_WIN32)
    QSKIP("Windows looks for the g.USBamp API");
#else
    auto device = AcqDevice::open("gusbamp://");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::BackendUnavailable);
    QVERIFY(device.error().text().find("simulate=1") != std::string::npos);
#endif
}

//=============================================================================================================

void TestAcqDevices::testEegoStreamLayout()
{
    auto device = AcqDevice::open("eego://?simulate=1&reference=8&bipolar=2");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("eego"));
    QVERIFY(device->description().device.find("simulated EE") != std::string::npos);

    // The SDK's format: doubles in volts; referential and bipolar inputs, trigger word, sample counter
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float64);
    QCOMPARE(info->layout, AcqLayout::Interleaved);
    QCOMPARE(info->sfreq, 512.0);
    QCOMPARE(info->channelCount(), std::size_t(12));
    const std::vector<std::pair<std::size_t, std::string>> names = {{0, "EEG 001"}, {7, "EEG 008"}, {8, "BIP 1"}, {9, "BIP 2"}, {10, "STI 014"}, {11, "COUNTER"}};
    for (const auto& [index, name] : names) {
        QCOMPARE(info->channels[index].name, name);
    }
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[0].scale, 1.0);
    QCOMPARE(info->channels[8].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[10].kind, AcqChannelKind::Stim);
    QCOMPARE(info->channels[10].unit, AcqUnit::None);
    QCOMPARE(info->channels[11].kind, AcqChannelKind::Misc);
    const std::vector<std::pair<std::string, std::string>> ranges = {{"reference_range", "1 V"}, {"bipolar_range", "4 V"}};
    QVERIFY(info->properties == ranges);
}

//=============================================================================================================

void TestAcqDevices::testEegoMatchesSynthetic()
{
    // The simulated amplifier digitises the synthetic session of the same seed
    const Recording amp = record("eego://?simulate=1&reference=8&bipolar=2&rate=500&seed=3&realtime=0&duration=2");
    const Recording ref = record("synthetic://?eeg=10&sfreq=500&seed=3&realtime=0&duration=2&block=5");
    QCOMPARE(amp.channels, std::size_t(12));
    QCOMPARE(ref.channels, std::size_t(11));
    QCOMPARE(amp.frames(), std::size_t(1000));
    QCOMPARE(ref.frames(), amp.frames());

    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 11; ++c) {
            QCOMPARE(amp.at(s, c), ref.at(s, c));
        }
        QCOMPARE(amp.at(s, 11), static_cast<double>(s)); // the sample counter
    }
    QVERIFY(ref.events.size() >= 1);
    QCOMPARE(amp.events.size(), ref.events.size());
    for (std::size_t e = 0; e < ref.events.size(); ++e) {
        QCOMPARE(amp.events[e].sample, ref.events[e].sample);
        QCOMPARE(amp.events[e].code, ref.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testEegoImpedances()
{
    auto device = AcqDevice::open("eego://?simulate=1&reference=8&bipolar=2&seed=5");
    QVERIFY(device);
    auto impedances = device->measureImpedances();
    QVERIFY(impedances);
    QCOMPARE(impedances->size(), std::size_t(8));
    for (std::size_t i = 0; i < impedances->size(); ++i) {
        QCOMPARE((*impedances)[i].channel, i);
        QVERIFY((*impedances)[i].ohms >= 1e3 && (*impedances)[i].ohms <= 20e3);
    }
    QVERIFY(device->start().ok());
    QCOMPARE(device->measureImpedances().error().code(), AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqDevices::testEegoWaitsForGatingReader()
{
    // Four seconds are more than the ring holds; the replay waits while the reader is busy
    auto device = AcqDevice::open("eego://?simulate=1&reference=4&bipolar=0&rate=1000&realtime=0&duration=4");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(4000));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testEegoInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("rate") << "eego://?simulate=1&rate=600";
    QTest::newRow("reference range") << "eego://?simulate=1&reference_range=2";
    QTest::newRow("bipolar range") << "eego://?simulate=1&bipolar_range=9";
    QTest::newRow("no inputs") << "eego://?simulate=1&reference=0";
    QTest::newRow("realtime") << "eego://?simulate=1&realtime=maybe";
}

void TestAcqDevices::testEegoInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testEegoHardwareUnavailable()
{
    auto device = AcqDevice::open("eego://?sdk=/no/such/dir/eego-SDK");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::BackendUnavailable);
    QVERIFY(device.error().text().find("simulate=1") != std::string::npos);
}

//=============================================================================================================

void TestAcqDevices::testEegoDriverReadsTheSdk()
{
    // The driver against a stand-in SDK library with known samples
    const std::string sdk = "eego://?sdk=" EEGO_SDK_STUB;
    auto device = AcqDevice::open(sdk + "&rate=1000&reference_range=0.75");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().device, std::string("ANT Neuro EE-STUB STUB-1"));
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->channelCount(), std::size_t(7));
    QCOMPARE(info->channels[4].name, std::string("BIP 1"));
    QCOMPARE(info->channels[5].name, std::string("STI 014"));

    auto impedances = device->measureImpedances();
    QVERIFY(impedances);
    QCOMPARE(impedances->size(), std::size_t(4));
    QCOMPARE((*impedances)[3].ohms, 4e3);

    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::vector<double> frames;
    std::vector<AcqEvent> events;
    while (frames.size() < 7 * 100) {
        auto view = reader->acquire(1, 5s);
        QVERIFY(view);
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < 7; ++c) {
                    double v = 0.0;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    frames.push_back(v);
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    QVERIFY(device->stop().ok());
    for (std::size_t s = 0; s < 100; ++s) {
        QCOMPARE(frames[s * 7 + 0], 1e-6 + static_cast<double>(s) * 1e-9);
        QCOMPARE(frames[s * 7 + 4], 5e-6 + static_cast<double>(s) * 1e-9);
        QCOMPARE(frames[s * 7 + 6], static_cast<double>(s));
    }
    QVERIFY(!events.empty());
    QCOMPARE(events.front().sample, std::uint64_t(20));
    QCOMPARE(events.front().code, 3);

    // The SDK's own refusals
    QCOMPARE(openError(sdk + "&rate=512"), AcqErrorCode::InvalidParameter);
    QCOMPARE(openError("eego://OTHER-2?sdk=" EEGO_SDK_STUB), AcqErrorCode::DeviceNotFound);
}

//=============================================================================================================

void TestAcqDevices::testNeuromagStreamLayout()
{
    auto device = AcqDevice::open("neuromag://?simulate=1&meg=4&eeg=2");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("neuromag"));
    QVERIFY(device->description().device.find("simulated Vectorview") != std::string::npos);

    // The acquisition's format: an int32 per channel, MEG, EEG, then STI 014, as the measurement info describes them
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Int32);
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->channelCount(), std::size_t(7));
    QCOMPARE(info->channels[0].name, std::string("MEG 0001"));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Meg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[0].scale, static_cast<double>(1e-15F));
    QVERIFY(info->channels[0].fiff);
    QCOMPARE(info->channels[0].fiff->coilType, 3024);
    QCOMPARE(info->channels[0].fiff->coordFrame, 1); // device
    QCOMPARE(info->channels[0].fiff->scannerNo, 1);
    QCOMPARE(info->channels[4].name, std::string("EEG 001"));
    QCOMPARE(info->channels[4].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[4].fiff->coordFrame, 4); // head
    QCOMPARE(info->channels[6].name, std::string("STI 014"));
    QCOMPARE(info->channels[6].kind, AcqChannelKind::Stim);
    // The whole measurement info travels with the stream, for the FIFF bridge
    QCOMPARE(info->extensions.size(), std::size_t(1));
    QCOMPARE(info->extensions[0].schema, std::string("fiff.MeasInfo/1"));
    QVERIFY(!info->extensions[0].data.empty());
}

//=============================================================================================================

void TestAcqDevices::testNeuromagMatchesSynthetic()
{
    // The simulated acquisition digitises the synthetic session of the same seed, MEG first
    const Recording amp = record("neuromag://?simulate=1&meg=4&eeg=2&rate=500&seed=3&realtime=0&duration=2");
    const Recording ref = record("synthetic://?meg=4&eeg=2&sfreq=500&seed=3&realtime=0&duration=2&block=100");
    QCOMPARE(amp.channels, std::size_t(7));
    QCOMPARE(ref.channels, std::size_t(7));
    QCOMPARE(amp.frames(), std::size_t(1000));
    QCOMPARE(ref.frames(), amp.frames());

    const std::array<std::size_t, 7> source = {2, 3, 4, 5, 0, 1, 6};
    for (std::size_t s = 0; s < amp.frames(); ++s) {
        for (std::size_t c = 0; c < 7; ++c) {
            QVERIFY2(std::abs(amp.at(s, c) - ref.at(s, source[c])) <= 0.5 * amp.scale[c] + 1e-30, qPrintable(QString::number(c)));
        }
    }
    QVERIFY(ref.events.size() >= 1);
    QCOMPARE(amp.events.size(), ref.events.size());
    for (std::size_t e = 0; e < ref.events.size(); ++e) {
        QCOMPARE(amp.events[e].sample, ref.events[e].sample);
        QCOMPARE(amp.events[e].code, ref.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testNeuromagWaitsForGatingReader()
{
    // Six seconds are more than the ring holds; the replay waits while the reader is busy
    auto device = AcqDevice::open("neuromag://?simulate=1&meg=4&realtime=0&duration=6");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(6000));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testNeuromagInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "neuromag://megpc?simulate=1";
    QTest::newRow("rate") << "neuromag://?simulate=1&rate=0";
    QTest::newRow("buffer") << "neuromag://?simulate=1&buffer=-1";
    QTest::newRow("autostart") << "neuromag://?simulate=1&autostart=maybe";
    QTest::newRow("timeout") << "neuromag://?simulate=1&timeout=-1";
    QTest::newRow("port") << "neuromag://?port=0";
}

void TestAcqDevices::testNeuromagInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testNeuromagHardwareUnavailable()
{
#if defined(_WIN32)
    QCOMPARE(openError("neuromag://"), AcqErrorCode::PlatformUnsupported);
#else
    auto device = AcqDevice::open("neuromag://?root=/no/such/dacq&autostart=0&timeout=0.5");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("/no/such/dacq") != std::string::npos);
#endif
}

//=============================================================================================================

void TestAcqDevices::testNeuromagReadsTheDataServer()
{
#if defined(_WIN32)
    QSKIP("the Neuromag acquisition runs on Linux");
#else
    FakeDacq dacq;
    QVERIFY(dacq.ok());
    const std::string uri = std::format("neuromag://?root={}&port={}&buffer=4", dacq.root(), dacq.port());
    auto device = AcqDevice::open(uri);
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 600.0);
    QCOMPARE(info->channelCount(), std::size_t(3));
    QCOMPARE(info->channels[0].name, std::string("MEG 0111"));
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[1].name, std::string("MEG 0112"));
    QCOMPARE(info->channels[1].unit, AcqUnit::TeslaPerMeter);
    QCOMPARE(info->channels[1].scale, static_cast<double>(2.0F) * static_cast<double>(0.5F));
    QCOMPARE(info->channels[2].kind, AcqChannelKind::Stim);

    const Recording recorded = record_device(*device);
    // A buffer in the measurement file (big-endian), then one in the datagram; the closed file ends the stream
    QCOMPARE(recorded.frames(), std::size_t(8));
    for (std::size_t s = 0; s < 8; ++s) {
        QCOMPARE(recorded.at(s, 0), static_cast<double>(100 + s));
        QCOMPARE(recorded.at(s, 1), static_cast<double>(-static_cast<int>(s)));
        QCOMPARE(recorded.at(s, 2), s >= 5 ? 3.0 : 0.0);
    }
    QCOMPARE(recorded.events.size(), std::size_t(1));
    QCOMPARE(recorded.events[0].sample, std::uint64_t(5));
    QCOMPARE(recorded.events[0].code, 3);
    device->close();

    // The collector set the buffer and started and stopped both measurements: the info, then the data
    const std::vector<std::string> expected = {"pass", "name", "vara maxBuflen 4", "setu", "meas", "stop",
                                               "pass", "name", "vara maxBuflen 4", "setu", "meas", "stop"};
    QCOMPARE(dacq.commands(), expected);
#endif
}

//=============================================================================================================

void TestAcqDevices::testPolhemusStreamLayout()
{
    auto device = AcqDevice::open("polhemus://?simulate=1&stations=4,1,2");
    QVERIFY(device);
    QCOMPARE(device->description().backend, std::string("polhemus"));
    const auto info = device->stream().info();
    QCOMPARE(info->name, std::string("poses"));
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->sfreq, 40.0); // 120 records per second shared by three stations
    QCOMPARE(info->channelCount(), std::size_t(21));
    const std::array<const char*, 7> names = {"X", "Y", "Z", "QW", "QX", "QY", "QZ"};
    for (std::size_t c = 0; c < 21; ++c) {
        const AcqChannelInfo& channel = info->channels[c];
        QCOMPARE(channel.name, std::format("{} {}", std::array{1, 2, 4}[c / 7], names[c % 7]));
        QCOMPARE(channel.kind, c % 7 < 3 ? AcqChannelKind::Position : AcqChannelKind::Orientation);
        QCOMPARE(channel.unit, c % 7 < 3 ? AcqUnit::Meter : AcqUnit::None);
    }
}

//=============================================================================================================

void TestAcqDevices::testPolhemusSimulatorSweepsTheStylus()
{
    // One second of 120 records for two stations; centimetres and inches describe the same sweep
    const Recording cm = record("polhemus://?simulate=1&units=cm&realtime=0&duration=1&seed=7");
    const Recording in = record("polhemus://?simulate=1&units=in&realtime=0&duration=1&seed=7");
    QCOMPARE(cm.channels, std::size_t(14));
    QCOMPARE(cm.frames(), std::size_t(60));
    QCOMPARE(in.frames(), cm.frames());
    QVERIFY(cm.events.empty());
    for (std::size_t f = 0; f < cm.frames(); ++f) {
        for (std::size_t c = 0; c < 14; ++c) {
            QVERIFY2(std::abs(cm.at(f, c) - in.at(f, c)) < 2e-5, qPrintable(QString("frame %1 channel %2").arg(f).arg(c)));
        }
        // The tracker circles at 10 cm without turning; the stylus turns about z as it sweeps
        QVERIFY(std::abs(std::hypot(cm.at(f, 7), cm.at(f, 8)) - 0.10) < 1e-4);
        QVERIFY(std::abs(cm.at(f, 10) - 1.0) < 1e-6);
        QVERIFY(std::abs(std::hypot(cm.at(f, 3), cm.at(f, 6)) - 1.0) < 1e-5);
        QVERIFY(std::abs(cm.at(f, 4)) < 1e-6 && std::abs(cm.at(f, 5)) < 1e-6);
    }
    // The stylus moves from frame to frame
    QVERIFY(std::abs(cm.at(1, 0) - cm.at(0, 0)) > 1e-3);
}

//=============================================================================================================

void TestAcqDevices::testPolhemusButtonPressIsAnEvent()
{
    // A press every quarter second (15 updates) pauses the output; the pause repeats the last poses in a frame
    // with the event, and the output starts again. The press with the last record ends the session instead.
    const Recording r = record("polhemus://?simulate=1&button=0.25&pause=80&duration=1");
    QCOMPARE(r.frames(), std::size_t(63));
    QCOMPARE(r.events.size(), std::size_t(3));
    for (std::size_t e = 0; e < r.events.size(); ++e) {
        const std::size_t at = 16 * e + 15;
        QCOMPARE(r.events[e].sample, std::uint64_t(at));
        QCOMPARE(r.events[e].code, 1);
        for (std::size_t c = 0; c < r.channels; ++c) {
            QCOMPARE(r.at(at, c), r.at(at - 1, c));
        }
    }
}

//=============================================================================================================

void TestAcqDevices::testPolhemusInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "polhemus://COM3?simulate=1";
    QTest::newRow("station") << "polhemus://?simulate=1&stations=5";
    QTest::newRow("repeated station") << "polhemus://?simulate=1&stations=1,1";
    QTest::newRow("no station") << "polhemus://?simulate=1&stations=";
    QTest::newRow("units") << "polhemus://?simulate=1&units=mm";
    QTest::newRow("hemisphere zero") << "polhemus://?simulate=1&hemisphere=0,0,0";
    QTest::newRow("hemisphere short") << "polhemus://?simulate=1&hemisphere=1,2";
    QTest::newRow("baud") << "polhemus://?simulate=1&baud=0";
    QTest::newRow("pause") << "polhemus://?simulate=1&pause=0";
}

void TestAcqDevices::testPolhemusInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testPolhemusHardwareUnavailable()
{
#if defined(_WIN32)
    QCOMPARE(openError("polhemus://COM250"), AcqErrorCode::DeviceNotFound);
#else
    auto device = AcqDevice::open("polhemus:///no/such/port");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("/no/such/port") != std::string::npos);
#endif
}

//=============================================================================================================

void TestAcqDevices::testPolhemusReadsTheSerialPort()
{
#if defined(_WIN32)
    QSKIP("pseudo-terminals are POSIX");
#else
    FakeFastrak fastrak;
    QVERIFY(!fastrak.port().empty());
    auto device = AcqDevice::open(std::format("polhemus://{}?hemisphere=0,0,1", fastrak.port()));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QVERIFY(device->description().device.find(fastrak.port()) != std::string::npos);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    std::vector<float> frames;
    std::vector<AcqEvent> events;
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (frames.size() < 4 * 14 && std::chrono::steady_clock::now() < deadline) {
        auto view = reader->acquire(1, 100ms);
        if (!view) {
            continue;
        }
        const std::size_t first = frames.size();
        frames.resize(first + 14 * view->sampleCount());
        QVERIFY(decode(*view, AcqMatrixRef<float>::columnMajor(frames.data() + first, 14, view->sampleCount())).ok());
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    device->close();
    QCOMPARE(frames.size(), std::size_t(4 * 14));

    // Inches to metres; azimuth 90 degrees turns about z; the quaternion 0 0 0 2 is normalised
    const auto at = [&](std::size_t frame, std::size_t channel) {
        return static_cast<double>(frames[frame * 14 + channel]);
    };
    const std::array<double, 14> first = {0.0254, 0.0508, -0.0762, std::sqrt(0.5), 0, 0, std::sqrt(0.5),
                                          0.8829 * 0.0254, -0.3055 * 0.0254, -0.0254, 0, 0, 0, 1};
    for (std::size_t c = 0; c < 14; ++c) {
        QVERIFY2(std::abs(at(0, c) - first[c]) < 1e-6, qPrintable(QString::number(c)));
    }
    // A record without angles keeps the identity
    QVERIFY(std::abs(at(1, 2) - 0.0254) < 1e-6 && at(1, 3) == 1.0);
    QVERIFY(std::abs(at(1, 8) - 0.0254) < 1e-6);
    // The silence after the second update was a button press: a frame at its poses, then the next update
    QCOMPARE(events.size(), std::size_t(1));
    QCOMPARE(events[0].sample, std::uint64_t(2));
    QCOMPARE(events[0].code, 1);
    for (std::size_t c = 0; c < 14; ++c) {
        QCOMPARE(at(2, c), at(1, c));
    }
    QVERIFY(std::abs(at(3, 0) - 0.0508) < 1e-6 && std::abs(at(3, 8) - 0.0508) < 1e-6);

    const std::vector<std::string> expected = {"P", "l1,1", "l2,1", "l3,0", "l4,0", "O1,2,4,1", "H1,0,0,1", "O2,2,4,1", "H2,0,0,1", "C", "C", "P"};
    QVERIFY(QTest::qWaitFor([&] { return fastrak.commands().size() >= expected.size(); }, 2000));
    QCOMPARE(fastrak.commands(), expected);
#endif
}

//=============================================================================================================

void TestAcqDevices::testFtBufferStreamLayout()
{
    auto device = AcqDevice::open("ftbuffer://?simulate=1&meg=4&eeg=2&rate=500");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("ftbuffer"));
    // neuromag2ft puts the acquisition's channels and measurement info, and floats in physical units
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->sfreq, 500.0);
    QCOMPARE(info->channelCount(), std::size_t(7));
    QCOMPARE(info->channels[0].name, std::string("MEG 0001"));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Meg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[0].scale, 1.0);
    QVERIFY(info->channels[0].fiff.has_value());
    QCOMPARE(info->channels[4].name, std::string("EEG 001"));
    QCOMPARE(info->channels[6].name, std::string("STI 014"));
    QCOMPARE(info->channels[6].kind, AcqChannelKind::Stim);
    QCOMPARE(info->extensions.size(), std::size_t(1));
    QCOMPARE(info->extensions[0].schema, std::string("fiff.MeasInfo/1"));
}

//=============================================================================================================

void TestAcqDevices::testFtBufferMatchesNeuromag()
{
    // The relay puts the acquisition's samples times calibration and range
    const Recording relayed = record("ftbuffer://?simulate=1&meg=4&eeg=2&rate=500&block=100&seed=3&realtime=0&duration=2");
    const Recording direct = record("neuromag://?simulate=1&meg=4&eeg=2&rate=500&seed=3&realtime=0&duration=2");
    QCOMPARE(relayed.channels, std::size_t(7));
    QCOMPARE(relayed.frames(), std::size_t(1000));
    QCOMPARE(direct.frames(), relayed.frames());
    for (std::size_t s = 0; s < relayed.frames(); ++s) {
        for (std::size_t c = 0; c < 7; ++c) {
            QVERIFY2(std::abs(relayed.at(s, c) - direct.at(s, c)) <= 1e-6 * std::abs(direct.at(s, c)) + 1e-30, qPrintable(QString::number(c)));
        }
    }
    QVERIFY(!direct.events.empty());
    QCOMPARE(relayed.events.size(), direct.events.size());
    for (std::size_t e = 0; e < direct.events.size(); ++e) {
        QCOMPARE(relayed.events[e].sample, direct.events[e].sample);
        QCOMPARE(relayed.events[e].code, direct.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testFtBufferWaitsForGatingReader()
{
    // Six seconds are more than the stream's ring holds; the relay waits while the reader is busy
    auto device = AcqDevice::open("ftbuffer://?simulate=1&meg=4&realtime=0&duration=6");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);

    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(6000));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testFtBufferInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "ftbuffer://localhost?simulate=1";
    QTest::newRow("port") << "ftbuffer://localhost:0";
    QTest::newRow("port text") << "ftbuffer://localhost:ft";
    QTest::newRow("host") << "ftbuffer://:1972";
    QTest::newRow("timeout") << "ftbuffer://?simulate=1&timeout=0";
    QTest::newRow("channels") << "ftbuffer://?simulate=1&meg=0";
    QTest::newRow("rate") << "ftbuffer://?simulate=1&rate=0";
}

void TestAcqDevices::testFtBufferInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testFtBufferServerUnavailable()
{
    auto device = AcqDevice::open("ftbuffer://127.0.0.1:1?timeout=0.5");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("127.0.0.1:1") != std::string::npos);
}

//=============================================================================================================

void TestAcqDevices::testFtBufferReadsTheServer()
{
#if defined(_WIN32)
    QSKIP("the fake server uses POSIX sockets");
#else
    FakeFtBuffer buffer;
    QVERIFY(buffer.port() > 0);
    auto device = AcqDevice::open(std::format("ftbuffer://127.0.0.1:{}", buffer.port()));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 600.0);
    QCOMPARE(info->channelCount(), std::size_t(3));
    QCOMPARE(info->channels[0].name, std::string("MEG 0111"));
    QCOMPARE(info->channels[1].unit, AcqUnit::TeslaPerMeter);
    QCOMPARE(info->channels[1].scale, 1.0);
    QCOMPARE(info->channels[2].kind, AcqChannelKind::Stim);

    // The isotrak point joined the measurement info: a big-endian FIFF_DIG_POINT tag of 20 bytes
    const std::vector<std::byte>& file = info->extensions.at(0).data;
    std::vector<std::byte> dig;
    for (const std::int32_t word : {213, 33, 20}) {
        append_value(dig, word, true);
    }
    QVERIFY(std::search(file.begin(), file.end(), dig.begin(), dig.end()) != file.end());

    const Recording recorded = record_device(*device);
    QCOMPARE(recorded.frames(), std::size_t(8));
    for (std::size_t s = 0; s < 8; ++s) {
        QCOMPARE(static_cast<float>(recorded.at(s, 0)), 1e-12F * static_cast<float>(100 + s));
        QCOMPARE(static_cast<float>(recorded.at(s, 1)), -1e-11F * static_cast<float>(s));
    }
    // The numeric event of the buffer, then the change of STI 014; the event with a text value is not one
    QCOMPARE(recorded.events.size(), std::size_t(2));
    QCOMPARE(recorded.events[0].sample, std::uint64_t(2));
    QCOMPARE(recorded.events[0].code, 7);
    QCOMPARE(recorded.events[1].sample, std::uint64_t(5));
    QCOMPARE(recorded.events[1].code, 3);
    device->close();

    // Header, the counts at start, the counts with data, events, samples, and the new header's counts
    const std::vector<std::uint16_t> expected = {0x201, 0x402, 0x402, 0x203, 0x202, 0x402};
    QCOMPARE(buffer.commands(), expected);
#endif
}

//=============================================================================================================

void TestAcqDevices::testRtServerStreamLayout()
{
    auto device = AcqDevice::open("rtserver://?simulate=1&meg=4&eeg=2&rate=500");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("rtserver"));
    // The server sends the recording's channels and measurement info, and calibrated floats
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->sfreq, 500.0);
    QCOMPARE(info->channelCount(), std::size_t(7));
    QCOMPARE(info->channels[0].name, std::string("MEG 0001"));
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[0].scale, 1.0);
    QCOMPARE(info->channels[6].kind, AcqChannelKind::Stim);
    QCOMPARE(info->extensions.size(), std::size_t(1));
    QCOMPARE(info->extensions[0].schema, std::string("fiff.MeasInfo/1"));
}

//=============================================================================================================

void TestAcqDevices::testRtServerMatchesNeuromag()
{
    // The connector sends the recording's samples times calibration and range
    const Recording replayed = record("rtserver://?simulate=1&meg=4&eeg=2&rate=500&buffer=50&seed=3&realtime=0&duration=2");
    const Recording direct = record("neuromag://?simulate=1&meg=4&eeg=2&rate=500&seed=3&realtime=0&duration=2");
    QCOMPARE(replayed.channels, std::size_t(7));
    QCOMPARE(replayed.frames(), std::size_t(1000));
    QCOMPARE(direct.frames(), replayed.frames());
    for (std::size_t s = 0; s < replayed.frames(); ++s) {
        for (std::size_t c = 0; c < 7; ++c) {
            QVERIFY2(std::abs(replayed.at(s, c) - direct.at(s, c)) <= 1e-6 * std::abs(direct.at(s, c)) + 1e-30, qPrintable(QString::number(c)));
        }
    }
    QVERIFY(!direct.events.empty());
    QCOMPARE(replayed.events.size(), direct.events.size());
    for (std::size_t e = 0; e < direct.events.size(); ++e) {
        QCOMPARE(replayed.events[e].sample, direct.events[e].sample);
        QCOMPARE(replayed.events[e].code, direct.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testRtServerRestarts()
{
    // A stopped stream starts again where the server is then
    auto device = AcqDevice::open("rtserver://?simulate=1&meg=4&rate=1000");
    QVERIFY(device);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    for (int run = 0; run < 2; ++run) {
        QVERIFY(device->start().ok());
        auto view = reader->acquire(100, 5s);
        QVERIFY2(view, view ? "" : view.error().text().c_str());
        QVERIFY(reader->release(*view).ok());
        device->stop();
    }
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testRtServerInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "rtserver://localhost?simulate=1";
    QTest::newRow("port") << "rtserver://localhost:0";
    QTest::newRow("no data port") << "rtserver://localhost:65535";
    QTest::newRow("port text") << "rtserver://localhost:rt";
    QTest::newRow("host") << "rtserver://:4217";
    QTest::newRow("timeout") << "rtserver://?simulate=1&timeout=0";
    QTest::newRow("channels") << "rtserver://?simulate=1&meg=0";
    QTest::newRow("connector") << "rtserver://?simulate=1&connector=-1";
    QTest::newRow("buffer") << "rtserver://?simulate=1&buffer=-1";
    QTest::newRow("simfile") << "rtserver://?simulate=1&simfile=a.fif";
}

void TestAcqDevices::testRtServerInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testRtServerServerUnavailable()
{
    auto device = AcqDevice::open("rtserver://127.0.0.1:1?timeout=0.5");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("127.0.0.1:1") != std::string::npos);
}

//=============================================================================================================

void TestAcqDevices::testRtServerReadsTheServer()
{
#if defined(_WIN32)
    QSKIP("the fake server uses POSIX sockets");
#else
    FakeRtServer server;
    QVERIFY(server.port() > 0);
    auto device = AcqDevice::open(std::format("rtserver://127.0.0.1:{}?connector=2&simfile=/data/run.fif&buffer=50&alias=scan", server.port()));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 600.0);
    QCOMPARE(info->channelCount(), std::size_t(3));
    QCOMPARE(info->channels[0].name, std::string("MEG 0111"));
    QCOMPARE(info->channels[1].unit, AcqUnit::TeslaPerMeter);
    QCOMPARE(info->channels[1].scale, 1.0);
    QCOMPARE(info->channels[2].kind, AcqChannelKind::Stim);

    // The buffer of an earlier start is dropped; the end of the raw-data block ends the stream
    const Recording recorded = record_device(*device);
    QCOMPARE(recorded.frames(), std::size_t(8));
    for (std::size_t s = 0; s < 8; ++s) {
        QCOMPARE(static_cast<float>(recorded.at(s, 0)), 1e-12F * static_cast<float>(100 + s));
        QCOMPARE(static_cast<float>(recorded.at(s, 1)), -1e-11F * static_cast<float>(s));
    }
    QCOMPARE(recorded.events.size(), std::size_t(1));
    QCOMPARE(recorded.events[0].sample, std::uint64_t(5));
    QCOMPARE(recorded.events[0].code, 3);
    device->close();

    const std::vector<std::string> expected = {"selcon 2", "simfile /data/run.fif", "bufsize 50", "measinfo 5", "start 5", "stop 5"};
    QVERIFY(QTest::qWaitFor([&] { return server.commands().size() >= expected.size(); }, 2000));
    QCOMPARE(server.commands(), expected);
    QCOMPARE(server.alias(), std::string("scan"));

    // A file the connector refuses
    FakeRtServer refusing;
    QCOMPARE(openError(std::format("rtserver://127.0.0.1:{}?simfile=notes.txt", refusing.port())), AcqErrorCode::InvalidParameter);
#endif
}

//=============================================================================================================
// MAIN
//=============================================================================================================

void TestAcqDevices::testNatusStreamLayout()
{
    auto device = AcqDevice::open("natus://?simulate=1&channels=8&rate=1024");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("natus"));
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->sfreq, 1024.0);
    QCOMPARE(info->channelCount(), std::size_t(8));
    QCOMPARE(info->channels[0].name, std::string("EEG 001"));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[7].scale, 1e-7);
}

//=============================================================================================================

void TestAcqDevices::testNatusMatchesSynthetic()
{
    // The streaming machine sends the amplifier's volts in units of 0.1 uV
    const Recording natus = record("natus://?simulate=1&channels=6&rate=512&block=32&seed=4&realtime=0&duration=2");
    const Recording direct = record("synthetic://?sfreq=512&stim=0&eeg=6&block=32&seed=4&realtime=0&duration=2");
    QCOMPARE(natus.channels, std::size_t(6));
    QCOMPARE(natus.frames(), std::size_t(1024));
    QCOMPARE(direct.frames(), natus.frames());
    for (std::size_t s = 0; s < natus.frames(); ++s) {
        for (std::size_t c = 0; c < 6; ++c) {
            QVERIFY(std::abs(natus.at(s, c) - direct.at(s, c)) <= 1e-6 * std::abs(direct.at(s, c)) + 1e-13);
        }
    }
}

//=============================================================================================================

void TestAcqDevices::testNatusWaitsForGatingReader()
{
    auto device = AcqDevice::open("natus://?simulate=1&channels=4&rate=2048&realtime=0&duration=6");
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::this_thread::sleep_for(300ms);
    std::size_t frames = 0;
    while (true) {
        auto view = reader->acquire(1, 10s);
        if (!view) {
            QCOMPARE(view.error().code(), AcqErrorCode::EndOfStream);
            break;
        }
        frames += view->sampleCount();
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(frames, std::size_t(6 * 2048));
    QVERIFY(device->lastError().ok());
}

//=============================================================================================================

void TestAcqDevices::testNatusInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "natus://0.0.0.0?simulate=1";
    QTest::newRow("port") << "natus://?port=0";
    QTest::newRow("channels") << "natus://?simulate=1&channels=0";
    QTest::newRow("rate") << "natus://?simulate=1&rate=0";
    QTest::newRow("datagram") << "natus://?simulate=1&channels=1024&block=64";
    QTest::newRow("address") << "natus://localhost";
}

void TestAcqDevices::testNatusInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testNatusReceivesDatagrams()
{
#if defined(_WIN32)
    QSKIP("the sender uses POSIX sockets");
#else
    // A free port: bind one, note it, let it go
    const int probe = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t length = sizeof(to);
    QVERIFY(::bind(probe, reinterpret_cast<sockaddr*>(&to), sizeof(to)) == 0);
    QVERIFY(::getsockname(probe, reinterpret_cast<sockaddr*>(&to), &length) == 0);
    ::close(probe);

    auto device = AcqDevice::open(std::format("natus://127.0.0.1?port={}&channels=2&rate=2048", ntohs(to.sin_port)));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QVERIFY(openError(std::format("natus://127.0.0.1?port={}", ntohs(to.sin_port))) == AcqErrorCode::DeviceIo); // taken
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    const int sender = ::socket(AF_INET, SOCK_DGRAM, 0);
    const auto send = [&](std::initializer_list<float> values) {
        std::vector<std::byte> datagram;
        for (const float v : values) {
            append_value(datagram, v);
        }
        ::sendto(sender, datagram.data(), datagram.size(), 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
    };
    send({0.0F, 2.0F, 2.0F, 10.0F, -20.0F, 30.0F, -40.0F}); // two samples of two channels
    send({1.0F, 1.0F, 2.0F, 50.0F, -60.0F});
    std::array<float, 6> got{};
    const auto read = reader->read(AcqMatrixRef<float>::columnMajor(got.data(), 2, 3), 2s);
    QVERIFY2(read, read ? "" : read.error().text().c_str());
    const std::array<float, 6> expected = {1e-6F, -2e-6F, 3e-6F, -4e-6F, 5e-6F, -6e-6F};
    for (std::size_t i = 0; i < got.size(); ++i) {
        QVERIFY(std::abs(got[i] - expected[i]) < 1e-12F);
    }

    // A streaming machine set up for other channels ends the stream with the reason
    send({2.0F, 1.0F, 3.0F, 1.0F, 2.0F, 3.0F});
    QVERIFY(QTest::qWaitFor([&] { return !device->lastError().ok(); }, 2000));
    QCOMPARE(device->lastError().code(), AcqErrorCode::DeviceIo);
    QVERIFY(device->lastError().text().find("3 channels") != std::string::npos);
    ::close(sender);
#endif
}

//=============================================================================================================

void TestAcqDevices::testBabyMegStreamLayout()
{
    auto device = AcqDevice::open("babymeg://?simulate=1&meg=6&ref=2&rate=500");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("babymeg"));
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float32);
    QCOMPARE(info->sfreq, 500.0);
    QCOMPARE(info->channelCount(), std::size_t(17)); // 8 magnetometers, 8 trigger lines, the trigger word
    QCOMPARE(info->channels[0].name, std::string("MEG001"));
    QCOMPARE(info->channels[0].kind, AcqChannelKind::Meg);
    QCOMPARE(info->channels[0].unit, AcqUnit::Tesla);
    QCOMPARE(info->channels[0].fiff->coilType, 7002);
    QCOMPARE(info->channels[6].fiff->coilType, 7003);
    QCOMPARE(info->channels[6].fiff->kind, 301);
    QCOMPARE(info->channels[0].scale, double(1e-13F) * double(1.0F / 0.5F));
    QCOMPARE(info->channels[8].name, std::string("TRG001"));
    QCOMPARE(info->channels[16].name, std::string("DTRG01"));
    QCOMPARE(info->channels[16].kind, AcqChannelKind::Stim);
    // The sensor positions reach the measurement info: r0 of the first sensor lies on the 7.5 cm sphere
    const auto& loc = info->channels[0].fiff->loc;
    QVERIFY(std::abs(std::hypot(loc[0], loc[1], loc[2]) - 0.075) < 1e-6);
    QCOMPARE(info->extensions.size(), std::size_t(1));
}

//=============================================================================================================

void TestAcqDevices::testBabyMegMatchesSynthetic()
{
    // The server sends the session's tesla divided by cal / gain; its trigger lines carry the session's trigger word
    const Recording meg = record("babymeg://?simulate=1&meg=3&ref=1&rate=500&samples=50&seed=5&realtime=0&duration=2");
    const Recording direct = record("synthetic://?sfreq=500&meg=4&eeg=0&stim=1&block=50&seed=5&realtime=0&duration=2");
    QCOMPARE(meg.channels, std::size_t(13));
    QCOMPARE(meg.frames(), std::size_t(1000));
    QCOMPARE(direct.frames(), meg.frames());
    for (std::size_t s = 0; s < meg.frames(); ++s) {
        for (std::size_t c = 0; c < 4; ++c) {
            QVERIFY(std::abs(meg.at(s, c) - direct.at(s, c)) <= 1e-6 * std::abs(direct.at(s, c)) + 1e-20);
        }
        QCOMPARE(meg.at(s, 12), direct.at(s, 4)); // DTRG01 is the trigger word
    }
    QVERIFY(!direct.events.empty());
    QCOMPARE(meg.events.size(), direct.events.size());
    for (std::size_t e = 0; e < direct.events.size(); ++e) {
        QCOMPARE(meg.events[e].sample, direct.events[e].sample);
        QCOMPARE(meg.events[e].code, direct.events[e].code);
    }
}

//=============================================================================================================

void TestAcqDevices::testBabyMegInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("target") << "babymeg://localhost?simulate=1";
    QTest::newRow("port") << "babymeg://?port=0";
    QTest::newRow("command port") << "babymeg://?command_port=70000";
    QTest::newRow("sensors") << "babymeg://?simulate=1&meg=0&ref=0";
    QTest::newRow("rate") << "babymeg://?simulate=1&rate=0";
    QTest::newRow("samples") << "babymeg://?simulate=1&samples=0";
}

void TestAcqDevices::testBabyMegInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testBabyMegServerUnavailable()
{
    auto device = AcqDevice::open("babymeg://127.0.0.1?port=1&timeout=0.5");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::DeviceNotFound);
    QVERIFY(device.error().text().find("127.0.0.1:1") != std::string::npos);
}

//=============================================================================================================

void TestAcqDevices::testBabyMegReadsTheServer()
{
#if defined(_WIN32)
    QSKIP("the fake server uses POSIX sockets");
#else
    FakeBabyMeg server;
    QVERIFY(server.data_port() > 0 && server.command_port() > 0);
    auto device = AcqDevice::open(std::format("babymeg://127.0.0.1?port={}&command_port={}", server.data_port(), server.command_port()));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 600.0);
    QCOMPARE(info->channelCount(), std::size_t(3));
    QCOMPARE(info->channels[0].name, std::string("MEG001"));
    QCOMPARE(info->channels[1].kind, AcqChannelKind::Stim);

    // Two packages of four samples; the third request finds the server idle until QUIT
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    std::array<float, 24> got{};
    const auto read = reader->read(AcqMatrixRef<float>::columnMajor(got.data(), 3, 8), 2s);
    QVERIFY2(read, read ? "" : read.error().text().c_str());
    for (std::size_t s = 0; s < 8; ++s) {
        QVERIFY(std::abs(got[3 * s] - 2e-13F * static_cast<float>(s + 1)) < 1e-18F); // raw s + 1 times 1e-13 / 0.5
    }
    const std::array<float, 8> word = {0, 0, 1, 1, 1, 0, 0, 1};
    for (std::size_t s = 0; s < 8; ++s) {
        QCOMPARE(got[3 * s + 2], word[s]);
    }
    device->close();

    const std::vector<std::string> expected = {"INFO", "DATA", "DATA", "DATA", "QUIT", "QREL"};
    QVERIFY(QTest::qWaitFor([&] { return server.commands().size() >= expected.size(); }, 2000));
    QCOMPARE(server.commands(), expected);
#endif
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowStreamLayout()
{
    // A board by name and by BrainFlow's id
    for (const std::string board : {"synthetic", "-1"}) {
        auto device = AcqDevice::open("brainflow://" + board + "?library=" BRAINFLOW_STUB);
        QVERIFY2(device, device ? "" : device.error().text().c_str());
        QCOMPARE(device->description().backend, std::string("brainflow"));
        QCOMPARE(device->description().backendVersion, std::string("5.12.1"));
        QCOMPARE(device->description().device, std::string("BrainFlow Synthetic (board -1)"));
    }

    // BrainFlow's rows in its units: microvolts for EEG, seconds for the timestamp
    auto device = AcqDevice::open("brainflow://synthetic?library=" BRAINFLOW_STUB);
    QVERIFY(device);
    const auto info = device->stream().info();
    QCOMPARE(info->format, AcqSampleFormat::Float64);
    QCOMPARE(info->layout, AcqLayout::Interleaved);
    QCOMPARE(info->sfreq, 250.0);
    QCOMPARE(info->channelCount(), std::size_t(12));
    const std::vector<std::string> names = {"PACKAGE", "Fz", "Cz", "Pz", "Oz", "ACC 001",
                                            "ACC 002", "ACC 003", "TEMP 001", "BATTERY", "TIMESTAMP", "MARKER"};
    for (std::size_t c = 0; c < names.size(); ++c) {
        QCOMPARE(info->channels[c].name, names[c]);
    }
    QCOMPARE(info->channels[1].kind, AcqChannelKind::Eeg); // EEG wins over the same rows as EMG
    QCOMPARE(info->channels[1].unit, AcqUnit::Volt);
    QCOMPARE(info->channels[1].scale, 1e-6);
    QCOMPARE(info->channels[5].kind, AcqChannelKind::Accelerometer);
    QCOMPARE(info->channels[5].unit, AcqUnit::Unknown);
    QCOMPARE(info->channels[8].kind, AcqChannelKind::Temperature);
    QCOMPARE(info->channels[8].unit, AcqUnit::DegreeCelsius);
    QCOMPARE(info->channels[10].kind, AcqChannelKind::Timestamp);
    QCOMPARE(info->channels[10].unit, AcqUnit::Second);
    QCOMPARE(info->channels[11].kind, AcqChannelKind::Marker);
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowReadsTheBoard()
{
    auto device = AcqDevice::open("brainflow://synthetic?library=" BRAINFLOW_STUB);
    QVERIFY(device);
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    constexpr std::size_t width = 12;
    std::vector<double> frames;
    std::vector<AcqEvent> events;
    while (frames.size() < width * 100) {
        auto view = reader->acquire(1, 5s);
        QVERIFY2(view, view ? "" : view.error().text().c_str());
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < width; ++c) {
                    double v = 0.0;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    frames.push_back(v);
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    const AcqClockModel clock = device->stream().clock();
    QVERIFY(device->stop().ok());

    // Rows become channels, sample after sample
    for (std::size_t s = 0; s < 100; ++s) {
        QCOMPARE(frames[s * width + 1], 1.0 + static_cast<double>(s) / 1000.0);
        QCOMPARE(frames[s * width + 4], 4.0 + static_cast<double>(s) / 1000.0);
        QCOMPARE(frames[s * width + 10], 1.7e9 + static_cast<double>(s) / 250.0);
    }
    // The marker row as events
    QCOMPARE(events.size(), std::size_t(1));
    QCOMPARE(events.front().sample, std::uint64_t(30));
    QCOMPARE(events.front().code, 7);
    // The timestamp row as the device clock: UTC of a sample is its timestamp
    QCOMPARE(clock.source, AcqClockSource::Device);
    const double utc = static_cast<double>(clock.physicalNs(50) + clock.utcOffsetNs) * 1e-9;
    QVERIFY2(std::abs(utc - (1.7e9 + 50.0 / 250.0)) < 1e-5, std::format("{}", utc).c_str());
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowPassesInputParams()
{
    // The stand-in's Cyton opens only on its serial port, which reaches BrainFlow in the input parameters
    QCOMPARE(openError("brainflow://cyton?library=" BRAINFLOW_STUB), AcqErrorCode::DeviceNotFound);
    auto device = AcqDevice::open("brainflow://cyton?library=" BRAINFLOW_STUB "&serial_port=COM-STUB&timeout=5");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().device, std::string("BrainFlow Cyton (board 0)"));
    QCOMPARE(device->description().uri.find("serial_port=COM-STUB") != std::string::npos, true);

    // A board BrainFlow describes but was built without here
    auto ganglion = AcqDevice::open("brainflow://ganglion?library=" BRAINFLOW_STUB);
    QVERIFY(!ganglion);
    QCOMPARE(ganglion.error().code(), AcqErrorCode::PlatformUnsupported);
    QVERIFY2(ganglion.error().text().find("Ganglion") != std::string::npos, ganglion.error().text().c_str());
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("no board") << QString("brainflow://?library=" BRAINFLOW_STUB);
    QTest::newRow("unknown name") << QString("brainflow://nonesuch?library=" BRAINFLOW_STUB);
    QTest::newRow("unsupported id") << QString("brainflow://5?library=" BRAINFLOW_STUB);
    QTest::newRow("port not a number") << QString("brainflow://-1?library=" BRAINFLOW_STUB "&ip_port=x");
    QTest::newRow("unknown parameter") << QString("brainflow://-1?library=" BRAINFLOW_STUB "&serialport=COM1");
}

void TestAcqDevices::testBrainFlowInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowLibraryUnavailable()
{
    auto device = AcqDevice::open("brainflow://synthetic?library=/nonexistent/BoardController");
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::BackendUnavailable);
    const std::string text = device.error().text();
    QVERIFY2(text.find("BrainFlow not found") != std::string::npos, text.c_str());
    QVERIFY2(text.find("/nonexistent/BoardController") != std::string::npos, text.c_str());
    QVERIFY2(text.find("pip install brainflow") != std::string::npos, text.c_str());
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowRefusesUnsupportedVersion()
{
    auto device = AcqDevice::open("brainflow://synthetic?library=" BRAINFLOW_STUB_OLD);
    QVERIFY(!device);
    QCOMPARE(device.error().code(), AcqErrorCode::UnsupportedVersion);
    QVERIFY2(device.error().text().find("4.9.2") != std::string::npos, device.error().text().c_str());
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowSyntheticBoard()
{
    // BrainFlow itself, where it is installed (MNE_CPP_BRAINFLOW_LIB or pip install brainflow)
    auto device = AcqDevice::open("brainflow://synthetic");
    if (!device && device.error() == AcqErrorCode::BackendUnavailable) {
        QSKIP("BrainFlow is not installed");
    }
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->sfreq, 250.0);
    QCOMPARE(info->channels[1].name, std::string("Fz"));
    QCOMPARE(info->channels[1].kind, AcqChannelKind::Eeg);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    auto view = reader->acquire(25, 5s);
    QVERIFY2(view, view ? "" : view.error().text().c_str());
    QVERIFY(reader->release(*view).ok());
    QCOMPARE(device->stream().clock().source, AcqClockSource::Device);
    QVERIFY(device->stop().ok());
}

//=============================================================================================================

void TestAcqDevices::testStreamingBoardDecodesDatagrams()
{
#if defined(_WIN32)
    QSKIP("the sender uses POSIX sockets");
#else
    const int probe = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t length = sizeof(to);
    QVERIFY(::bind(probe, reinterpret_cast<sockaddr*>(&to), sizeof(to)) == 0);
    QVERIFY(::getsockname(probe, reinterpret_cast<sockaddr*>(&to), &length) == 0);
    ::close(probe);

    // The synthetic board: 32 rows at 250 Hz, EEG in rows 1-16, timestamp in row 30, marker in row 31
    auto device = AcqDevice::open(std::format("streaming_board://127.0.0.1:{}?board=synthetic", ntohs(to.sin_port)));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("streaming_board"));
    const auto info = device->stream().info();
    QCOMPARE(info->channelCount(), std::size_t(32));
    QCOMPARE(info->sfreq, 250.0);
    QCOMPARE(info->channels[1].name, std::string("Fz"));
    QCOMPARE(info->channels[1].kind, AcqChannelKind::Eeg);
    QCOMPARE(info->channels[31].kind, AcqChannelKind::Marker);
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    constexpr double kStart = 1.8e9;
    const auto sample = [](std::size_t s) {
        std::array<double, 32> rows{};
        for (std::size_t r = 0; r < rows.size(); ++r) {
            rows[r] = static_cast<double>(s * 100 + r);
        }
        rows[30] = kStart + static_cast<double>(s) / 250.0;
        rows[31] = s == 1 ? 5.0 : 0.0;
        return rows;
    };
    const int sender = ::socket(AF_INET, SOCK_DGRAM, 0);
    const auto send = [&](std::size_t first, std::size_t count, std::size_t cut = 0) {
        std::vector<double> datagram;
        for (std::size_t s = first; s < first + count; ++s) {
            const auto rows = sample(s);
            datagram.insert(datagram.end(), rows.begin(), rows.end());
        }
        ::sendto(sender, datagram.data(), datagram.size() * sizeof(double) - cut, 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
    };
    send(0, 2);    // any number of samples per datagram
    send(2, 1, 8); // not whole samples: dropped
    send(2, 1);

    std::vector<double> got;
    std::vector<AcqEvent> events;
    while (got.size() < 3 * 32) {
        auto view = reader->acquire(1, 2s);
        QVERIFY2(view, view ? "" : view.error().text().c_str());
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < 32; ++c) {
                    double v = 0.0;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    got.push_back(v);
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    QCOMPARE(got.size(), std::size_t(3 * 32));
    for (std::size_t s = 0; s < 3; ++s) {
        const auto rows = sample(s);
        for (std::size_t r = 0; r < 32; ++r) {
            QCOMPARE(got[s * 32 + r], rows[r]);
        }
    }
    QCOMPARE(events.size(), std::size_t(1));
    QCOMPARE(events[0].sample, std::uint64_t(1));
    QCOMPARE(events[0].code, 5);
    QCOMPARE(device->stream().clock().source, AcqClockSource::Device);
    ::close(sender);
    QVERIFY(device->stop().ok());
#endif
}

//=============================================================================================================

void TestAcqDevices::testStreamingBoardSimulates()
{
    auto device = AcqDevice::open("streaming_board://?simulate=1");
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    QCOMPARE(info->channelCount(), std::size_t(32));
    QCOMPARE(info->channels[16].name, std::string("F8"));
    AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    // The synthetic session's first trial starts after a second; its EEG is tens of microvolts
    std::size_t samples = 0;
    double peak = 0.0;
    std::vector<AcqEvent> events;
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (events.empty() && std::chrono::steady_clock::now() < deadline) {
        auto view = reader->acquire(1, 1s);
        QVERIFY2(view, view ? "" : view.error().text().c_str());
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s, ++samples) {
                double v = 0.0;
                std::memcpy(&v, part.at(1, s), sizeof(v));
                peak = std::max(peak, std::abs(v * info->channels[1].scale));
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    QVERIFY(!events.empty());
    QVERIFY(events.front().code == 1 || events.front().code == 2);
    QVERIFY(samples > 200);
    QVERIFY2(peak > 1e-6 && peak < 1e-3, qPrintable(QString::number(peak)));
    QVERIFY(device->stop().ok());
    QVERIFY(device->start().ok()); // a restart streams again
    QVERIFY(reader->acquire(1, 2s));
    QVERIFY(device->stop().ok());
}

//=============================================================================================================

void TestAcqDevices::testStreamingBoardInvalidParameters_data()
{
    QTest::addColumn<QString>("uri");
    QTest::newRow("no board") << "streaming_board://127.0.0.1:6677";
    QTest::newRow("no port") << "streaming_board://127.0.0.1?board=synthetic";
    QTest::newRow("port 0") << "streaming_board://127.0.0.1:0?board=synthetic";
    QTest::newRow("port text") << "streaming_board://127.0.0.1:x?board=synthetic";
    QTest::newRow("no address") << "streaming_board://localhost:6677?board=synthetic";
    QTest::newRow("unknown board") << "streaming_board://127.0.0.1:6677?board=nosuchboard";
    QTest::newRow("simulator target") << "streaming_board://127.0.0.1:6677?simulate=1";
    QTest::newRow("unknown key") << "streaming_board://?simulate=1&colour=red";
}

void TestAcqDevices::testStreamingBoardInvalidParameters()
{
    QFETCH(QString, uri);
    QCOMPARE(openError(uri.toStdString()), AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqDevices::testStreamingBoardMatchesBrainFlow_data()
{
    QTest::addColumn<QString>("address");
    QTest::newRow("unicast") << "127.0.0.1";
    QTest::newRow("multicast") << "239.255.43.22";
}

void TestAcqDevices::testStreamingBoardMatchesBrainFlow()
{
    QFETCH(QString, address);
#if defined(_WIN32)
    QSKIP("the free port is found with POSIX sockets");
#else
    // BrainFlow's synthetic board streams to us while BrainFlow keeps its own copy: both must agree sample by sample
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (python.isEmpty()) {
        QSKIP("no python3 on the PATH");
    }
    const int probe = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t length = sizeof(to);
    QVERIFY(::bind(probe, reinterpret_cast<sockaddr*>(&to), sizeof(to)) == 0);
    QVERIFY(::getsockname(probe, reinterpret_cast<sockaddr*>(&to), &length) == 0);
    ::close(probe);
    const int port = ntohs(to.sin_port);

    if (address != QStringLiteral("127.0.0.1")) {
        // A member of the group, joined the way the backend joins, must hear another process first; managed firewalls
        // drop that for unsigned programs, and hosts without a multicast route cannot join at all
        bool heard = false;
        {
            auto member = mne::UdpSocket::bind(address.toStdString(), static_cast<std::uint16_t>(port), true);
            if (!member) {
                QSKIP(qPrintable(QStringLiteral("this machine cannot join %1: %2").arg(address, QString::fromStdString(member.error().message()))));
            }
            QProcess::execute(python, {QStringLiteral("-c"), QStringLiteral("import socket; socket.socket(socket.AF_INET, socket.SOCK_DGRAM)"
                                                                            ".sendto(b'x', ('%1', %2))")
                                                                 .arg(address)
                                                                 .arg(port)});
            std::array<std::byte, 16> datagram{};
            heard = member->receive(datagram, 1000ms).has_value();
        }
        if (!heard) {
            QSKIP("this machine drops multicast datagrams from other programs");
        }
    }

    auto device = AcqDevice::open(std::format("streaming_board://{}:{}?board=-1", address.toStdString(), port));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    QVERIFY(device->start().ok());

    QTemporaryDir dir;
    const QString out = dir.filePath(QStringLiteral("brainflow.f64"));
    QProcess process;
    process.start(python, {QStringLiteral(BRAINFLOW_STREAM_SCRIPT), address, QString::number(port), QStringLiteral("3"), out});
    QVERIFY(process.waitForStarted());
    std::vector<double> got;
    std::vector<AcqEvent> events;
    while (process.state() != QProcess::NotRunning || reader->available() > 0) {
        process.waitForFinished(10);
        auto view = reader->acquire(1, 10ms);
        if (!view) {
            continue;
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < 32; ++c) {
                    double v = 0.0;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    got.push_back(v);
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            events.push_back(view->events()[e]);
        }
        QVERIFY(reader->release(*view).ok());
    }
    if (process.exitCode() == 2) {
        QSKIP("BrainFlow is not installed for python3");
    }
    QVERIFY2(process.exitCode() == 0, process.readAll().constData());
    QVERIFY(device->stop().ok());

    std::ifstream file(out.toStdString(), std::ios::binary);
    std::array<std::int64_t, 2> shape{};
    file.read(reinterpret_cast<char*>(shape.data()), sizeof(shape));
    QCOMPARE(shape[0], std::int64_t(32));
    const auto samples = static_cast<std::size_t>(shape[1]);
    std::vector<double> brainflow(32 * samples);
    file.read(reinterpret_cast<char*>(brainflow.data()), static_cast<std::streamsize>(brainflow.size() * sizeof(double)));
    QVERIFY(file);

    // BrainFlow streams whole batches, so its copy may hold a last partial batch more
    QVERIFY2(got.size() / 32 <= samples && got.size() / 32 + 3 > samples, qPrintable(QString("%1 of %2").arg(got.size() / 32).arg(samples)));
    for (std::size_t s = 0; s < got.size() / 32; ++s) {
        for (std::size_t r = 0; r < 32; ++r) {
            QCOMPARE(got[s * 32 + r], brainflow[r * samples + s]);
        }
    }
    // The sender's marker arrives as an event at its sample; BrainFlow's own STREAMING_BOARD would lose it
    QCOMPARE(events.size(), std::size_t(1));
    QCOMPARE(events[0].code, 7);
    QCOMPARE(brainflow[31 * samples + events[0].sample], 7.0);
#endif
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowFileReplays()
{
    // Ganglion: 15 rows at 200 Hz, EEG in rows 1-4, timestamp in row 13, marker in row 14
    QTemporaryDir dir;
    const std::string path = dir.filePath(QStringLiteral("ganglion.csv")).toStdString();
    {
        std::ofstream file(path);
        for (int s = 0; s < 5; ++s) {
            for (int r = 0; r < 15; ++r) {
                const double value = r == 13 ? 1.8e9 + s / 200.0 : r == 14 ? (s == 3 ? 4.0 : 0.0)
                                                                           : s * 100 + r + 0.25;
                file << std::format("{:.6f}{}", value, r < 14 ? '\t' : '\n');
            }
        }
        file << "\r\n"; // blank lines and Windows line ends are fine
    }
    auto device = AcqDevice::open(std::format("brainflow_file://{}?board=ganglion&realtime=0", path));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    QCOMPARE(device->description().backend, std::string("brainflow_file"));
    const auto info = device->stream().info();
    QCOMPARE(info->channelCount(), std::size_t(15));
    QCOMPARE(info->sfreq, 200.0);
    QCOMPARE(info->channels[1].kind, AcqChannelKind::Eeg);
    const Recording recording = record_device(*device);
    QCOMPARE(recording.frames(), std::size_t(5));
    QCOMPARE(recording.at(2, 1), 201.25 * 1e-6);
    QCOMPARE(recording.at(4, 13), 1.8e9 + 4 / 200.0);
    QCOMPARE(recording.events.size(), std::size_t(1));
    QCOMPARE(recording.events[0].sample, std::uint64_t(3));
    QCOMPARE(recording.events[0].code, 4);

    // Real time: 5 samples at 200 Hz take 25 ms; a loop goes on past the end
    auto looped = AcqDevice::open(std::format("brainflow_file://{}?board=1&loop=1", path));
    QVERIFY(looped);
    auto reader = looped->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);
    const auto started = std::chrono::steady_clock::now();
    QVERIFY(looped->start().ok());
    QVERIFY(reader->acquire(12, 2s));
    QVERIFY(std::chrono::steady_clock::now() - started >= 50ms);
    QVERIFY(looped->stop().ok());
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowFileRefusesMalformedFiles_data()
{
    QTest::addColumn<QString>("content");
    QTest::addColumn<QString>("parameters");
    QTest::addColumn<int>("code");
    const QString row = QStringLiteral("0\t1\t2\t3\t4\t5\t6\t7\t8\t9\t10\t11\t12\t13\t14\n");
    QTest::newRow("other board") << row << "board=synthetic" << int(AcqErrorCode::UnsupportedFormat);
    QTest::newRow("short line") << row + "1\t2\n"
                                << "board=ganglion" << int(AcqErrorCode::UnsupportedFormat);
    QTest::newRow("text") << QString(row).replace(QStringLiteral("7"), QStringLiteral("x")) << "board=ganglion" << int(AcqErrorCode::UnsupportedFormat);
    QTest::newRow("commas") << QString(row).replace(QLatin1Char('\t'), QLatin1Char(',')) << "board=ganglion" << int(AcqErrorCode::UnsupportedFormat);
    QTest::newRow("empty") << "\n"
                           << "board=ganglion" << int(AcqErrorCode::UnsupportedFormat);
    QTest::newRow("no board") << row << "realtime=0" << int(AcqErrorCode::InvalidParameter);
    QTest::newRow("unknown board") << row << "board=nosuchboard" << int(AcqErrorCode::InvalidParameter);
    QTest::newRow("speed") << row << "board=ganglion&speed=0" << int(AcqErrorCode::InvalidParameter);
}

void TestAcqDevices::testBrainFlowFileRefusesMalformedFiles()
{
    QFETCH(QString, content);
    QFETCH(QString, parameters);
    QFETCH(int, code);
    QTemporaryDir dir;
    const std::string path = dir.filePath(QStringLiteral("file.csv")).toStdString();
    std::ofstream(path) << content.toStdString();
    QCOMPARE(int(openError(std::format("brainflow_file://{}?{}", path, parameters.toStdString()))), code);
    QCOMPARE(openError("brainflow_file:///no/such/file.csv?board=ganglion"), AcqErrorCode::DeviceNotFound);
}

//=============================================================================================================

void TestAcqDevices::testBrainFlowFileMatchesBrainFlow()
{
    // What BrainFlow's file streamer wrote replays as the samples BrainFlow itself read, to its six decimals
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (python.isEmpty()) {
        QSKIP("no python3 on the PATH");
    }
    QTemporaryDir dir;
    const QString out = dir.filePath(QStringLiteral("brainflow.f64"));
    const QString csv = dir.filePath(QStringLiteral("brainflow.csv"));
    QProcess process;
    process.start(python, {QStringLiteral(BRAINFLOW_STREAM_SCRIPT), QStringLiteral("127.0.0.1"), QStringLiteral("9"), QStringLiteral("2"), out, csv});
    QVERIFY(process.waitForFinished(30000));
    if (process.exitCode() == 2) {
        QSKIP("BrainFlow is not installed for python3");
    }
    QVERIFY2(process.exitCode() == 0, process.readAll().constData());

    std::ifstream file(out.toStdString(), std::ios::binary);
    std::array<std::int64_t, 2> shape{};
    file.read(reinterpret_cast<char*>(shape.data()), sizeof(shape));
    const auto samples = static_cast<std::size_t>(shape[1]);
    std::vector<double> brainflow(32 * samples);
    file.read(reinterpret_cast<char*>(brainflow.data()), static_cast<std::streamsize>(brainflow.size() * sizeof(double)));
    QVERIFY(file);

    auto device = AcqDevice::open(std::format("brainflow_file://{}?board=synthetic&realtime=0", csv.toStdString()));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const Recording recording = record_device(*device);
    QCOMPARE(recording.frames(), samples);
    const auto info = device->stream().info();
    for (std::size_t s = 0; s < samples; ++s) {
        for (std::size_t r = 0; r < 32; ++r) {
            const double expected = brainflow[r * samples + s] * info->channels[r].scale;
            QVERIFY2(std::abs(recording.at(s, r) - expected) <= 5e-7 * info->channels[r].scale,
                     qPrintable(QString("sample %1 row %2: %3 vs %4").arg(s).arg(r).arg(recording.at(s, r), 0, 'g', 17).arg(expected, 0, 'g', 17)));
        }
    }
    QCOMPARE(recording.events.size(), std::size_t(1));
    QCOMPARE(recording.events[0].code, 7);
    QCOMPARE(brainflow[31 * samples + recording.events[0].sample], 7.0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqDevices)
#include "test_acq_devices.moc"
