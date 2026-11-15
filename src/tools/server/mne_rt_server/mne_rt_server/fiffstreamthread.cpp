//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamthread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffStreamThread Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"

#include <cstring>
#include "fiffstreamserver.h"
#include "mne_rt_commands.h"

//=============================================================================================================
// Fiff INCLUDES
//=============================================================================================================

#include <fiff/fiff_byte_swap.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtEndian>
#include <QtNetwork>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace mne;

namespace
{

/** Appends the tags @p write puts on a FIFF stream to @p block. */
template<typename Write>
void appendTags(QByteArray& block, Write&& write)
{
    auto bytes = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *bytes;
    FiffStream stream(std::move(bytes));
    write(stream);
    block.append(reinterpret_cast<const char*>(written.data().data()), static_cast<qsizetype>(written.size()));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamThread::FiffStreamThread(qint32 id, int socketDescriptor, QObject* parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, m_iSocketDescriptor(socketDescriptor)
, m_bIsSendingRawBuffer(false)
, m_bIsRunning(false)
{
}

//=============================================================================================================

FiffStreamThread::~FiffStreamThread()
{
    //Remove from client list
    FiffStreamServer* t_pFiffStreamServer = qobject_cast<FiffStreamServer*>(this->parent());
    if (t_pFiffStreamServer)
        t_pFiffStreamServer->m_qClientList.remove(m_iDataClientId);

    m_bIsRunning = false;
    QThread::wait();
}

//=============================================================================================================

void FiffStreamThread::startMeas(qint32 ID)
{
    if (ID == m_iDataClientId) {
        qDebug() << "Activate raw buffer sending.";

        m_qMutex.lock();
        appendTags(m_qSendBlock, [](FiffStream& out) { out.start_block(FIFFB_RAW_DATA); });
        m_bIsSendingRawBuffer = true;
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void FiffStreamThread::stopMeas(qint32 ID)
{
    qDebug() << "void FiffStreamThread::stopMeas(qint32 ID)";
    if (ID == m_iDataClientId || ID == -1) {
        qDebug() << "stop raw buffer sending.";

        m_qMutex.lock();
        appendTags(m_qSendBlock, [](FiffStream& out) { out.end_block(FIFFB_RAW_DATA); });
        m_bIsSendingRawBuffer = false;
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void FiffStreamThread::parseCommand(const FiffTag& p_pTag)
{
    if (p_pTag.size() >= 4) {
        qint32 t_iCmd = 0;
        std::memcpy(&t_iCmd, p_pTag.data(), sizeof(t_iCmd));
        t_iCmd = mne::swap_int(t_iCmd);

        if (t_iCmd == MNE_RT_SET_CLIENT_ALIAS) {
            //
            // Set Client Alias
            //
            m_sDataClientAlias = QString::fromUtf8(reinterpret_cast<const char*>(p_pTag.data()) + 4, static_cast<qsizetype>(p_pTag.size() - 4));
            qInfo("FiffStreamClient (ID %d): new alias = '%s'\r\n", m_iDataClientId, m_sDataClientAlias.toUtf8().constData());
        } else if (t_iCmd == MNE_RT_GET_CLIENT_ID) {
            //
            // Send Client ID
            //
            qInfo("FiffStreamClient (ID %d): send client ID %d\r\n", m_iDataClientId, m_iDataClientId);
            writeClientId();
        } else {
            qInfo("FiffStreamClient (ID %d): unknown command\r\n", m_iDataClientId);
        }
    } else {
        qInfo("FiffStreamClient (ID %d): unknown command\r\n", m_iDataClientId);
    }
}

//=============================================================================================================

void FiffStreamThread::sendRawBuffer(QSharedPointer<Eigen::MatrixXf> m_pMatRawData)
{
    if (m_bIsSendingRawBuffer) {
        QMutexLocker locker(&m_qMutex);
        appendTags(m_qSendBlock, [&m_pMatRawData](FiffStream& out) {
            out.write_float(FIFF_DATA_BUFFER, m_pMatRawData->data(), m_pMatRawData->rows() * m_pMatRawData->cols());
        });
    }
}

//=============================================================================================================

void FiffStreamThread::sendMeasurementInfo(qint32 ID, const FiffInfo& p_fiffInfo)
{
    if (ID == m_iDataClientId) {
        QMutexLocker locker(&m_qMutex);
        appendTags(m_qSendBlock, [&p_fiffInfo](FiffStream& out) { p_fiffInfo.writeToStream(out); });
    }
}

//=============================================================================================================

void FiffStreamThread::writeClientId()
{
    QMutexLocker locker(&m_qMutex);
    appendTags(m_qSendBlock, [this](FiffStream& out) { out.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId); });
}

//=============================================================================================================

void FiffStreamThread::run()
{
    m_bIsRunning = true;

    FiffStreamServer* t_pParentServer = qobject_cast<FiffStreamServer*>(this->parent());

    connect(t_pParentServer, &FiffStreamServer::remitMeasInfo,
            this, &FiffStreamThread::sendMeasurementInfo);
    connect(t_pParentServer, &FiffStreamServer::remitRawBuffer,
            this, &FiffStreamThread::sendRawBuffer);
    connect(t_pParentServer, &FiffStreamServer::startMeasFiffStreamClient,
            this, &FiffStreamThread::startMeas);
    connect(t_pParentServer, &FiffStreamServer::stopMeasFiffStreamClient,
            this, &FiffStreamThread::stopMeas);

    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(m_iSocketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    } else {
        qInfo("FiffStreamClient (assigned ID %d) accepted from\n\tIP:\t%s\n\tPort:\t%d\n",
              m_iDataClientId,
              QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
              t_qTcpSocket.peerPort());
    }

    while (t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning) {
        //
        // Write available data
        //
        QByteArray t_qBlock;
        m_qMutex.lock();
        t_qBlock.swap(m_qSendBlock);
        m_qMutex.unlock();
        // The socket buffers the block and flushes it in waitForReadyRead; blocking here would stall the producers.
        if (!t_qBlock.isEmpty()) {
            t_qTcpSocket.write(t_qBlock);
        }

        // Read: wait 10 ms for a tag header (kind, type, size, next), then for its data
        t_qTcpSocket.waitForReadyRead(10);
        constexpr qint64 kHeaderBytes = 4 * sizeof(qint32);
        if (t_qTcpSocket.bytesAvailable() < kHeaderBytes) {
            continue;
        }
        QByteArray tagBytes = t_qTcpSocket.read(kHeaderBytes);
        const qint32 size = qFromBigEndian<qint32>(tagBytes.constData() + 2 * sizeof(qint32));
        while (t_qTcpSocket.bytesAvailable() < size && t_qTcpSocket.state() == QAbstractSocket::ConnectedState) {
            t_qTcpSocket.waitForReadyRead(10);
        }
        tagBytes += t_qTcpSocket.read(size);
        const auto* begin = reinterpret_cast<const std::byte*>(tagBytes.constData());
        FiffStream in(std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(begin, begin + tagBytes.size())));
        if (const std::optional<FiffTag> tag = in.read_tag(); tag && tag->kind == FIFF_MNE_RT_COMMAND) {
            parseCommand(*tag);
        }
    }

    t_qTcpSocket.disconnectFromHost();
    if (t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
