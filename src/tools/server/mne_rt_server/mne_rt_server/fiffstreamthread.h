//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamthread.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffStreamThread Class.
 */

#ifndef FIFFSTREAMTHREAD_H
#define FIFFSTREAMTHREAD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>

#include <mne/com/tcp_socket.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamThread : public QThread
{
    Q_OBJECT

public:
    FiffStreamThread(qint32 id, mne::TcpSocket socket, QObject* parent);

    ~FiffStreamThread();

    void run();

    inline qint32 getID();

    inline QString getAlias();

    //    void deactivateRawBufferSending();

    void parseCommand(const mne::FiffTag& p_pTag);

    void writeClientId();


private:
    qint32 m_iDataClientId;
    QString m_sDataClientAlias;

    mne::TcpSocket m_socket;

    QMutex m_qMutex;
    QByteArray m_qSendBlock;

    bool m_bIsSendingRawBuffer;

    std::atomic<bool> m_bIsRunning;

    void startMeas(qint32 ID);

    void stopMeas(qint32 ID);

    void sendMeasurementInfo(qint32 ID, const mne::FiffInfo& p_fiffInfo);

    void sendRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData);
    //void readToBuffer1();
};

inline qint32 FiffStreamThread::getID()
{
    return m_iDataClientId;
}

inline QString FiffStreamThread::getAlias()
{
    return m_sDataClientAlias;
}
} // NAMESPACE

#endif //FIFFSTREAMTHREAD_H
