//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiffblockreader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Definition of the FiffBlockReader class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "fiffblockreader.h"

#include <base/byte_stream.h>
#include <base/string_utils.h>
#include <base/thread_pool.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffBlockReader::FiffBlockReader(QObject* parent)
: QObject(parent)
, m_file(new QFile(this))
{
}

//=============================================================================================================

FiffBlockReader::~FiffBlockReader()
{
    if (m_readFuture.valid()) {
        m_readFuture.wait();
    }
}

//=============================================================================================================

bool FiffBlockReader::open(const QString& path)
{
    close();

    // FiffStream::open() calls device->open() internally, so the QFile must NOT
    // be pre-opened here. Just set the file name and pass it to FiffRawData.
    m_file->setFileName(path);
    if (!QFile::exists(path)) {
        Log::warning("[FiffBlockReader] File not found: {}", path.toStdString());
        return false;
    }

    try {
        m_raw = std::make_shared<FiffRawData>(path.toStdU16String());
    } catch (const std::exception& e) {
        Log::warning("[FiffBlockReader] Exception constructing FiffRawData: {}", e.what());
        return false;
    }
    if (m_raw->empty()) {
        Log::warning("[FiffBlockReader] Not a valid FIFF raw file: {}", path.toStdString());
        m_raw.reset();
        return false;
    }

    m_fiffInfo = std::make_shared<FiffInfo>(m_raw->info);
    m_firstSample = m_raw->first_samp;
    m_lastSample = m_raw->last_samp;

    Log::info("[FiffBlockReader] Opened {} | channels: {} | samples: {} | sfreq: {}", path.toStdString(), m_fiffInfo->nchan, (m_lastSample - m_firstSample + 1), m_fiffInfo->sfreq);

    return true;
}

//=============================================================================================================

bool FiffBlockReader::openBuffer(const QByteArray& data, const QString& displayName)
{
    Q_UNUSED(displayName)

    close();
    m_file->setFileName(QString());

    const auto* begin = reinterpret_cast<const std::byte*>(data.constData());
    try {
        m_raw = std::make_shared<FiffRawData>(std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(begin, begin + data.size())));
    } catch (const std::exception& e) {
        Log::warning("[FiffBlockReader] Exception constructing FiffRawData from buffer: {}", e.what());
        return false;
    }

    if (m_raw->empty()) {
        Log::warning("[FiffBlockReader] Not a valid FIFF raw buffer");
        m_raw.reset();
        return false;
    }

    m_fiffInfo = std::make_shared<FiffInfo>(m_raw->info);
    m_firstSample = m_raw->first_samp;
    m_lastSample = m_raw->last_samp;

    Log::info("[FiffBlockReader] Opened in-memory raw buffer | channels: {} | samples: {} | sfreq: {}", m_fiffInfo->nchan, (m_lastSample - m_firstSample + 1), m_fiffInfo->sfreq);

    return true;
}

//=============================================================================================================

void FiffBlockReader::close()
{
    if (m_readFuture.valid()) {
        m_loading = false; // discard incoming result
        m_readFuture.wait();
    }
    m_raw.reset();
    m_fiffInfo.reset();
    m_firstSample = 0;
    m_lastSample = 0;
    if (m_file->isOpen())
        m_file->close();
}

//=============================================================================================================

void FiffBlockReader::loadBlockAsync(int from, int to)
{
    if (!m_raw)
        return;

    from = qBound(m_firstSample, from, m_lastSample);
    to = qBound(m_firstSample, to, m_lastSample);
    if (from > to)
        return;

    if (m_readFuture.valid()) {
        // A read is already in flight — queue this request instead of blocking.
        // The finished handler will start it immediately when the current read ends.
        m_pendingFrom = from;
        m_pendingTo = to;
        m_hasPending = true;
        m_loading = false; // discard the in-flight result
        return;
    }

    m_hasPending = false;
    startRead(from, to);
}

//=============================================================================================================

void FiffBlockReader::startRead(int from, int to)
{
    m_inFlightFrom = from;
    m_loading = true;
    m_readFuture = ThreadPool::global().submit([this, pThis = QPointer<FiffBlockReader>(this), from, to]() {
        MatrixXd result = doRead(from, to);
        if (pThis) {
            QMetaObject::invokeMethod(pThis, &FiffBlockReader::onReadFinished, Qt::QueuedConnection);
        }
        return result;
    });
}

//=============================================================================================================

void FiffBlockReader::onReadFinished()
{
    MatrixXd result = m_readFuture.get();
    bool shouldEmit = m_loading;
    int emitFrom = m_inFlightFrom;
    m_loading = false;

    if (m_hasPending) {
        // A newer request arrived while we were reading — start it now.
        m_hasPending = false;
        startRead(m_pendingFrom, m_pendingTo);
        return; // discard the superseded result
    }

    if (!shouldEmit)
        return;
    if (result.rows() > 0 && result.cols() > 0)
        emit blockLoaded(result, emitFrom);
}

//=============================================================================================================

MatrixXd FiffBlockReader::readBlockSync(int from, int to)
{
    if (!m_raw)
        return {};
    from = qBound(m_firstSample, from, m_lastSample);
    to = qBound(m_firstSample, to, m_lastSample);
    if (from > to)
        return {};
    return doRead(from, to);
}

//=============================================================================================================

MatrixXd FiffBlockReader::doRead(int from, int to)
{
    // FiffRawData closes its stream after the header; keeping it open spares a reopen per block.
    if (!m_raw->file->reopen()) {
        Log::warning("[FiffBlockReader] Cannot reopen file: {}", m_file->fileName().toStdString());
        return {};
    }

    MatrixXd data, times;
    if (!m_raw->read_raw_segment(data, times, from, to)) {
        Log::warning("[FiffBlockReader] read_raw_segment failed for {} - {}", from, to);
        return {};
    }
    return data;
}

//=============================================================================================================

void FiffBlockReader::updateProjections()
{
    if (!m_raw || !m_fiffInfo)
        return;

    Log::warning("[FiffBlockReader] updateProjections called. FiffInfo ptr= {}", reinterpret_cast<void*>(m_fiffInfo.get()));

    // Count active projectors for diagnostics
    int nActive = 0;
    for (int i = 0; i < std::ssize(m_fiffInfo->projs); ++i) {
        Log::warning("[FiffBlockReader] proj {} desc= {} active= {}", i, m_fiffInfo->projs[i].desc, m_fiffInfo->projs[i].active);
        if (m_fiffInfo->projs[i].active)
            ++nActive;
    }

    // Recompute the projector matrix from active flags in FiffInfo::projs
    Eigen::MatrixXd matProj;
    qint32 nproj = m_fiffInfo->make_projector(matProj);
    if (nproj > 0) {
        m_raw->proj = matProj;
        // Verify it's not identity
        double offDiagNorm = 0.0;
        if (matProj.rows() > 0 && matProj.cols() > 0) {
            Eigen::MatrixXd diff = matProj - Eigen::MatrixXd::Identity(matProj.rows(), matProj.cols());
            offDiagNorm = diff.norm();
        }
        Log::warning("[FiffBlockReader] Projections updated: {} active projectors, {} applied. Matrix: {} x {} off-identity norm: {}", nActive, nproj, matProj.rows(), matProj.cols(), offDiagNorm);
    } else {
        m_raw->proj.resize(0, 0); // Clear — no active projectors
        Log::warning("[FiffBlockReader] Projections cleared (0 active).");
    }
}
