//=============================================================================================================
/**
 * @file     spectrogram.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of spectrogram class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrogram.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd Spectrogram::makeSpectrogram(VectorXd signal, qint32 windowSize = 0)
{
    //QElapsedTimer timer;
    //timer.start();

    signal.array() -= signal.mean();
    QList<SpectogramInputData> lData;
    int iThreadSize = QThread::idealThreadCount()*2;
    int iStepsSize = signal.rows()/iThreadSize;
    int iResidual = signal.rows()%iThreadSize;

    SpectogramInputData dataTemp;
    dataTemp.vecInputData = signal;
    dataTemp.window_size = windowSize;
    if(dataTemp.window_size == 0) {
        dataTemp.window_size = signal.rows()/15;
    }

    for (int i = 0; i < iThreadSize; ++i) {
        dataTemp.iRangeLow = i*iStepsSize;
        dataTemp.iRangeHigh = i*iStepsSize+iStepsSize;
        lData.append(dataTemp);
    }

    dataTemp.iRangeLow = iThreadSize*iStepsSize;
    dataTemp.iRangeHigh = iThreadSize*iStepsSize+iResidual;
    lData.append(dataTemp);

    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(lData,
                                                              compute,
                                                              reduce);
    resultMat.waitForFinished();

    //qDebug() << "Spectrogram::make_spectrogram - timer.elapsed()" << timer.elapsed();
    return resultMat.result();
}

//=============================================================================================================

VectorXd Spectrogram::gaussWindow(qint32 sample_count, qreal scale, quint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sample_count);

    for(qint32 n = 0; n < sample_count; n++)
    {
        qreal t = (qreal(n) - translation) / scale;
        gauss[n] = exp(-3.14 * pow(t, 2))*pow(sqrt(scale),(-1))*pow(qreal(2),(0.25));
    }

    return gauss;
}

//=============================================================================================================

MatrixXd Spectrogram::compute(const SpectogramInputData& inputData)
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    Eigen::FFT<double> fft;
    MatrixXd tf_matrix = MatrixXd::Zero(inputData.vecInputData.rows()/2, inputData.vecInputData.rows());
    VectorXd envelope, windowed_sig, real_coeffs;
    VectorXcd fft_win_sig;
    qint32 window_size = inputData.window_size;

    for(quint32 translate = inputData.iRangeLow; translate < inputData.iRangeHigh; translate++) {
        envelope = gaussWindow(inputData.vecInputData.rows(), window_size, translate);

        windowed_sig = VectorXd::Zero(inputData.vecInputData.rows());
        fft_win_sig = VectorXcd::Zero(inputData.vecInputData.rows());

        windowed_sig = inputData.vecInputData.array() * envelope.array();\

        fft.fwd(fft_win_sig, windowed_sig);

        real_coeffs = fft_win_sig.segment(0,inputData.vecInputData.rows()/2).array().abs2();

        tf_matrix.col(translate) = real_coeffs;
    }

    return tf_matrix;
}

//=============================================================================================================

void Spectrogram::reduce(MatrixXd &resultData,
                         const MatrixXd &data)
{
    if(resultData.size() == 0) {
        resultData = data;
    } else {
        resultData += data;
    }
}
