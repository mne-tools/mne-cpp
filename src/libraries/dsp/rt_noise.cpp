//=============================================================================================================
/**
 * @file     rtnoise.cpp
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
 * @brief     Definition of the RtNoiseWorker and RtNoise classes.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_noise.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS — RtNoiseWorker
//=============================================================================================================

RtNoiseWorker::RtNoiseWorker(qint32 iFftLength,
                             FiffInfo::SPtr pFiffInfo,
                             qint32 iDataLength)
: m_iFftLength(iFftLength)
, m_dFs(pFiffInfo->sfreq)
, m_iDataLength(iDataLength < 1 ? 10 : iDataLength)
{
    m_fWin = hanning(m_iFftLength, 0);
}

//=============================================================================================================

void RtNoiseWorker::doWork(const MatrixXd& matData)
{
    if(m_bFirstBlock) {
        m_iNumOfBlocks = m_iDataLength;
        m_iBlockSize = static_cast<int>(matData.cols());
        m_iSensors = static_cast<int>(matData.rows());
        m_matCircBuf.resize(m_iSensors, m_iNumOfBlocks * m_iBlockSize);
        m_iBlockIndex = 0;
        m_bFirstBlock = false;
    }

    // Accumulate block into circular buffer
    m_matCircBuf.block(0, m_iBlockIndex * m_iBlockSize, m_iSensors, m_iBlockSize) = matData;

    m_iBlockIndex++;
    if(m_iBlockIndex < m_iNumOfBlocks) {
        return;
    }

    // Enough blocks accumulated — compute spectrum
    m_iBlockIndex = 0;

    const int iTotalSamples = m_iNumOfBlocks * m_iBlockSize;
    const int iHalfSpec = m_iFftLength / 2 + 1;
    const int nb = iTotalSamples / m_iFftLength + 1;

    MatrixXd sum_psdx = MatrixXd::Zero(m_iSensors, iHalfSpec);
    RowVectorXd vecDataZeroPad = RowVectorXd::Zero(m_iFftLength);
    RowVectorXcd vecFreqData(iHalfSpec);

    for(int n = 0; n < nb; ++n) {
        const int iOffset = n * m_iFftLength;

        for(int i = 0; i < m_iSensors; ++i) {
            // Extract and zero-pad segment
            vecDataZeroPad.setZero();
            const int iCopyLen = std::min(m_iFftLength, iTotalSamples - iOffset);
            vecDataZeroPad.head(iCopyLen) = m_matCircBuf.block(i, iOffset, 1, iCopyLen);

            // Apply Hanning window
            for(int k = 0; k < m_iFftLength; ++k) {
                vecDataZeroPad[k] *= m_fWin[k];
            }

            // FFT
            Eigen::FFT<double> fft;
            fft.SetFlag(fft.HalfSpectrum);
            fft.fwd(vecFreqData, vecDataZeroPad);

            // PSD from FFT
            for(int j = 0; j < iHalfSpec; ++j) {
                const double mag = std::abs(vecFreqData(j));
                double spower = mag / (m_dFs * m_iFftLength);
                if(j > 0 && j < m_iFftLength / 2) {
                    spower *= 2.0;
                }
                sum_psdx(i, j) += spower;
            }
        }
    }

    // Convert to dB
    MatrixXd matResult(m_iSensors, iHalfSpec);
    for(int i = 0; i < m_iSensors; ++i) {
        for(int j = 0; j < iHalfSpec; ++j) {
            matResult(i, j) = 10.0 * std::log10(sum_psdx(i, j) / nb);
        }
    }

    emit resultReady(matResult);
}

//=============================================================================================================

QVector<float> RtNoiseWorker::hanning(int N, short itype)
{
    QVector<float> w(N, 0.0f);

    const int n = (itype == 1) ? N - 1 : N;

    if(n % 2 == 0) {
        const int half = n / 2;
        for(int i = 0; i < half; ++i) {
            w[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * (i + 1) / (n + 1)));
        }
        int idx = half - 1;
        for(int i = half; i < n; ++i) {
            w[i] = w[idx--];
        }
    } else {
        const int half = (n + 1) / 2;
        for(int i = 0; i < half; ++i) {
            w[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * (i + 1) / (n + 1)));
        }
        int idx = half - 2;
        for(int i = half; i < n; ++i) {
            w[i] = w[idx--];
        }
    }

    if(itype == 1) {
        for(int i = N - 1; i >= 1; --i) {
            w[i] = w[i - 1];
        }
        w[0] = 0.0f;
    }

    return w;
}

//=============================================================================================================
// DEFINE MEMBER METHODS — RtNoise
//=============================================================================================================

RtNoise::RtNoise(qint32 iFftLength,
                  FiffInfo::SPtr pFiffInfo,
                  qint32 iDataLength,
                  QObject *parent)
: QObject(parent)
{
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");

    auto* worker = new RtNoiseWorker(iFftLength, pFiffInfo, iDataLength);
    worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished,
            worker, &QObject::deleteLater);
    connect(this, &RtNoise::operate,
            worker, &RtNoiseWorker::doWork);
    connect(worker, &RtNoiseWorker::resultReady,
            this, &RtNoise::SpecCalculated);
}

//=============================================================================================================

RtNoise::~RtNoise()
{
    if(m_bIsRunning) {
        stop();
    }
}

//=============================================================================================================

void RtNoise::append(const MatrixXd& matData)
{
    emit operate(matData);
}

//=============================================================================================================

bool RtNoise::isRunning() const
{
    return m_bIsRunning;
}

//=============================================================================================================

bool RtNoise::start()
{
    if(m_workerThread.isRunning()) {
        m_workerThread.wait();
    }

    m_bIsRunning = true;
    m_workerThread.start();
    return true;
}

//=============================================================================================================

bool RtNoise::stop()
{
    m_bIsRunning = false;
    m_workerThread.quit();
    m_workerThread.wait();
    return true;
}

//=============================================================================================================

bool RtNoise::wait(unsigned long time)
{
    return m_workerThread.wait(time);
}

