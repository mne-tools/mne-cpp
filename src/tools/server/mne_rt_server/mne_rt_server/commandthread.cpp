//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandthread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandThread Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandthread.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QtEndian>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandThread::CommandThread(mne::TcpSocket socket, qint32 p_iId, QObject* parent)
: QThread(parent)
, m_socket(std::move(socket))
, m_bIsRunning(false)
, m_iThreadID(p_iId)
{
}

//=============================================================================================================

CommandThread::~CommandThread()
{
    m_bIsRunning = false;
    QThread::wait();
}

//=============================================================================================================

void CommandThread::attachCommandReply(QString p_blockReply, qint32 p_iID)
{
    qDebug() << "CommandThread::attachCommandReply";
    if (p_iID == m_iThreadID) {
        m_qMutex.lock();
        m_qSendData = p_blockReply;
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void CommandThread::run()
{
    m_bIsRunning = true;

    qInfo("CommandClient connection accepted (ID %d)\n", m_iThreadID);

    // Frames are a big-endian quint16 size followed by a QDataStream (Qt 6.0) serialized QString
    QByteArray t_received;
    std::array<std::byte, 4096> t_chunk{};

    while (m_socket.is_open() && m_bIsRunning) {
        //
        // Write available data
        //
        QString t_sReply;
        m_qMutex.lock();
        t_sReply.swap(m_qSendData);
        m_qMutex.unlock();
        if (t_sReply.size() > 0) {
            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_6_0);
            out << static_cast<quint16>(0);
            out << t_sReply;
            out.device()->seek(0);
            out << static_cast<quint16>(block.size() - sizeof(quint16));

            if (m_socket.write(std::as_bytes(std::span(block.constData(), static_cast<std::size_t>(block.size()))))) {
                break;
            }
        }

        //
        // Read: Wait 100ms for incomming data, parse complete frames and continue
        //
        const auto t_read = m_socket.read_some(t_chunk, std::chrono::milliseconds(100));
        if (t_read) {
            t_received.append(reinterpret_cast<const char*>(t_chunk.data()), static_cast<qsizetype>(*t_read));
        } else if (t_read.error() != std::errc::timed_out) {
            break;
        }

        while (t_received.size() >= static_cast<qsizetype>(sizeof(quint16))) {
            const quint16 blockSize = qFromBigEndian<quint16>(t_received.constData());
            if (blockSize >= 65000) { //Sanity Check -> allowed maximal blocksize is 65.000
                t_received.clear();
                break;
            }
            if (t_received.size() < static_cast<qsizetype>(sizeof(quint16)) + blockSize) {
                break;
            }
            QByteArray t_block = t_received.mid(sizeof(quint16), blockSize);
            t_received.remove(0, static_cast<qsizetype>(sizeof(quint16)) + blockSize);

            QDataStream t_in(t_block);
            t_in.setVersion(QDataStream::Qt_6_0);
            QString t_sCommand;
            t_in >> t_sCommand;

            t_sCommand = t_sCommand.simplified();

            //
            // Parse command
            //
            if (!t_sCommand.isEmpty())
                emit newCommand(t_sCommand, m_iThreadID);
        }
    }

    m_socket.close();
}
