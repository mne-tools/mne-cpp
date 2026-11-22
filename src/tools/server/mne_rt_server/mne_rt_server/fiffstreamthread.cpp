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

#include <mne/base/log.h>
#include "fiffstreamthread.h"

#include "fiffstreamserver.h"
#include "mne_rt_commands.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <span>
#include <system_error>
#include <utility>

//=============================================================================================================
// Fiff INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_byte_swap.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace mne;

namespace
{

/**
 * Longest wait for a client to take a block. One that does not (it stopped reading) is dropped, so neither the
 * memory of the pending blocks nor the shutdown of the server depends on it.
 */
constexpr std::chrono::milliseconds kWriteTimeout(2000);

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

FiffStreamThread::FiffStreamThread(qint32 id, mne::TcpSocket socket, QObject* parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, m_socket(std::move(socket))
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
        Log::debug("Activate raw buffer sending.");

        m_qMutex.lock();
        appendTags(m_qSendBlock, [](FiffStream& out) { out.start_block(FIFFB_RAW_DATA); });
        m_bIsSendingRawBuffer = true;
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void FiffStreamThread::stopMeas(qint32 ID)
{
    Log::debug("void FiffStreamThread::stopMeas(qint32 ID)");
    if (ID == m_iDataClientId || ID == -1) {
        Log::debug("stop raw buffer sending.");

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
            Log::info("FiffStreamClient (ID {}): new alias = '{}'\r\n", m_iDataClientId, m_sDataClientAlias.toStdString());
        } else if (t_iCmd == MNE_RT_GET_CLIENT_ID) {
            //
            // Send Client ID
            //
            Log::info("FiffStreamClient (ID {}): send client ID {}\r\n", m_iDataClientId, m_iDataClientId);
            writeClientId();
        } else {
            Log::info("FiffStreamClient (ID {}): unknown command\r\n", m_iDataClientId);
        }
    } else {
        Log::info("FiffStreamClient (ID {}): unknown command\r\n", m_iDataClientId);
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

    Log::info("FiffStreamClient (assigned ID {}) accepted\n", m_iDataClientId);

    constexpr qsizetype kHeaderBytes = 4 * sizeof(qint32);
    QByteArray t_received;
    std::array<std::byte, 4096> t_chunk{};

    while (m_socket.is_open() && m_bIsRunning) {
        //
        // Write available data
        //
        QByteArray t_qBlock;
        m_qMutex.lock();
        t_qBlock.swap(m_qSendBlock);
        m_qMutex.unlock();
        // The producers only append to m_qSendBlock, so writing here in this thread never stalls them.
        if (!t_qBlock.isEmpty()) {
            if (const std::error_code t_error = m_socket.write(std::as_bytes(std::span(t_qBlock.constData(), static_cast<std::size_t>(t_qBlock.size()))), kWriteTimeout)) {
                Log::info("FiffStreamClient (ID {}) dropped: {}\n", m_iDataClientId, t_error.message());
                break;
            }
        }

        // Read: wait 10 ms for data; parse every complete tag (kind, type, size, next, data)
        const auto t_read = m_socket.read_some(t_chunk, std::chrono::milliseconds(10));
        if (t_read) {
            t_received.append(reinterpret_cast<const char*>(t_chunk.data()), static_cast<qsizetype>(*t_read));
        } else if (t_read.error() != std::errc::timed_out) {
            break;
        }
        while (t_received.size() >= kHeaderBytes) {
            const qint32 size = qFromBigEndian<qint32>(t_received.constData() + 2 * sizeof(qint32));
            if (size < 0) {
                t_received.clear();
                break;
            }
            if (t_received.size() < kHeaderBytes + size) {
                break;
            }
            const auto* begin = reinterpret_cast<const std::byte*>(t_received.constData());
            FiffStream in(std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(begin, begin + kHeaderBytes + size)));
            t_received.remove(0, kHeaderBytes + size);
            if (const std::optional<FiffTag> tag = in.read_tag(); tag && tag->kind == FIFF_MNE_RT_COMMAND) {
                parseCommand(*tag);
            }
        }
    }

    m_socket.close();
}
