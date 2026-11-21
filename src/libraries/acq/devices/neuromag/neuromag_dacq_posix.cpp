//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     neuromag_dacq_posix.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    A client of the Neuromag data acquisition: its data server and its collector.
 *
 * The data server, on the acquisition computer, sends every client a message per FIFF tag over a UNIX datagram
 * socket. The data follow in a second datagram, lie in a block of the server's shared memory, or are in the file
 * being written (big-endian). The collector, a TCP service, starts and stops measurements and sets the buffer
 * length.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_transport.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>

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

// The data server's message per tag and its shared memory, as the acquisition declares them
struct DataMessage
{
    std::int32_t kind;
    std::int32_t type;
    std::int32_t size;
    std::int32_t loc;       // position in the file, -1 if not there
    std::int32_t shmem_buf; // shared memory block, -1 if not there
    std::int32_t shmem_loc; // position in the shared memory file, -1 if not there
};

constexpr std::size_t kShmClients = 10;
constexpr std::size_t kShmData = 500 * 1500 * 4;
constexpr std::size_t kShmBlocks = 100;

struct ShmClient
{
    std::int32_t client_id;
    std::int32_t done;
};

struct ShmBlock
{
    ShmClient clients[kShmClients];
    unsigned char data[kShmData];
};

constexpr const char* kCollectorPassword = "homunculus122";
constexpr int kMaxKind = 20000;
constexpr std::int32_t kMaxSize = 100000000;
constexpr std::chrono::milliseconds kReply(2000);

AcqError system_error(const std::string& what)
{
    return {AcqErrorCode::DeviceIo, std::format("{}: {}", what, std::strerror(errno))};
}

/** @return true when @p fd becomes readable within @p timeout. */
bool readable(int fd, std::chrono::milliseconds timeout)
{
    pollfd p{fd, POLLIN, 0};
    return ::poll(&p, 1, static_cast<int>(timeout.count())) > 0 && (p.revents & POLLIN) != 0;
}

sockaddr_un unix_address(const std::string& path)
{
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
    return address;
}

/** The collector: a line-based TCP control service. */
class Collector
{
public:
    ~Collector()
    {
        close();
    }

    AcqError open(const std::string& host, int port)
    {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* found = nullptr;
        if (::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &found) != 0 || found == nullptr) {
            return {AcqErrorCode::DeviceNotFound, std::format("the Neuromag collector host {} is unknown", host)};
        }
        for (addrinfo* a = found; a != nullptr && m_fd < 0; a = a->ai_next) {
            const int fd = ::socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            if (fd >= 0 && ::connect(fd, a->ai_addr, a->ai_addrlen) == 0) {
                m_fd = fd;
            } else if (fd >= 0) {
                ::close(fd);
            }
        }
        ::freeaddrinfo(found);
        if (m_fd < 0) {
            return {AcqErrorCode::DeviceNotFound, std::format("no Neuromag collector answers on {}:{}", host, port)};
        }
        for (const std::string& command : {std::format("pass {}\r\n", kCollectorPassword), std::string("name mne_acq\r\n")}) {
            if (AcqError e = send(command); !e.ok()) {
                return e;
            }
        }
        return {};
    }

    void close()
    {
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    /** Sends @p command and drains the reply. */
    AcqError send(const std::string& command)
    {
        if (::send(m_fd, command.data(), command.size(), 0) != static_cast<ssize_t>(command.size())) {
            return system_error("the Neuromag collector closed the connection");
        }
        std::array<char, 4096> reply{};
        if (!readable(m_fd, kReply) || ::recv(m_fd, reply.data(), reply.size(), 0) <= 0) {
            return {AcqErrorCode::DeviceIo, "the Neuromag collector did not answer " + command.substr(0, 4)};
        }
        while (readable(m_fd, std::chrono::milliseconds(0)) && ::recv(m_fd, reply.data(), reply.size(), 0) > 0) {
        }
        return {};
    }

private:
    int m_fd = -1;
};

class NeuromagClient final : public NeuromagTransport
{
public:
    explicit NeuromagClient(const NeuromagDacq& dacq)
    : m_dacq(dacq)
    , m_path(std::format("{}/sockets/dacq_client_{}", dacq.root, dacq.client))
    {
    }

    ~NeuromagClient() override
    {
        stop();
    }

    NeuromagClient(const NeuromagClient&) = delete;
    NeuromagClient& operator=(const NeuromagClient&) = delete;

    std::string description() const override
    {
        return "DACQ " + m_dacq.root;
    }

    AcqError start() override
    {
        if (m_dacq.autostart || m_dacq.buffer > 0) {
            if (AcqError e = m_collector.open(m_dacq.host, m_dacq.port); !e.ok()) {
                return e;
            }
            if (m_dacq.buffer > 0) {
                for (const std::string& command : {std::format("vara maxBuflen {}\n", m_dacq.buffer), std::string("setu\n")}) {
                    if (AcqError e = m_collector.send(command); !e.ok()) {
                        stop();
                        return e;
                    }
                }
            }
        }
        if (AcqError e = connect(); !e.ok()) {
            stop();
            return e;
        }
        if (m_dacq.autostart) {
            if (AcqError e = m_collector.send("meas\r\n"); !e.ok()) {
                stop();
                return e;
            }
            m_measuring = true;
        }
        return {};
    }

    void stop() override
    {
        if (m_measuring) {
            static_cast<void>(m_collector.send("stop\r\n"));
            m_measuring = false;
        }
        m_collector.close();
        if (m_socket >= 0) {
            static_cast<void>(request(-m_dacq.client));
            ::close(m_socket);
            ::unlink(m_path.c_str());
            m_socket = -1;
        }
        if (m_shm != nullptr) {
            ::shmdt(m_shm);
            m_shm = nullptr;
        }
        m_file.close();
        m_shmem_file.close();
    }

    AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) override
    {
        if (!readable(m_socket, timeout)) {
            return AcqError(AcqErrorCode::Timeout);
        }
        DataMessage message{};
        if (::recv(m_socket, &message, sizeof(message), 0) != static_cast<ssize_t>(sizeof(message))) {
            return system_error("the Neuromag data server closed the connection");
        }
        if (message.kind > kMaxKind || message.size < 0 || message.size > kMaxSize) {
            message.kind = fiff::kNop; // survives the occasional corrupt message
            message.size = 0;
        }
        m_tag.kind = message.kind;
        m_tag.type = message.type;
        m_tag.data.resize(static_cast<std::size_t>(message.size));
        if (message.size > 0) {
            if (AcqError e = read_data(message); !e.ok()) {
                return e;
            }
        }
        if (message.kind == fiff::kCloseFile) {
            m_file.close();
        } else if (message.kind == fiff::kNewFile) {
            m_file.close();
            m_file.open(std::string(reinterpret_cast<const char*>(m_tag.data.data()), strnlen(reinterpret_cast<const char*>(m_tag.data.data()), m_tag.data.size())),
                        std::ios::binary);
            if (!m_shmem_file.is_open()) {
                m_shmem_file.open(m_dacq.root + "/raw/data_server_shmem", std::ios::binary);
            }
        }
        return &m_tag;
    }

private:
    /** Opens the client socket and registers with the data server. */
    AcqError connect()
    {
        m_socket = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (m_socket < 0) {
            return system_error("no socket for the Neuromag data server");
        }
        ::unlink(m_path.c_str());
        const sockaddr_un address = unix_address(m_path);
        const mode_t mask = ::umask(0); // the server, another user, must be able to write to it
        const int bound = ::bind(m_socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
        ::umask(mask);
        if (bound != 0) {
            return {AcqErrorCode::DeviceNotFound,
                    std::format("no Neuromag acquisition in {} (cannot create {}: {})", m_dacq.root, m_path, std::strerror(errno))};
        }
        auto result = request(m_dacq.client);
        if (!result) {
            return result.error();
        }
        if (*result < 0) {
            return {AcqErrorCode::DeviceIo, std::format("the Neuromag data server refused client {}", m_dacq.client)};
        }
        return {};
    }

    /** Sends a (dis)connect request and returns the server's answer. */
    AcqExpected<std::int32_t> request(std::int32_t id)
    {
        const sockaddr_un server = unix_address(m_dacq.root + "/sockets/dacq_server");
        if (::sendto(m_socket, &id, sizeof(id), 0, reinterpret_cast<const sockaddr*>(&server), sizeof(server)) != static_cast<ssize_t>(sizeof(id))) {
            return AcqError(AcqErrorCode::DeviceNotFound, std::format("no Neuromag data server at {}: {}", server.sun_path, std::strerror(errno)));
        }
        std::int32_t result = -1;
        if (!readable(m_socket, kReply) || ::recv(m_socket, &result, sizeof(result), 0) != static_cast<ssize_t>(sizeof(result))) {
            return AcqError(AcqErrorCode::DeviceIo, "the Neuromag data server did not answer");
        }
        return result;
    }

    AcqError read_data(const DataMessage& message)
    {
        const auto size = static_cast<std::size_t>(message.size);
        if (message.loc < 0 && message.shmem_buf < 0 && message.shmem_loc < 0) {
            if (!readable(m_socket, kReply) || ::recv(m_socket, m_tag.data.data(), size, 0) != static_cast<ssize_t>(size)) {
                return system_error("the Neuromag data server sent no data");
            }
            return {};
        }
        const bool shared = m_dacq.client / 10000 > 0;
        if (message.shmem_buf >= 0 && shared) {
            return copy_shared(message, size);
        }
        // From the file the server writes, big-endian like every FIFF file
        std::ifstream& file = shared && message.shmem_loc >= 0 ? m_shmem_file : m_file;
        const std::int32_t loc = shared && message.shmem_loc >= 0 ? message.shmem_loc : message.loc;
        if (!file.is_open() || !file.seekg(loc) || !file.read(reinterpret_cast<char*>(m_tag.data.data()), static_cast<std::streamsize>(size))) {
            file.clear();
            return {AcqErrorCode::DeviceIo, std::format("cannot read tag {} of the measurement file", message.kind)};
        }
        big_to_native(message.type, m_tag.data);
        return {};
    }

    AcqError copy_shared(const DataMessage& message, std::size_t size)
    {
        if (m_shm == nullptr) {
            const std::string key_file = m_dacq.root + "/shmem/data_server";
            const key_t key = ::ftok(key_file.c_str(), 'A');
            const int id = key == -1 ? -1 : ::shmget(key, kShmBlocks * sizeof(ShmBlock), 0);
            void* memory = id == -1 ? nullptr : ::shmat(id, nullptr, 0);              // writable: the done flags tell the server a block is read
            if (memory == nullptr || reinterpret_cast<std::intptr_t>(memory) == -1) { // shmat fails with (void*)-1
                return system_error("cannot attach the Neuromag shared memory");
            }
            m_shm = static_cast<ShmBlock*>(memory);
        }
        if (static_cast<std::size_t>(message.shmem_buf) >= kShmBlocks || size > kShmData) {
            return {AcqErrorCode::DeviceIo, std::format("the Neuromag data server named shared block {} of {} bytes", message.shmem_buf, size)};
        }
        ShmBlock& block = m_shm[message.shmem_buf];
        std::memcpy(m_tag.data.data(), block.data, size);
        for (ShmClient& client : block.clients) {
            if (client.client_id == m_dacq.client) {
                client.done = 1; // the server may reuse the block
            }
        }
        return {};
    }

    NeuromagDacq m_dacq;
    std::string m_path; // of the client socket
    Collector m_collector;
    bool m_measuring = false;
    int m_socket = -1;
    ShmBlock* m_shm = nullptr;
    std::ifstream m_file;       // the measurement file being written
    std::ifstream m_shmem_file; // the server's overflow file of the shared memory
    NeuromagTag m_tag;          // the last tag
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::unique_ptr<NeuromagTransport>> mne::detail::make_neuromag_client(const NeuromagDacq& dacq)
{
    if (sizeof(sockaddr_un::sun_path) <= dacq.root.size() + std::string_view("/sockets/dacq_client_000000").size()) {
        return AcqError(AcqErrorCode::InvalidParameter, "root=" + dacq.root + ": too long for a socket path");
    }
    return std::unique_ptr<NeuromagTransport>(std::make_unique<NeuromagClient>(dacq));
}
