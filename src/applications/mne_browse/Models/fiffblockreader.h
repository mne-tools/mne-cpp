//=============================================================================================================
/**
 * @file     fiffblockreader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
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
 * @brief    Declaration of the FiffBlockReader class.
 *
 */

#ifndef FIFFBLOCKREADER_H
#define FIFFBLOCKREADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QFile>
#include <QSharedPointer>
#include <QFutureWatcher>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * @brief Lightweight, demand-paging FIFF raw data reader.
 *
 * Opens a FIFF file and reads only the measurement info on construction (fast).
 * Individual sample blocks are read on demand via loadBlockAsync() and returned
 * through the blockLoaded() signal so the GUI stays responsive.
 *
 * Usage:
 * @code
 *   FiffBlockReader reader;
 *   if (reader.open("/path/to/data.fif")) {
 *       connect(&reader, &FiffBlockReader::blockLoaded, this, &MyClass::onBlockLoaded);
 *       reader.loadBlockAsync(reader.firstSample(), reader.firstSample() + 30000);
 *   }
 * @endcode
 */
class FiffBlockReader : public QObject
{
    Q_OBJECT

public:
    explicit FiffBlockReader(QObject *parent = nullptr);
    ~FiffBlockReader() override;

    //=========================================================================================================
    /**
     * Open a FIFF raw file and read only the measurement info.
     * This is fast (header only) and must be called before any other method.
     *
     * @param[in] path  Absolute path to the .fif file.
     * @return true on success.
     */
    bool open(const QString &path);

    //=========================================================================================================
    /**
     * Close the file and release all resources.
     */
    void close();

    bool isOpen() const { return !m_raw.isNull() && !m_raw->isEmpty(); }

    // ── File metadata ─────────────────────────────────────────────────

    QSharedPointer<FIFFLIB::FiffInfo> fiffInfo()   const { return m_fiffInfo; }
    int                               firstSample() const { return m_firstSample; }
    int                               lastSample()  const { return m_lastSample; }
    int                               totalSamples() const { return m_lastSample - m_firstSample + 1; }

    // ── Asynchronous block loading ────────────────────────────────────

    //=========================================================================================================
    /**
     * Request an asynchronous load of samples [from, to].
     * The result is delivered via blockLoaded().
     * Calling again while a load is in progress cancels the previous request
     * (the in-flight read completes, but its result is discarded).
     *
     * @param[in] from  First sample to load (absolute index).
     * @param[in] to    Last sample to load (inclusive, absolute index).
     */
    void loadBlockAsync(int from, int to);

    //=========================================================================================================
    /**
     * Synchronous block read.  Blocks the calling thread.
     * Returns an empty matrix on error.
     *
     * @param[in] from  First sample (absolute index).
     * @param[in] to    Last sample (inclusive, absolute index).
     */
    Eigen::MatrixXd readBlockSync(int from, int to);

signals:
    //=========================================================================================================
    /**
     * Emitted when an async block load completes.
     *
     * @param[in] data         channels × samples matrix (double).
     * @param[in] firstSample  Absolute sample index of column 0.
     */
    void blockLoaded(const Eigen::MatrixXd &data, int firstSample);

private:
    Eigen::MatrixXd doRead(int from, int to);

    QFile*                              m_file       = nullptr;
    QSharedPointer<FIFFLIB::FiffRawData> m_raw;
    QSharedPointer<FIFFLIB::FiffInfo>   m_fiffInfo;
    int                                 m_firstSample = 0;
    int                                 m_lastSample  = 0;

    QFutureWatcher<Eigen::MatrixXd>     m_watcher;
    int                                 m_inFlightFrom = 0; // firstSample of the currently running read
    bool                                m_loading      = false;

    // Queued-up request: set when loadBlockAsync() is called while a read is running.
    // The in-flight read is allowed to complete (no waitForFinished), but its result
    // is discarded; then the pending request is started immediately.
    bool                                m_hasPending   = false;
    int                                 m_pendingFrom  = 0;
    int                                 m_pendingTo    = 0;
};

} // namespace MNEBROWSE

#endif // FIFFBLOCKREADER_H
