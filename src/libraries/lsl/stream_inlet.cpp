//=============================================================================================================
/**
 * @file     stream_inlet.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the definition of the stream_inlet class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stream_inlet.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>
#include <QByteArray>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstring>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace lsl;

//=============================================================================================================
// PRIVATE IMPLEMENTATION
//=============================================================================================================

/**
 * @brief Private implementation for stream_inlet (PIMPL).
 */
class lsl::StreamInletPrivate
{
public:
    StreamInletPrivate(const stream_info& info)
    : m_info(info)
    , m_pSocket(nullptr)
    , m_bIsOpen(false)
    , m_iChannelCount(info.channel_count())
    , m_iBytesPerSample(static_cast<int>(info.channel_count() * sizeof(float)))
    {
    }

    ~StreamInletPrivate()
    {
        closeStream();
    }

    //=========================================================================================================
    /**
     * Open a TCP connection to the stream outlet.
     */
    void openStream()
    {
        if (m_bIsOpen) {
            return;
        }

        // Create socket (no parent, we manage lifetime ourselves)
        m_pSocket = new QTcpSocket();

        QString host = QString::fromStdString(m_info.data_host());
        quint16 port = static_cast<quint16>(m_info.data_port());

        if (host.isEmpty() || port == 0) {
            delete m_pSocket;
            m_pSocket = nullptr;
            throw std::runtime_error("[lsl::stream_inlet] Invalid data host or port in stream_info");
        }

        m_pSocket->connectToHost(host, port);

        if (!m_pSocket->waitForConnected(5000)) {
            QString err = m_pSocket->errorString();
            delete m_pSocket;
            m_pSocket = nullptr;
            throw std::runtime_error(std::string("[lsl::stream_inlet] Failed to connect to outlet: ")
                                     + err.toStdString());
        }

        // Read the handshake header: "LSL1" (4 bytes) + channel_count (4 bytes, little-endian)
        if (!m_pSocket->waitForReadyRead(5000)) {
            delete m_pSocket;
            m_pSocket = nullptr;
            throw std::runtime_error("[lsl::stream_inlet] Timeout waiting for handshake from outlet");
        }

        QByteArray header;
        while (header.size() < 8) {
            if (m_pSocket->bytesAvailable() == 0) {
                if (!m_pSocket->waitForReadyRead(5000)) {
                    break;
                }
            }
            header.append(m_pSocket->read(8 - header.size()));
        }

        if (header.size() < 8 || header.left(4) != QByteArray("LSL1", 4)) {
            delete m_pSocket;
            m_pSocket = nullptr;
            throw std::runtime_error("[lsl::stream_inlet] Invalid handshake from outlet");
        }

        // Read channel count from header (little-endian int32)
        int headerChannels = 0;
        std::memcpy(&headerChannels, header.constData() + 4, sizeof(int));
        if (headerChannels != m_iChannelCount) {
            qDebug() << "[lsl::stream_inlet] Warning: outlet reports" << headerChannels
                     << "channels, expected" << m_iChannelCount << "- using outlet value";
            m_iChannelCount = headerChannels;
            m_iBytesPerSample = m_iChannelCount * static_cast<int>(sizeof(float));
        }

        m_bIsOpen = true;
    }

    //=========================================================================================================
    /**
     * Close the TCP connection.
     */
    void closeStream()
    {
        if (m_pSocket) {
            if (m_pSocket->state() == QAbstractSocket::ConnectedState) {
                m_pSocket->disconnectFromHost();
                if (m_pSocket->state() != QAbstractSocket::UnconnectedState) {
                    m_pSocket->waitForDisconnected(1000);
                }
            }
            delete m_pSocket;
            m_pSocket = nullptr;
        }
        m_bIsOpen = false;
        m_rawBuffer.clear();
    }

    //=========================================================================================================
    /**
     * Try to read pending data from the TCP socket into the raw byte buffer (non-blocking).
     *
     * @return True if at least one complete sample is available in the buffer.
     */
    bool readPending()
    {
        if (!m_bIsOpen || !m_pSocket) {
            return false;
        }

        // Non-blocking check for available data
        if (m_pSocket->bytesAvailable() > 0 || m_pSocket->waitForReadyRead(0)) {
            QByteArray data = m_pSocket->readAll();
            m_rawBuffer.append(data);
        }

        // A complete sample requires m_iBytesPerSample bytes
        return (m_iBytesPerSample > 0) && (m_rawBuffer.size() >= m_iBytesPerSample);
    }

    //=========================================================================================================
    /**
     * Extract all complete samples from the byte buffer as a chunk.
     *
     * @return Vector of samples, each sample is a vector of float channel values.
     */
    std::vector<std::vector<float>> pullChunkFloat()
    {
        std::vector<std::vector<float>> chunk;

        if (!m_bIsOpen || !m_pSocket) {
            return chunk;
        }

        // Read any pending data
        readPending();

        if (m_iBytesPerSample <= 0) {
            return chunk;
        }

        // Extract complete samples from the buffer
        int nCompleteSamples = m_rawBuffer.size() / m_iBytesPerSample;
        if (nCompleteSamples == 0) {
            return chunk;
        }

        chunk.reserve(nCompleteSamples);
        const char* ptr = m_rawBuffer.constData();

        for (int s = 0; s < nCompleteSamples; ++s) {
            std::vector<float> sample(m_iChannelCount);
            std::memcpy(sample.data(), ptr + s * m_iBytesPerSample, m_iBytesPerSample);
            chunk.push_back(std::move(sample));
        }

        // Remove consumed bytes from the buffer
        int consumedBytes = nCompleteSamples * m_iBytesPerSample;
        m_rawBuffer.remove(0, consumedBytes);

        return chunk;
    }

    stream_info     m_info;             /**< The stream info for this inlet. */
    QTcpSocket*     m_pSocket;          /**< TCP socket for data reception. */
    bool            m_bIsOpen;          /**< Whether the stream is currently open. */
    int             m_iChannelCount;    /**< Number of channels. */
    int             m_iBytesPerSample;  /**< Bytes per sample (channels * sizeof(float)). */
    QByteArray      m_rawBuffer;        /**< Raw byte buffer for incoming TCP data. */
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_inlet::stream_inlet(const stream_info& info)
: m_pImpl(new StreamInletPrivate(info))
{
}

//=============================================================================================================

stream_inlet::~stream_inlet()
{
    // unique_ptr destructor handles cleanup via StreamInletPrivate destructor
}

//=============================================================================================================

void stream_inlet::open_stream()
{
    m_pImpl->openStream();
}

//=============================================================================================================

void stream_inlet::close_stream()
{
    m_pImpl->closeStream();
}

//=============================================================================================================

bool stream_inlet::samples_available()
{
    return m_pImpl->readPending();
}

//=============================================================================================================

std::vector<std::vector<float>> stream_inlet::pull_chunk_float()
{
    return m_pImpl->pullChunkFloat();
}
