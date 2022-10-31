//=============================================================================================================
/**
 * @file     crosscorrelation.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    CrossCorrelation class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "crosscorrelation.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

#include <utils/spectral.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CrossCorrelation::CrossCorrelation()
{
}

//=============================================================================================================

Network CrossCorrelation::calculate(ConnectivitySettings& connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    Network finalNetwork("XCOR");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "CrossCorrelation::calculate - Input data is empty";
        return finalNetwork;
    }

    if(AbstractMetric::m_bStorageModeIsActive == false) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.setSamplingFrequency(connectivitySettings.getSamplingFrequency());

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        rowVert = RowVectorXf::Zero(3);

        if(connectivitySettings.getNodePositions().rows() != 0 && i < connectivitySettings.getNodePositions().rows()) {
            rowVert(0) = connectivitySettings.getNodePositions().row(i)(0);
            rowVert(1) = connectivitySettings.getNodePositions().row(i)(1);
            rowVert(2) = connectivitySettings.getNodePositions().row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    // Generate tapers
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Compute the cross correlation in parallel
    QMutex mutex;
    MatrixXd matDist;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                matDist,
                mutex,
                iNfft,
                tapers);
    };

//    iTime = timer.elapsed();
//    qWarning() << "Preparation" << iTime;
//    timer.restart();

    // Calculate connectivity matrix over epochs and average afterwards
    QFuture<void> resultMat = QtConcurrent::map(connectivitySettings.getTrialData(),
                                                computeLambda);
    resultMat.waitForFinished();

    matDist /= connectivitySettings.size();

//    iTime = timer.elapsed();
//    qWarning() << "ComputeSpectraPSDCSD" << iTime;
//    timer.restart();

    //Add edges to network
    MatrixXd matWeight(1,1);
    QSharedPointer<NetworkEdge> pEdge;
    int j;

    for(int i = 0; i < matDist.rows(); ++i) {
        for(j = i; j < matDist.cols(); ++j) {
            matWeight << matDist(i,j);

            pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

//    iTime = timer.elapsed();
//    qWarning() << "Compute" << iTime;
//    timer.restart();

    return finalNetwork;
}

//=============================================================================================================

void CrossCorrelation::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                               MatrixXd& matDist,
                               QMutex& mutex,
                               int iNfft,
                               const QPair<MatrixXd, VectorXd>& tapers)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    // Calculate tapered spectra if not available already
    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecResultFreq;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    int i, j;
    int iNRows = inputData.matData.rows();

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if(inputData.vecTapSpectra.isEmpty()) {
        int iNFreqs = int(floor(iNfft / 2.0)) + 1;
        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra
            for(j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0,0,1,rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));;
                } else {
                    vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                }

                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecResultFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecResultFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.append(matTapSpectrum);
        }
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "CrossCorrelation::compute timer - Tapered spectra:" << iTime;
//    timer.restart();

    // Perform multiplication and transform back to time domain to find max XCOR coefficient
    // Note that the result in time domain is mirrored around the center of the data (compared to Matlab)
    MatrixXd matDistTrial = MatrixXd::Zero(iNRows, iNRows);
    RowVectorXcd vecResultXCor;
    int idx = 0;
    double denom = tapers.second.sum();

    for(i = 0; i < inputData.vecTapSpectra.size(); ++i) {
        vecResultFreq = inputData.vecTapSpectra.at(i).colwise().sum() / denom;

        for(j = i; j < inputData.vecTapSpectra.size(); ++j) {
            vecResultXCor = vecResultFreq.cwiseProduct(inputData.vecTapSpectra.at(j).colwise().sum() / denom);

            fft.inv(vecInputFFT, vecResultXCor, iNfft);

            vecInputFFT.maxCoeff(&idx);

            matDistTrial(i,j) = vecInputFFT(idx);
        }
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "CrossCorrelation::compute timer - Multiplication and inv FFT:" << iTime;
//    timer.restart();

    // Sum up weights
    mutex.lock();

    if(matDist.rows() != matDistTrial.rows() || matDist.cols() != matDistTrial.cols()) {
        matDist.resize(matDistTrial.rows(), matDistTrial.cols());
        matDist.setZero();
    }

    matDist += matDistTrial;

    mutex.unlock();

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "CrossCorrelation::compute timer - Summing up matDist:" << iTime;
//    timer.restart();

    if(!m_bStorageModeIsActive) {
        inputData.vecTapSpectra.clear();
    }
}
