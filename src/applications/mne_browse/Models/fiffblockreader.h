//=============================================================================================================
/**
 * @file     fiffblockreader.h
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
#include <QBuffer>
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
    //=========================================================================================================
    /**
     * Constructs an unopened demand-paged FIFF reader.
     *
     * @param[in] parent    Parent QObject.
     */
    explicit FiffBlockReader(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the FIFF block reader and releases any open resources.
     */
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
     * Open raw FIFF content that is already resident in memory.
     * Primarily used by the WASM/browser file picker path where there is no stable filesystem path.
     *
     * @param[in] data         Raw FIFF file contents.
     * @param[in] displayName  Friendly name used by the caller for status/UI display.
     * @return true on success.
     */
    bool openBuffer(const QByteArray &data, const QString &displayName = QString());

    //=========================================================================================================
    /**
     * Close the file and release all resources.
     */
    void close();

    //=========================================================================================================
    /**
     * Returns whether a raw file is currently open.
     *
     * @return True if the reader holds a valid raw FIFF handle.
     */
    bool isOpen() const { return !m_raw.isNull() && !m_raw->isEmpty(); }

    // ── File metadata ─────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Returns the measurement information of the currently opened raw file.
     *
     * @return Shared measurement info pointer.
     */
    QSharedPointer<FIFFLIB::FiffInfo> fiffInfo()   const { return m_fiffInfo; }

    //=========================================================================================================
    /**
     * Returns the first sample index of the opened raw file.
     *
     * @return First raw-file sample index.
     */
    int                               firstSample() const { return m_firstSample; }

    //=========================================================================================================
    /**
     * Returns the last sample index of the opened raw file.
     *
     * @return Last raw-file sample index.
     */
    int                               lastSample()  const { return m_lastSample; }

    //=========================================================================================================
    /**
     * Returns the total number of available samples.
     *
     * @return Total raw-file sample count.
     */
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
    //=========================================================================================================
    /**
     * Performs the actual block read for a sample range.
     *
     * @param[in] from  First sample to load.
     * @param[in] to    Last sample to load.
     * @return Loaded channel-by-sample matrix, or an empty matrix on error.
     */
    Eigen::MatrixXd doRead(int from, int to);

    QFile*                               m_file        = nullptr; /**< File-backed source device for desktop paths. */
    QBuffer*                             m_buffer      = nullptr; /**< Memory-backed source device used for in-memory loads. */
    QByteArray                           m_bufferData;            /**< Owned buffer for memory-backed FIFF content. */
    QSharedPointer<FIFFLIB::FiffRawData> m_raw;                   /**< Raw FIFF reader used for header and block access. */
    QSharedPointer<FIFFLIB::FiffInfo>    m_fiffInfo;              /**< Cached measurement information of the open file. */
    int                                  m_firstSample = 0;       /**< First sample index of the open raw file. */
    int                                  m_lastSample  = 0;       /**< Last sample index of the open raw file. */

    QFutureWatcher<Eigen::MatrixXd>      m_watcher;               /**< Watcher for the currently running async block read. */
    int                                  m_inFlightFrom = 0;      /**< First sample of the currently running async read. */
    bool                                 m_loading      = false;  /**< True while an async read is running. */

    // Queued-up request: set when loadBlockAsync() is called while a read is running.
    // The in-flight read is allowed to complete (no waitForFinished), but its result
    // is discarded; then the pending request is started immediately.
    bool                                 m_hasPending   = false;  /**< True if another block read is queued behind the current one. */
    int                                  m_pendingFrom  = 0;      /**< First sample of the queued request. */
    int                                  m_pendingTo    = 0;      /**< Last sample of the queued request. */
};

} // namespace MNEBROWSE

#endif // FIFFBLOCKREADER_H
