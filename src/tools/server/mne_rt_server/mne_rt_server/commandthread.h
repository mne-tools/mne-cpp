//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandthread.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandThread Class.
 */

#ifndef COMMANDTHREAD_H
#define COMMANDTHREAD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/com/tcp_socket.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

class CommandThread : public QThread
{
    Q_OBJECT

public:
    CommandThread(mne::TcpSocket socket, qint32 p_iId, QObject* parent);

    ~CommandThread();

    void attachCommandReply(QString p_blockReply, qint32 p_iID);

    void run();

signals:
    void newCommand(QString p_sCommand, qint32 p_iThreadID);

private:
    mne::TcpSocket m_socket;

    std::atomic<bool> m_bIsRunning;
    qint32 m_iThreadID;

    QMutex m_qMutex;
    QString m_qSendData;
};
} // NAMESPACE

#endif //COMMANDTHREAD_H
