//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     backendchannel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     November, 2026
 * @version  dev
 * @brief    Implements the loopback TCP channel between the workbench and its backend processes.
 */

#include "backendchannel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <span>
#include <utility>

using namespace MNEANALYZESTUDIO;

namespace
{

constexpr std::chrono::milliseconds kPollInterval(100);
constexpr std::chrono::milliseconds kHandshakeTimeout(5000); /**< For the client's token. */
constexpr std::chrono::milliseconds kHelloTimeout(1000);     /**< For the backend's answer to it. */
constexpr qsizetype kMaxHandshakeLine = 256;
constexpr const char* kLoopback = "127.0.0.1";

std::span<const std::byte> bytesOf(const QByteArray& data)
{
    return std::as_bytes(std::span(data.constData(), static_cast<std::size_t>(data.size())));
}

/** @return 128 random bits in hexadecimal. */
QByteArray newToken()
{
    std::random_device device;
    QByteArray bits;
    for (int k = 0; k < 4; ++k) {
        const std::uint32_t word = device();
        bits.append(reinterpret_cast<const char*>(&word), sizeof(word));
    }
    return bits.toHex();
}

/** @return Whether @p a equals @p b, in a time that does not depend on where they differ. */
bool sameSecret(const QByteArray& a, const QByteArray& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    unsigned char difference = 0;
    for (qsizetype k = 0; k < a.size(); ++k) {
        difference |= static_cast<unsigned char>(a[k] ^ b[k]);
    }
    return difference == 0;
}

QByteArray helloLine(const QString& serviceName)
{
    return "hello " + serviceName.toUtf8() + '\n';
}

/**
 * Reads up to the first newline of @p socket within @p timeout.
 *
 * @param[in, out] pending   What arrived; the line and its newline are taken from its front.
 *
 * @return The line without its newline, or nothing if none arrived in time or the line is too long.
 */
std::optional<QByteArray> readLine(mne::TcpSocket& socket, QByteArray& pending, std::chrono::milliseconds timeout,
                                   const std::stop_token& stop = {})
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::array<std::byte, 512> chunk{};
    while (true) {
        const qsizetype end = pending.indexOf('\n');
        if (end >= 0) {
            QByteArray line = pending.left(end);
            pending.remove(0, end + 1);
            return line;
        }
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (pending.size() > kMaxHandshakeLine || left <= std::chrono::milliseconds::zero() || stop.stop_requested()) {
            return std::nullopt;
        }
        const auto read = socket.read_some(chunk, std::min(left, kPollInterval));
        if (read) {
            pending.append(reinterpret_cast<const char*>(chunk.data()), static_cast<qsizetype>(*read));
        } else if (read.error() != std::errc::timed_out) {
            return std::nullopt;
        }
    }
}

} // namespace

QString MNEANALYZESTUDIO::backendPortFile(const QString& serviceName)
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (directory.isEmpty()) {
        directory = QDir::tempPath();
    }
    return QDir(directory).filePath(serviceName + QStringLiteral(".port"));
}

std::optional<BackendEndpoint> MNEANALYZESTUDIO::publishedBackend(const QString& serviceName)
{
    QFile file(backendPortFile(serviceName));
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QList<QByteArray> lines = file.readAll().split('\n');
    if (lines.size() < 2) {
        return std::nullopt;
    }
    bool ok = false;
    const ushort port = lines[0].trimmed().toUShort(&ok);
    const QByteArray token = lines[1].trimmed();
    if (!ok || port == 0 || token.isEmpty()) {
        return std::nullopt;
    }
    return BackendEndpoint{port, token};
}

//=============================================================================================================

BackendConnection::BackendConnection(QObject* parent)
: QObject(parent)
{
}

BackendConnection::BackendConnection(mne::TcpSocket socket, QByteArray token, QString serviceName)
: m_socket(std::move(socket))
, m_expectedToken(std::move(token))
, m_serviceName(std::move(serviceName))
{
}

BackendConnection::~BackendConnection()
{
    m_open = false;
    m_reader.request_stop();
    if (m_reader.joinable()) {
        m_reader.join();
    }
    m_socket.close();
}

bool BackendConnection::connectToService(const QString& serviceName, std::chrono::milliseconds timeout)
{
    close();
    m_errorString.clear();

    const QString portFile = backendPortFile(serviceName);
    QString reason = QStringLiteral("%1 has not published its port in %2.").arg(serviceName, portFile);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (remaining <= std::chrono::milliseconds::zero()) {
            m_errorString = reason;
            return false;
        }
        if (const std::optional<BackendEndpoint> endpoint = publishedBackend(serviceName)) {
            auto socket = mne::TcpSocket::connect(kLoopback, endpoint->port, remaining);
            if (socket) {
                QByteArray pending;
                const std::optional<QByteArray> hello = !socket->write(bytesOf(endpoint->token + '\n'), std::min(remaining, kHelloTimeout))
                    ? readLine(*socket, pending, std::min(remaining, kHelloTimeout))
                    : std::nullopt;
                if (!hello || *hello + '\n' != helloLine(serviceName)) {
                    // Something else answers on the port: the file outlived its backend
                    m_errorString = QStringLiteral("Port %1 named in %2 is not %3 (it did not answer the handshake); "
                                                   "the file is stale.")
                                        .arg(endpoint->port)
                                        .arg(portFile, serviceName);
                    return false;
                }
                m_socket = std::move(*socket);
                m_open = true;
                startReading(pending);
                emit connected();
                return true;
            }
            reason = QStringLiteral("Nothing answers on port %1 named in %2: %3 is not running and the file is stale, "
                                    "or it is still starting.")
                         .arg(endpoint->port)
                         .arg(portFile, serviceName);
        }
        std::this_thread::sleep_for(std::min(std::chrono::milliseconds(50), remaining));
    }
}

bool BackendConnection::isConnected() const
{
    return m_open;
}

QString BackendConnection::errorString() const
{
    return m_errorString;
}

bool BackendConnection::send(const QByteArray& data)
{
    if (!m_open) {
        return false;
    }
    if (m_socket.write(bytesOf(data))) {
        close();
        return false;
    }
    return true;
}

void BackendConnection::close()
{
    const bool wasOpen = m_open.exchange(false);
    m_reader.request_stop();
    if (m_reader.joinable()) {
        m_reader.join();
    }
    m_socket.close();
    if (wasOpen) {
        emit disconnected();
    }
}

bool BackendConnection::authenticate(QByteArray& pending, const std::stop_token& stop)
{
    // Nothing past the first line is read unless it is the token
    const std::optional<QByteArray> line = readLine(m_socket, pending, kHandshakeTimeout, stop);
    if (!line || !sameSecret(line->trimmed(), m_expectedToken) || m_socket.write(bytesOf(helloLine(m_serviceName)), kHelloTimeout)) {
        return false;
    }
    m_open = true;
    QMetaObject::invokeMethod(this, [this]() { emit connected(); }, Qt::QueuedConnection);
    return true;
}

void BackendConnection::startReading(QByteArray pending)
{
    m_reader = std::jthread([this, pending = std::move(pending)](const std::stop_token& stop) mutable {
        if (!m_expectedToken.isEmpty() && !authenticate(pending, stop)) {
            QMetaObject::invokeMethod(this, [this]() { handleReadFailure(); }, Qt::QueuedConnection);
            return;
        }
        std::array<std::byte, 4096> chunk{};
        while (!stop.stop_requested()) {
            qsizetype end = -1;
            while ((end = pending.indexOf('\n')) >= 0) {
                QByteArray line = pending.left(end + 1);
                pending.remove(0, end + 1);
                QMetaObject::invokeMethod(this, [this, line = std::move(line)]() { emit lineReceived(line); }, Qt::QueuedConnection);
            }
            const auto read = m_socket.read_some(chunk, kPollInterval);
            if (!read) {
                if (read.error() == std::errc::timed_out) {
                    continue;
                }
                QMetaObject::invokeMethod(this, [this]() { handleReadFailure(); }, Qt::QueuedConnection);
                return;
            }
            pending.append(reinterpret_cast<const char*>(chunk.data()), static_cast<qsizetype>(*read));
        }
    });
}

void BackendConnection::handleReadFailure()
{
    // A connection close() ended already is not reported twice
    if (!m_socket.is_open()) {
        return;
    }
    m_open = false;
    m_reader.request_stop();
    if (m_reader.joinable()) {
        m_reader.join();
    }
    m_socket.close();
    emit disconnected();
}

//=============================================================================================================

BackendServer::BackendServer(QObject* parent)
: QObject(parent)
{
}

BackendServer::~BackendServer()
{
    stop();
}

bool BackendServer::listen(const QString& serviceName)
{
    stop();

    auto server = mne::TcpServer::listen(kLoopback, 0);
    if (!server) {
        return false;
    }
    const std::uint16_t port = server->port();
    const QByteArray token = newToken();

    // The token makes the file a secret: only the user may read it, also while it is written
    const QString portFile = backendPortFile(serviceName);
    QDir().mkpath(QFileInfo(portFile).absolutePath());
    QSaveFile file(portFile);
    if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        return false;
    }
    file.write(QByteArray::number(port) + '\n' + token + '\n');
    if (!file.commit() || !QFile::setPermissions(portFile, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        return false;
    }

    m_serviceName = serviceName;
    m_token = token;
    m_port = port;
    // The handshake runs off this thread, so a client in this very thread (a test) can wait for it
    QThread* home = thread();
    m_listener = std::jthread([this, home, server = std::move(*server)](const std::stop_token& stop) mutable {
        while (!stop.stop_requested()) {
            auto client = server.accept(kPollInterval);
            if (!client) {
                if (client.error() != std::errc::timed_out) {
                    std::this_thread::sleep_for(kPollInterval);
                }
                continue;
            }
            auto* connection = new BackendConnection(std::move(*client), m_token, m_serviceName);
            connection->moveToThread(home);
            // Queued before the reader can queue connected(), so the server sees it
            QMetaObject::invokeMethod(
                connection,
                [server = QPointer<BackendServer>(this), connection]() {
                    if (!server) {
                        connection->deleteLater();
                        return;
                    }
                    connection->setParent(server);
                    connect(connection, &BackendConnection::disconnected, connection, &QObject::deleteLater);
                    connect(connection, &BackendConnection::connected, server, [server, connection]() { emit server->newConnection(connection); });
                },
                Qt::QueuedConnection);
            connection->startReading(QByteArray());
        }
    });
    return true;
}

std::uint16_t BackendServer::port() const
{
    return m_port;
}

void BackendServer::stop()
{
    m_listener.request_stop();
    if (m_listener.joinable()) {
        m_listener.join();
    }
    // Another instance of the service may have published its port since; leave that one in place.
    if (!m_serviceName.isEmpty()) {
        const std::optional<BackendEndpoint> published = publishedBackend(m_serviceName);
        if (published && published->token == m_token) {
            QFile::remove(backendPortFile(m_serviceName));
        }
    }
    m_serviceName.clear();
    m_token.clear();
    m_port = 0;
}
