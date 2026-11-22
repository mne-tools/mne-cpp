//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     backendchannel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     November, 2026
 * @version  dev
 * @brief    Declares the loopback TCP channel between the workbench and its backend processes.
 *
 * A backend listens on 127.0.0.1 on a port the system chooses and publishes that port with a random 128-bit token
 * in a file only the user can read, named after the service in the user's runtime directory. A client reads the
 * file, connects and sends the token as its first line; the backend closes a connection whose first line is not
 * the token and answers the token with the line "hello <service name>", so a client of a stale file that names a
 * port another program reuses fails at once. Then both send newline-terminated lines (one JSON-RPC message each).
 */

#ifndef MNE_ANALYZE_STUDIO_BACKENDCHANNEL_H
#define MNE_ANALYZE_STUDIO_BACKENDCHANNEL_H

#include "studio_core_global.h"

#include <mne/com/tcp_server.h>
#include <mne/com/tcp_socket.h>

#include <QByteArray>
#include <QObject>
#include <QString>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stop_token>
#include <thread>

namespace MNEANALYZESTUDIO
{

/**
 * @param[in] serviceName    Name of the backend service, e.g. mne_analyze_studio.neuro_kernel.
 *
 * @return The file in which the backend publishes its port.
 */
STUDIOCORESHARED_EXPORT QString backendPortFile(const QString& serviceName);

/** What a backend publishes for its clients. */
struct BackendEndpoint
{
    std::uint16_t port = 0; /**< Loopback port the backend listens on. */
    QByteArray token;       /**< Secret a client sends as its first line, in hexadecimal. */
};

/**
 * @param[in] serviceName    Name of the backend service.
 *
 * @return What the backend published, if it published anything.
 */
STUDIOCORESHARED_EXPORT std::optional<BackendEndpoint> publishedBackend(const QString& serviceName);

/**
 * @brief One line-oriented connection to or from a backend; a thread reads it and delivers every line in the
 *        thread of the object.
 */
class STUDIOCORESHARED_EXPORT BackendConnection : public QObject
{
    Q_OBJECT

public:
    explicit BackendConnection(QObject* parent = nullptr);

    ~BackendConnection() override;

    /**
     * Connects to the backend that published its port under @p serviceName, retrying until @p timeout passes
     * while the port is unpublished or refuses, and emits connected() on success. Fails at once if the port
     * answers but not as the service. A connection that is open is closed first.
     *
     * @param[in] serviceName    Name of the backend service.
     * @param[in] timeout        Maximum wait for the backend.
     *
     * @return Whether the connection is open.
     */
    bool connectToService(const QString& serviceName, std::chrono::milliseconds timeout);

    /** @return Whether the connection is open. */
    bool isConnected() const;

    /** @return Why the last connectToService() failed. */
    QString errorString() const;

    /**
     * @param[in] data   Bytes to send, all of them.
     *
     * @return Whether they were sent; a failed send closes the connection.
     */
    bool send(const QByteArray& data);

    /** Closes the connection; nothing happens if there is none. */
    void close();

signals:
    void connected();
    void lineReceived(const QByteArray& line);
    void disconnected();

private:
    friend class BackendServer;

    /**
     * Takes over a connection a backend accepted; once reading, it emits connected() after the client sent
     * @p token as its first line, and disconnected() without reading further if it sent anything else.
     *
     * @param[in] socket         The connection.
     * @param[in] token          The token the client has to send.
     * @param[in] serviceName    Name of the backend service, for the answer to the token.
     */
    BackendConnection(mne::TcpSocket socket, QByteArray token, QString serviceName);

    void startReading(QByteArray pending);
    bool authenticate(QByteArray& pending, const std::stop_token& stop);
    void handleReadFailure();

    mne::TcpSocket m_socket;
    QByteArray m_expectedToken; /**< Set on the backend's side until the client sent it. */
    QString m_serviceName;
    QString m_errorString;
    std::atomic<bool> m_open{false};
    std::jthread m_reader;
};

/**
 * @brief Accepts the connections to a backend on the loopback interface and publishes the port for its clients.
 */
class STUDIOCORESHARED_EXPORT BackendServer : public QObject
{
    Q_OBJECT

public:
    explicit BackendServer(QObject* parent = nullptr);
    ~BackendServer() override;

    /**
     * Listens on 127.0.0.1 on a port the system chooses and publishes it with a new token under @p serviceName.
     *
     * @param[in] serviceName    Name of the backend service.
     *
     * @return Whether the server listens and the port is published.
     */
    bool listen(const QString& serviceName);

    /** @return The port the server listens on, 0 if it does not. */
    std::uint16_t port() const;

signals:
    /** A client connected and sent the token; the server owns @p connection, which is deleted when it closes. */
    void newConnection(MNEANALYZESTUDIO::BackendConnection* connection);

private:
    void stop();

    QString m_serviceName;
    QByteArray m_token;
    std::uint16_t m_port = 0;
    std::jthread m_listener;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_BACKENDCHANNEL_H
