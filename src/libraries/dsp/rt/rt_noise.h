//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_noise.h
 * @since March 2026
 * @brief Real-time noise power-spectral-density estimation from streaming data blocks.
 *
 * RtNoiseWorker accumulates a configurable number of contiguous data blocks,
 * applies an FFT of user-defined length and averages the squared magnitude
 * spectra into a running noise-PSD estimate. The estimator follows the
 * classical periodogram-averaging recipe (Welch with rectangular windowing
 * and no overlap), so longer accumulation windows trade temporal
 * responsiveness for spectral variance reduction. RtNoise is the QObject
 * front-end that owns the worker, forwards configuration changes, and re-
 * emits the PSD result back on the GUI thread.
 *
 * This estimate is what feeds the live noise-spectrum display and provides
 * a sanity-check companion to @ref RtCov whenever a quick frequency-domain
 * view of the noise floor is more informative than the full channel–channel
 * covariance matrix.
 */

#ifndef RT_NOISE_RTPROCESSING_H
#define RT_NOISE_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * @brief Background worker that computes a noise power spectral density estimate from accumulated data blocks.
 */
class DSPSHARED_EXPORT RtNoiseWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates the worker.
     *
     * @param[in] iFftLength    FFT length (number of samples per window).
     * @param[in] pFiffInfo     Associated Fiff Information.
     * @param[in] iDataLength   Number of blocks to accumulate before computing the spectrum.
     */
    explicit RtNoiseWorker(qint32 iFftLength,
                           FIFFLIB::FiffInfo::SPtr pFiffInfo,
                           qint32 iDataLength);

    //=========================================================================================================
    /**
     * Process one data block, accumulate, and emit the spectrum when enough blocks are collected.
     *
     * @param[in] matData   The incoming data block (channels x samples).
     */
    void doWork(const Eigen::MatrixXd& matData);

signals:
    /**
     * Emitted when a new power spectral density matrix is available.
     *
     * @param[out] matSpecData  The computed spectrum (channels x frequency bins) in dB.
     */
    void resultReady(const Eigen::MatrixXd& matSpecData);

private:
    static QVector<float> hanning(int N, short itype);

    qint32              m_iFftLength;       /**< FFT window length. */
    double              m_dFs;              /**< Sampling frequency. */
    qint32              m_iDataLength;      /**< Number of blocks to accumulate. */
    QVector<float>      m_fWin;             /**< Hanning window coefficients. */

    int                 m_iNumOfBlocks = 0; /**< Total blocks to accumulate. */
    int                 m_iBlockSize = 0;   /**< Columns per block. */
    int                 m_iSensors = 0;     /**< Number of sensor rows. */
    int                 m_iBlockIndex = 0;  /**< Current block write index. */
    bool                m_bFirstBlock = true;/**< True until first block arrives. */
    Eigen::MatrixXd     m_matCircBuf;       /**< Circular accumulation buffer. */
};

//=============================================================================================================
/**
 * @brief Controller that manages RtNoiseWorker for real-time noise spectrum estimation.
 */
class DSPSHARED_EXPORT RtNoise : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtNoise> SPtr;             /**< Shared pointer type for RtNoise. */
    typedef QSharedPointer<const RtNoise> ConstSPtr;  /**< Const shared pointer type for RtNoise. */

    //=========================================================================================================
    /**
     * Creates the real-time noise estimation object.
     *
     * @param[in] iFftLength    Number of samples per FFT window.
     * @param[in] pFiffInfo     Associated Fiff Information.
     * @param[in] iDataLength   Number of blocks to accumulate before computing.
     * @param[in] parent        Parent QObject (optional).
     */
    explicit RtNoise(qint32 iFftLength,
                     FIFFLIB::FiffInfo::SPtr pFiffInfo,
                     qint32 iDataLength,
                     QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the real-time noise estimation object.
     */
    ~RtNoise();

    //=========================================================================================================
    /**
     * Submit incoming data for noise estimation.
     *
     * @param[in] matData  Data block (channels x samples).
     */
    void append(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Returns whether the worker thread is running.
     */
    bool isRunning() const;

    //=========================================================================================================
    /**
     * Starts the worker thread.
     *
     * @return true on success.
     */
    bool start();

    //=========================================================================================================
    /**
     * Stops the worker thread.
     *
     * @return true on success.
     */
    bool stop();

    //=========================================================================================================
    /**
     * Blocks until the worker thread has finished, or until the timeout (ms) expires.
     */
    bool wait(unsigned long time = ULONG_MAX);

signals:
    /**
     * Emitted when a new spectrum is available. Forwarded from the worker.
     */
    void SpecCalculated(const Eigen::MatrixXd& matSpecData);

    /**
     * Internal signal to forward data to the worker thread.
     */
    void operate(const Eigen::MatrixXd& matData);

private:
    QThread             m_workerThread;     /**< The worker thread. */
    bool                m_bIsRunning = false;/**< Running state flag. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // RT_NOISE_RTPROCESSING_H
