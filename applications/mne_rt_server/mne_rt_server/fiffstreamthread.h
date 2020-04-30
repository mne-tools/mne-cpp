//=============================================================================================================
/**
 * @file     fiffstreamthread.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Limin Sun, Lorenz Esch, Matti Hamalainen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     Declaration of the FiffStreamThread Class.
 *
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

    void parseCommand(QSharedPointer<FIFFLIB::FiffTag> p_pTag);

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
