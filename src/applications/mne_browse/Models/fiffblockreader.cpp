//=============================================================================================================
/**
 * @file     fiffblockreader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Definition of the FiffBlockReader class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffblockreader.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffBlockReader::FiffBlockReader(QObject *parent)
    : QObject(parent)
    , m_file(new QFile(this))
    , m_buffer(new QBuffer(this))
{
    connect(&m_watcher, &QFutureWatcher<MatrixXd>::finished, this, [this]() {
        bool shouldEmit  = m_loading;
        int  emitFrom    = m_inFlightFrom;
        m_loading = false;

        if (m_hasPending) {
            // A newer request arrived while we were reading — start it now.
            int from = m_pendingFrom;
            int to   = m_pendingTo;
            m_hasPending   = false;
            m_inFlightFrom = from;
            m_loading      = true;
            m_watcher.setFuture(QtConcurrent::run([this, from, to]() {
                return doRead(from, to);
            }));
            return; // discard the superseded result
        }

        if (!shouldEmit)
            return;
        MatrixXd result = m_watcher.result();
        if (result.rows() > 0 && result.cols() > 0)
            emit blockLoaded(result, emitFrom);
    });
}

//=============================================================================================================

FiffBlockReader::~FiffBlockReader()
{
    if (m_watcher.isRunning()) {
        m_watcher.waitForFinished();
    }
}

//=============================================================================================================

bool FiffBlockReader::open(const QString &path)
{
    close();

    // FiffStream::open() calls device->open() internally, so the QFile must NOT
    // be pre-opened here. Just set the file name and pass it to FiffRawData.
    m_file->setFileName(path);
    if (!QFile::exists(path)) {
        qWarning() << "[FiffBlockReader] File not found:" << path;
        return false;
    }

    try {
        m_raw = QSharedPointer<FiffRawData>(new FiffRawData(*m_file));
    } catch (const std::exception &e) {
        qWarning() << "[FiffBlockReader] Exception constructing FiffRawData:" << e.what();
        return false;
    }
    if (m_raw->isEmpty()) {
        qWarning() << "[FiffBlockReader] Not a valid FIFF raw file:" << path;
        m_raw.reset();
        return false;
    }

    m_fiffInfo   = QSharedPointer<FiffInfo>(new FiffInfo(m_raw->info));
    m_firstSample = m_raw->first_samp;
    m_lastSample  = m_raw->last_samp;

    qInfo() << "[FiffBlockReader] Opened" << path
            << "| channels:" << m_fiffInfo->nchan
            << "| samples:" << (m_lastSample - m_firstSample + 1)
            << "| sfreq:" << m_fiffInfo->sfreq;

    return true;
}

//=============================================================================================================

bool FiffBlockReader::openBuffer(const QByteArray &data, const QString &displayName)
{
    Q_UNUSED(displayName)

    close();
    m_file->setFileName(QString());

    m_bufferData = data;
    m_buffer->setData(m_bufferData);
    if (!m_buffer->open(QIODevice::ReadOnly)) {
        qWarning() << "[FiffBlockReader] Could not open in-memory FIFF buffer";
        m_bufferData.clear();
        return false;
    }

    try {
        m_raw = QSharedPointer<FiffRawData>(new FiffRawData(*m_buffer));
    } catch (const std::exception &e) {
        qWarning() << "[FiffBlockReader] Exception constructing FiffRawData from buffer:" << e.what();
        m_buffer->close();
        m_bufferData.clear();
        return false;
    }

    if (m_raw->isEmpty()) {
        qWarning() << "[FiffBlockReader] Not a valid FIFF raw buffer";
        m_raw.reset();
        m_buffer->close();
        m_bufferData.clear();
        return false;
    }

    m_fiffInfo    = QSharedPointer<FiffInfo>(new FiffInfo(m_raw->info));
    m_firstSample = m_raw->first_samp;
    m_lastSample  = m_raw->last_samp;

    qInfo() << "[FiffBlockReader] Opened in-memory raw buffer"
            << "| channels:" << m_fiffInfo->nchan
            << "| samples:" << (m_lastSample - m_firstSample + 1)
            << "| sfreq:" << m_fiffInfo->sfreq;

    return true;
}

//=============================================================================================================

void FiffBlockReader::close()
{
    if (m_watcher.isRunning()) {
        m_loading = false; // discard incoming result
        m_watcher.waitForFinished();
    }
    m_raw.reset();
    m_fiffInfo.reset();
    m_firstSample = 0;
    m_lastSample  = 0;
    if (m_file->isOpen())
        m_file->close();
    if (m_buffer->isOpen())
        m_buffer->close();
    m_bufferData.clear();
}

//=============================================================================================================

void FiffBlockReader::loadBlockAsync(int from, int to)
{
    if (!m_raw) return;

    from = qBound(m_firstSample, from, m_lastSample);
    to   = qBound(m_firstSample, to,   m_lastSample);
    if (from > to) return;

    if (m_watcher.isRunning()) {
        // A read is already in flight — queue this request instead of blocking.
        // The finished handler will start it immediately when the current read ends.
        m_pendingFrom = from;
        m_pendingTo   = to;
        m_hasPending  = true;
        m_loading     = false; // discard the in-flight result
        return;
    }

    m_hasPending   = false;
    m_inFlightFrom = from;
    m_loading      = true;
    m_watcher.setFuture(QtConcurrent::run([this, from, to]() {
        return doRead(from, to);
    }));
}

//=============================================================================================================

MatrixXd FiffBlockReader::readBlockSync(int from, int to)
{
    if (!m_raw) return {};
    from = qBound(m_firstSample, from, m_lastSample);
    to   = qBound(m_firstSample, to,   m_lastSample);
    if (from > to) return {};
    return doRead(from, to);
}

//=============================================================================================================

MatrixXd FiffBlockReader::doRead(int from, int to)
{
    // FiffRawData closes the underlying device after parsing the header.
    // Re-open it so read_raw_segment can seek and read data blocks.
    if (!m_file->fileName().isEmpty()) {
        if (!m_file->isOpen()) {
            if (!m_file->open(QIODevice::ReadOnly)) {
                qWarning() << "[FiffBlockReader] Cannot reopen file:" << m_file->fileName();
                return {};
            }
        }
    } else {
        m_buffer->setData(m_bufferData);
        if (!m_buffer->isOpen()) {
            if (!m_buffer->open(QIODevice::ReadOnly)) {
                qWarning() << "[FiffBlockReader] Cannot reopen in-memory buffer";
                return {};
            }
        } else {
            m_buffer->seek(0);
        }
    }

    MatrixXd data, times;
    if (!m_raw->read_raw_segment(data, times, from, to)) {
        qWarning() << "[FiffBlockReader] read_raw_segment failed for"
                   << from << "-" << to;
        return {};
    }
    return data;
}
