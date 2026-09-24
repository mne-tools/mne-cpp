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
#include <com/rt_command/command_manager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QTcpServer>

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
class FiffStreamServer : public QTcpServer//, public ICommandParser //OLD remove this
{
    Q_OBJECT

    friend class FiffStreamThread;

public:

    FiffStreamServer(QObject *parent = 0);

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

//public slots: --> in Qt 5 not anymore declared as slot
    void forwardMeasInfo(qint32 ID, const FIFFLIB::FiffInfo& p_fiffInfo);
    void forwardRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData);

signals:
    void requestMeasInfo(qint32 ID);

    void startMeasFiffStreamClient(qint32 ID);
    void stopMeasFiffStreamClient(qint32 ID);

    void remitMeasInfo(qint32 ID, const FIFFLIB::FiffInfo& p_fiffInfo);
    void remitRawBuffer(QSharedPointer<Eigen::MatrixXf>);

    void closeFiffStreamServer();

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    //SLOTS
    //=========================================================================================================
    /**
     * Fiff data client list
     *
     * @param[in] p_command  The connector list command.
     */
    void comClist(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * specifies to which client to send the requested fiff info
     *
     * @param[in] p_command  The select connector command.
     */
    void comMeasinfo(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Starts the Measurement of a specified client
     *
     * @param[in] p_command  The start command.
     */
    void comStart(COMLIB::Command p_command);//comMeas

    //=========================================================================================================
    /**
     * Stops a fiff data client from receiving further data
     *
     * @param[in] p_command  The stop command.
     */
    void comStop(COMLIB::Command p_command);

    //=========================================================================================================
    /**
     * Stops all connectors
     *
     * @param[in] p_command  The stop all command.
     */
    void comStopAll(COMLIB::Command p_command);

    QByteArray parseToId(QString& p_sRawId, qint32& p_iParsedId);

    QMap<qint32, FiffStreamThread*> m_qClientList;
    qint32                          m_iNextClientId;
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
