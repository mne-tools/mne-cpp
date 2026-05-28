//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file spectrogram.cpp
 * @since March 2026
 * @brief Definition of spectrogram class.
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
