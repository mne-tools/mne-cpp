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

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include <QSharedPointer>

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
    FiffStreamThread(qint32 id, int socketDescriptor, QObject *parent);

    ~FiffStreamThread();

    void run();

    inline qint32 getID();

    inline QString getAlias();

//    void deactivateRawBufferSending();

    void parseCommand(const std::unique_ptr<FIFFLIB::FiffTag>& p_pTag);

    void writeClientId();

//    void sendData(QTcpSocket& p_qTcpSocket);

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    qint32 m_iDataClientId;
    QString m_sDataClientAlias;

    int m_iSocketDescriptor;

    QMutex m_qMutex;
    QByteArray m_qSendBlock;

    bool m_bIsSendingRawBuffer;

    bool m_bIsRunning;

    void startMeas(qint32 ID);

    void stopMeas(qint32 ID);

    void sendMeasurementInfo(qint32 ID, const FIFFLIB::FiffInfo& p_fiffInfo);

    void sendRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData);
    //void readToBuffer1();
//    void readProc(QTcpSocket& p_qTcpSocket);
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
