//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_com.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests of the transports of mne_com against loopback peers and a pseudo-terminal.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <com/serial_port.h>
#include <com/tcp_server.h>
#include <com/tcp_socket.h>
#include <com/udp_socket.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

#ifndef _WIN32
/** A listening loopback socket that accepts one peer. */
class Listener
{
public:
    Listener()
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in tcp{};
        tcp.sin_family = AF_INET;
        tcp.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t length = sizeof(tcp);
        if (::bind(m_fd, reinterpret_cast<sockaddr*>(&tcp), sizeof(tcp)) == 0 && ::listen(m_fd, 1) == 0 &&
            ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&tcp), &length) == 0) {
            m_port = ntohs(tcp.sin_port);
        }
    }

    ~Listener()
    {
        ::close(m_fd);
    }

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    [[nodiscard]] std::uint16_t port() const
    {
        return m_port;
    }

    /** @return The accepted peer's descriptor. */
    [[nodiscard]] int accept() const
    {
        return ::accept(m_fd, nullptr, nullptr);
    }

private:
    int m_fd = -1;
    std::uint16_t m_port = 0;
};

std::span<const std::byte> bytes(std::string_view text)
{
    return std::as_bytes(std::span(text));
}

std::string text(std::span<const std::byte> data)
{
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}
#endif

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestCom
 */
class TestCom : public QObject
{
    Q_OBJECT

private slots:
    void tcpExchangesBytes();
    void tcpReadTimesOutWithoutConsuming();
    void tcpReportsAHalfAnswerAndAClosedPeer();
    void tcpConnectFails();
    void tcpServerAcceptsAndReadsWhatArrived();
    void tcpWriteTimesOutWhenThePeerDoesNotRead();
    void udpReceivesDatagrams();
    void udpSharesAMulticastGroup();
    void udpBindFails();
    void udpAnswersTheSender();
    void udpSharesAPortAndBroadcasts();
    void serialExchangesBytes();
    void serialOpenFails();
    void serialDrainsOnClose();
    void socketsMove();
};

//=============================================================================================================

void TestCom::tcpExchangesBytes()
{
#ifdef _WIN32
    QSKIP("the peer uses POSIX sockets");
#else
    Listener listener;
    QVERIFY(listener.port() > 0);
    auto socket = TcpSocket::connect("127.0.0.1", listener.port(), 1s);
    QVERIFY(socket);
    const int peer = listener.accept();
    QVERIFY(peer >= 0);

    QVERIFY(!socket->write(bytes("ping")));
    std::array<char, 4> got{};
    QCOMPARE(::recv(peer, got.data(), got.size(), MSG_WAITALL), ssize_t(4));
    QCOMPARE(std::string(got.data(), 4), std::string("ping"));

    // Two sends arrive as one read of their total size
    ::send(peer, "po", 2, 0);
    std::thread later([peer] {
        std::this_thread::sleep_for(20ms);
        ::send(peer, "ng", 2, 0);
    });
    std::array<std::byte, 4> answer{};
    QVERIFY(!socket->read(answer, 1s));
    later.join();
    QCOMPARE(text(answer), std::string("pong"));
    ::close(peer);
#endif
}

//=============================================================================================================

void TestCom::tcpReadTimesOutWithoutConsuming()
{
#ifdef _WIN32
    QSKIP("the peer uses POSIX sockets");
#else
    Listener listener;
    auto socket = TcpSocket::connect("127.0.0.1", listener.port(), 1s);
    QVERIFY(socket);
    const int peer = listener.accept();
    std::array<std::byte, 2> answer{};
    QVERIFY(socket->read(answer, 20ms) == std::errc::timed_out);
    ::send(peer, "ok", 2, 0);
    QVERIFY(!socket->read(answer, 1s));
    QCOMPARE(text(answer), std::string("ok"));
    ::close(peer);
#endif
}

//=============================================================================================================

void TestCom::tcpReportsAHalfAnswerAndAClosedPeer()
{
#ifdef _WIN32
    QSKIP("the peer uses POSIX sockets");
#else
    Listener listener;
    auto socket = TcpSocket::connect("127.0.0.1", listener.port(), 1s);
    QVERIFY(socket);
    const int peer = listener.accept();
    ::send(peer, "ab", 2, 0);
    std::array<std::byte, 4> answer{};
    QVERIFY(socket->read(answer, 50ms) == std::errc::io_error);
    ::close(peer);
    QVERIFY(socket->read(answer, 1s) == std::errc::connection_reset);
#endif
}

//=============================================================================================================

void TestCom::tcpConnectFails()
{
    auto refused = TcpSocket::connect("127.0.0.1", 1, 5s); // Windows retries a refused connection for about 2 s
    QVERIFY(!refused);
    QVERIFY(refused.error() == std::errc::connection_refused);
    auto unknown = TcpSocket::connect("no-such-host.invalid", 4217, 1s);
    QVERIFY(!unknown);
    QVERIFY(unknown.error() == std::errc::host_unreachable);
}

//=============================================================================================================

void TestCom::tcpServerAcceptsAndReadsWhatArrived()
{
    auto server = TcpServer::listen("127.0.0.1", 0);
    QVERIFY2(server, server ? "" : server.error().message().c_str());
    QVERIFY(server->port() > 0);
    QVERIFY(server->accept(0ms).error() == std::errc::timed_out);
    QVERIFY(TcpServer::listen("localhost", 0).error() == std::errc::invalid_argument);

    auto client = TcpSocket::connect("127.0.0.1", server->port(), 1s);
    QVERIFY(client);
    auto peer = server->accept(1s);
    QVERIFY2(peer, peer ? "" : peer.error().message().c_str());
    QVERIFY(peer->is_open());

    // read_some takes what has arrived, not a fixed size
    const std::string_view hello = "hello";
    QVERIFY(!client->write(std::as_bytes(std::span(hello))));
    std::array<std::byte, 64> buffer{};
    std::size_t total = 0;
    while (total < hello.size()) {
        const auto got = peer->read_some(std::span(buffer).subspan(total), 1s);
        QVERIFY2(got, got ? "" : got.error().message().c_str());
        total += *got;
    }
    QCOMPARE(total, hello.size());
    QCOMPARE(std::string(reinterpret_cast<const char*>(buffer.data()), total), std::string(hello));
    QVERIFY(peer->read_some(buffer, 0ms).error() == std::errc::timed_out);

    // Both directions; a closed peer is reported, a closed socket is no longer open
    QVERIFY(!peer->write(std::as_bytes(std::span(hello))));
    std::array<std::byte, 5> answer{};
    QVERIFY(!client->read(answer, 1s));
    client->close();
    QVERIFY(!client->is_open());
    QVERIFY(peer->read_some(buffer, 1s).error() == std::errc::connection_reset);
}

//=============================================================================================================

void TestCom::tcpWriteTimesOutWhenThePeerDoesNotRead()
{
    auto server = TcpServer::listen("127.0.0.1", 0);
    QVERIFY(server);
    auto client = TcpSocket::connect("127.0.0.1", server->port(), 1s);
    QVERIFY(client);
    auto peer = server->accept(1s);
    QVERIFY(peer);

    // More than the socket buffers of both ends hold; the peer never reads
    const std::vector<std::byte> data(std::size_t{64} << 20);
    const auto start = std::chrono::steady_clock::now();
    QVERIFY(client->write(data, 200ms) == std::errc::timed_out);
    const auto took = std::chrono::steady_clock::now() - start;
    QVERIFY2(took >= 150ms && took < 5s, qPrintable(QString::number(std::chrono::duration_cast<std::chrono::milliseconds>(took).count())));

    // What fits is still sent at once
    std::array<std::byte, 4> small{};
    auto reader = TcpSocket::connect("127.0.0.1", server->port(), 1s);
    QVERIFY(reader);
    auto other = server->accept(1s);
    QVERIFY(other);
    QVERIFY(!reader->write(small, 0ms));
    QVERIFY(!other->read(small, 1s));
}

//=============================================================================================================

void TestCom::udpReceivesDatagrams()
{
#ifdef _WIN32
    QSKIP("the sender uses POSIX sockets");
#else
    auto socket = UdpSocket::bind("127.0.0.1", 0);
    QVERIFY(socket);
    QVERIFY(socket->port() > 0);
    std::array<std::byte, 8> buffer{};
    QVERIFY(socket->receive(buffer, 20ms).error() == std::errc::timed_out);

    const int sender = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(socket->port());
    ::inet_pton(AF_INET, "127.0.0.1", &to.sin_addr);
    for (const std::string_view datagram : {std::string_view("one"), std::string_view("a longer datagram")}) {
        ::sendto(sender, datagram.data(), datagram.size(), 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
    }
    ::close(sender);

    // Datagrams keep their bounds; a longer one is cut to the buffer
    auto size = socket->receive(buffer, 1s);
    QVERIFY(size);
    QCOMPARE(*size, std::size_t(3));
    QCOMPARE(text(std::span(buffer).first(3)), std::string("one"));
    size = socket->receive(buffer, 1s);
    QVERIFY(size);
    QCOMPARE(*size, buffer.size());
    QCOMPARE(text(buffer), std::string("a longer"));
#endif
}

//=============================================================================================================

void TestCom::udpSharesAMulticastGroup()
{
    // Two members of a group on one port both receive what a sender on this machine sends to the group
    const std::string group = "239.255.43.21";
    auto first = UdpSocket::bind(group, 0);
    QVERIFY2(first, first ? "" : first.error().message().c_str());
    auto second = UdpSocket::bind(group, first->port());
    QVERIFY2(second, second ? "" : second.error().message().c_str());
    auto sender = UdpSocket::bind("0.0.0.0", 0);
    QVERIFY(sender);
    const std::array datagram = {std::byte{1}, std::byte{2}, std::byte{3}};
    const std::error_code sent = sender->send_to(datagram, group, first->port());
    if (sent == std::errc::network_unreachable || sent == std::errc::host_unreachable || sent == std::errc::address_not_available) {
        QSKIP("this host has no route for multicast");
    }
    QVERIFY2(!sent, sent.message().c_str());
    std::array<std::byte, 8> buffer{};
    for (UdpSocket* member : {&*first, &*second}) {
        const auto size = member->receive(buffer, 1s);
        QVERIFY2(size, size ? "" : size.error().message().c_str());
        QCOMPARE(*size, datagram.size());
        QVERIFY(std::equal(datagram.begin(), datagram.end(), buffer.begin()));
    }

    // Unicast too, and a destination that is no address
    auto plain = UdpSocket::bind("127.0.0.1", 0);
    QVERIFY(plain);
    QVERIFY(!sender->send_to(datagram, "127.0.0.1", plain->port()));
    QVERIFY(plain->receive(buffer, 1s));
    QVERIFY(sender->send_to(datagram, "localhost", plain->port()) == std::errc::invalid_argument);
}

//=============================================================================================================

void TestCom::udpBindFails()
{
    QVERIFY(UdpSocket::bind("localhost", 0).error() == std::errc::invalid_argument); // an address, not a name
    auto taken = UdpSocket::bind("127.0.0.1", 0);
    QVERIFY(taken);
    QVERIFY(UdpSocket::bind("127.0.0.1", taken->port()).error() == std::errc::address_in_use);
}

//=============================================================================================================

void TestCom::udpAnswersTheSender()
{
    auto server = UdpSocket::bind("127.0.0.1", 0);
    auto client = UdpSocket::bind("127.0.0.1", 0);
    QVERIFY(server && client);
    const std::array query = {std::byte{'?'}};
    QVERIFY(!client->send_to(query, "127.0.0.1", server->port()));
    std::array<std::byte, 8> buffer{};
    const auto datagram = server->receive_from(buffer, 1s);
    QVERIFY2(datagram, datagram ? "" : datagram.error().message().c_str());
    QCOMPARE(datagram->size, std::size_t(1));
    QCOMPARE(datagram->address, std::string("127.0.0.1"));
    QCOMPARE(datagram->port, client->port());

    const std::array reply = {std::byte{'!'}, std::byte{'!'}};
    QVERIFY(!server->send_to(reply, datagram->address, datagram->port));
    const auto size = client->receive(buffer, 1s);
    QVERIFY(size);
    QCOMPARE(*size, reply.size());
    QVERIFY(server->receive_from(buffer, 0ms).error() == std::errc::timed_out);
}

//=============================================================================================================

void TestCom::udpSharesAPortAndBroadcasts()
{
    // Services on one machine share a port they all listen on, each a member of the group
    const std::string group = "239.255.43.22";
    auto first = UdpSocket::bind("0.0.0.0", 0, true);
    QVERIFY2(first, first ? "" : first.error().message().c_str());
    auto second = UdpSocket::bind("0.0.0.0", first->port(), true);
    QVERIFY2(second, second ? "" : second.error().message().c_str());
    QVERIFY(UdpSocket::bind("0.0.0.0", first->port()).error() == std::errc::address_in_use);
    for (UdpSocket* member : {&*first, &*second}) {
        QVERIFY(!member->join_group(group));
    }
    QVERIFY(first->join_group("10.0.0.1") == std::errc::invalid_argument); // no group
    QVERIFY(first->join_group(group, "nowhere") == std::errc::invalid_argument);

    auto sender = UdpSocket::bind("0.0.0.0", 0);
    QVERIFY(sender);
    QVERIFY(!sender->set_multicast_interface());
    QVERIFY(sender->set_multicast_interface("nowhere") == std::errc::invalid_argument);
    const std::array datagram = {std::byte{4}, std::byte{2}};
    const std::error_code sent = sender->send_to(datagram, group, first->port());
    if (sent == std::errc::network_unreachable || sent == std::errc::host_unreachable || sent == std::errc::address_not_available) {
        QSKIP("this host has no route for multicast");
    }
    QVERIFY2(!sent, sent.message().c_str());
    std::array<std::byte, 8> buffer{};
    for (UdpSocket* member : {&*first, &*second}) {
        const auto got = member->receive_from(buffer, 1s);
        QVERIFY2(got, got ? "" : got.error().message().c_str());
        QCOMPARE(got->size, datagram.size());
        QVERIFY(!got->address.empty()); // the port may differ where a network filter relays multicast
    }

    // Broadcasting is allowed
    QVERIFY(!sender->send_to(datagram, "255.255.255.255", first->port()));
}

//=============================================================================================================

void TestCom::serialExchangesBytes()
{
#ifdef _WIN32
    QSKIP("the device end uses a POSIX pseudo-terminal");
#else
    const int device = ::posix_openpt(O_RDWR | O_NOCTTY);
    QVERIFY(device >= 0 && ::grantpt(device) == 0 && ::unlockpt(device) == 0);
    auto port = SerialPort::open(::ptsname(device), 115200);
    QVERIFY2(port, port ? "" : port.error().message().c_str());
    QVERIFY(port->is_open());

    std::array<char, 16> buffer{};
    QVERIFY(port->read(buffer, 20ms).error() == std::errc::timed_out);
    QVERIFY(!port->write("C\r"));
    std::array<char, 2> command{};
    QCOMPARE(::read(device, command.data(), command.size()), ssize_t(2));
    QCOMPARE(std::string(command.data(), 2), std::string("C\r"));

    QCOMPARE(::write(device, "01 1.0\r\n", 8), ssize_t(8));
    auto got = port->read(buffer, 1s);
    QVERIFY(got);
    QCOMPARE(std::string(buffer.data(), *got), std::string("01 1.0\r\n"));
    ::close(device);
#endif
}

//=============================================================================================================

void TestCom::serialOpenFails()
{
    QVERIFY(SerialPort::open("/no/such/port", 9600).error() == std::errc::no_such_file_or_directory);
#ifndef _WIN32
    QVERIFY(SerialPort::open("/dev/null", 12345).error() == std::errc::invalid_argument);
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "test_com_not_a_port";
    std::ofstream(file) << "text";
    QVERIFY(SerialPort::open(file.string(), 9600).error() == std::errc::inappropriate_io_control_operation);
    std::filesystem::remove(file);
#endif
}

//=============================================================================================================

void TestCom::serialDrainsOnClose()
{
#ifdef _WIN32
    QSKIP("the device end uses a POSIX pseudo-terminal");
#else
    // The last command, e.g. the one that stops the device, must reach it although the port closes right after
    const int device = ::posix_openpt(O_RDWR | O_NOCTTY);
    QVERIFY(device >= 0 && ::grantpt(device) == 0 && ::unlockpt(device) == 0);
    char command = 0;
    ssize_t got = 0;
    std::thread reader;
    {
        auto port = SerialPort::open(::ptsname(device), 9600);
        QVERIFY(port);
        QVERIFY(!port->write("P"));
        reader = std::thread([&] { got = ::read(device, &command, 1); }); // a pseudo-terminal drains as it is read
    }
    reader.join();
    QCOMPARE(got, ssize_t(1));
    QCOMPARE(command, 'P');
    ::close(device);
#endif
}

//=============================================================================================================

void TestCom::socketsMove()
{
    auto first = UdpSocket::bind("127.0.0.1", 0);
    QVERIFY(first);
    const std::uint16_t port = first->port();
    UdpSocket moved = std::move(*first);
    QCOMPARE(moved.port(), port);
    UdpSocket assigned;
    assigned = std::move(moved);
    QCOMPARE(assigned.port(), port);
    // The moved-from socket closed nothing: the port is still taken
    QVERIFY(UdpSocket::bind("127.0.0.1", port).error() == std::errc::address_in_use);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCom)
#include "test_com.moc"
