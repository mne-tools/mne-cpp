//=============================================================================================================
/**
 * @file     rtnoise.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     RtNoiseWorker and RtNoise class declarations.
 *
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
