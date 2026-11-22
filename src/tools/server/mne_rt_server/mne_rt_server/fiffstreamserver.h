//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamserver.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffStreamServer Class.
 */

#ifndef FIFFSTREAMSERVER_H
#define FIFFSTREAMSERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <rtserver_connector/command_manager.h>

#include <com/tcp_server.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QStringList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <thread>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamThread;

//=============================================================================================================
/**
 * DECLARE CLASS FiffStreamServer
 *
 * @brief The FiffStreamServer class provides
 */
class FiffStreamServer : public QObject //, public ICommandParser //OLD remove this
{
    Q_OBJECT

    friend class FiffStreamThread;

public:
    FiffStreamServer(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the FiffStreamServer.
     */
    ~FiffStreamServer();

    //=========================================================================================================
    /**
     * ToDo...
     */
    inline FiffStreamThread* getClient(qint32 id);

    //=========================================================================================================
    /**
     * connect fiff stream server to mne_rt_server commands
     */
    void connectCommands();

    /**
     * Starts listening for fiff stream clients on all interfaces; a listener thread accepts them and hands every
     * connection to the thread of the server, which starts a FiffStreamThread for it.
     *
     * @param[in] port       Port to listen on.
     * @param[out] error     Why the server does not listen, if it does not.
     *
     * @return Whether the server listens.
     */
    bool listen(std::uint16_t port, QString& error);

    /**
     * Returns the port the server listens on, 0 if it does not.
     */
    std::uint16_t serverPort() const;

    //public slots: --> in Qt 5 not anymore declared as slot
    void forwardMeasInfo(qint32 ID, const mne::FiffInfo& p_fiffInfo);
    void forwardRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData);

signals:
    void requestMeasInfo(qint32 ID);

    void startMeasFiffStreamClient(qint32 ID);
    void stopMeasFiffStreamClient(qint32 ID);

    void remitMeasInfo(qint32 ID, const mne::FiffInfo& p_fiffInfo);
    void remitRawBuffer(QSharedPointer<Eigen::MatrixXf>);

    void closeFiffStreamServer();

private:
    void incomingConnection(mne::TcpSocket socket);

    //SLOTS
    //=========================================================================================================
    /**
     * Fiff data client list
     *
     * @param[in] p_command  The connector list command.
     */
    void comClist(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * specifies to which client to send the requested fiff info
     *
     * @param[in] p_command  The select connector command.
     */
    void comMeasinfo(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Starts the Measurement of a specified client
     *
     * @param[in] p_command  The start command.
     */
    void comStart(RTSERVER::Command p_command); //comMeas

    //=========================================================================================================
    /**
     * Stops a fiff data client from receiving further data
     *
     * @param[in] p_command  The stop command.
     */
    void comStop(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
     * Stops all connectors
     *
     * @param[in] p_command  The stop all command.
     */
    void comStopAll(RTSERVER::Command p_command);

    QByteArray parseToId(QString& p_sRawId, qint32& p_iParsedId);

    QMap<qint32, FiffStreamThread*> m_qClientList;
    qint32 m_iNextClientId;

    std::uint16_t m_iPort;   /**< The port the server listens on, 0 if it does not. */
    std::jthread m_listener; /**< Accepts the clients; declared last so it stops first. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

FiffStreamThread* FiffStreamServer::getClient(qint32 id)
{
    return m_qClientList[id];
}
} // NAMESPACE

#endif //FIFFSTREAMSERVER_H
