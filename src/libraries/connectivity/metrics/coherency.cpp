//=============================================================================================================
/**
 * @file     coherency.cpp
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Daniel Strohmeier, Lorenz Esch. All rights reserved.
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
 * @note Notes:
 * - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
 * - QtConcurrent can be used to speed up computation.
 *
 * @brief     Coherency class declaration.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
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

Coherency::Coherency()
{
}

//=============================================================================================================

void Coherency::calculateAbs(Network& finalNetwork,
                             ConnectivitySettings &connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(connectivitySettings.isEmpty()) {
        qDebug() << "Coherency::calculateReal - Input data is empty";
        return;
    }

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Compute PSD/CSD for each trial
    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().matPsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

//    iTime = timer.elapsed();
//    qWarning() << "Preparation" << iTime;
//    timer.restart();

    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
                                             computeLambda);
    result.waitForFinished();

//    iTime = timer.elapsed();
//    qWarning() << "ComputeSpectraPSDCSD" << iTime;
//    timer.restart();

    // Compute CSD/sqrt(PSD_X * PSD_Y)
    std::function<void(QPair<int,MatrixXcd>&)> computePSDCSDLambda = [&](QPair<int,MatrixXcd>& pairInput) {
        computePSDCSDAbs(mutex,
                         finalNetwork,
                         pairInput,
                         connectivitySettings.getIntermediateSumData().matPsdSum);
    };

    QFuture<void> resultCSDPSD = QtConcurrent::map(connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                                                   computePSDCSDLambda);
    resultCSDPSD.waitForFinished();

//    iTime = timer.elapsed();
//    qWarning() << "Compute" << iTime;
//    timer.restart();
}

//=============================================================================================================

void Coherency::calculateImag(Network& finalNetwork,
                              ConnectivitySettings &connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(connectivitySettings.isEmpty()) {
        qDebug() << "Coherency::calculateImag - Input data is empty";
        return;
    }

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Compute PSD/CSD for each trial
    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().matPsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

//    iTime = timer.elapsed();
//    qWarning() << "Preparation" << iTime;
//    timer.restart();

    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
                                             computeLambda);
    result.waitForFinished();

//    iTime = timer.elapsed();
//    qWarning() << "ComputeSpectraPSDCSD" << iTime;
//    timer.restart();

    // Compute CSD/sqrt(PSD_X * PSD_Y)
    std::function<void(QPair<int,MatrixXcd>&)> computePSDCSDLambda = [&](QPair<int,MatrixXcd>& pairInput) {
        computePSDCSDImag(mutex,
                          finalNetwork,
                          pairInput,
                          connectivitySettings.getIntermediateSumData().matPsdSum);
    };

    QFuture<void> resultCSDPSD = QtConcurrent::map(connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                                                   computePSDCSDLambda);
    resultCSDPSD.waitForFinished();

//    iTime = timer.elapsed();
//    qWarning() << "Compute" << iTime;
//    timer.restart();
}

//=============================================================================================================

void Coherency::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        MatrixXd& matPsdSum,
                        QVector<QPair<int,MatrixXcd> >& vecPairCsdSum,
                        QMutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const QPair<MatrixXd, VectorXd>& tapers)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(inputData.vecPairCsd.size() == iNRows) {
        //qDebug() << "Coherency::compute - vecPairCsd were already computed for this trial.";
        return;
    }

    //qDebug() << "Coherency::compute - vecPairCsdSum and matPsdSum are computed for this trial.";

    // Substract mean, compute tapered spectra and PSD
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    double denomPSD = tapers.second.cwiseAbs2().sum() / 2.0;

    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecTmpFreq;

    MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

    int i,j;

    inputData.matPsd = MatrixXd(iNRows, m_iNumberBinAmount);

    for (i = 0; i < iNRows; ++i) {
        // Substract mean
        rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

        // Calculate tapered spectra if not available already
        if(inputData.vecTapSpectra.size() != iNRows) {
            for(j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0,0,1,rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));;
                } else {
                    vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                }

                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.append(matTapSpectrum);
        }

        // Compute PSD (average over tapers if necessary).
        inputData.matPsd.row(i) = inputData.vecTapSpectra.at(i).block(0,m_iNumberBinStart,inputData.vecTapSpectra.at(i).rows(),m_iNumberBinAmount).cwiseAbs2().colwise().sum() / denomPSD;

        // Divide first and last element by 2 due to half spectrum
        if(m_iNumberBinStart == 0) {
            inputData.matPsd.row(i)(0) /= 2.0;
        }

        if(bNfftEven && m_iNumberBinStart + m_iNumberBinAmount >= iNFreqs) {
            inputData.matPsd.row(i).tail(1) /= 2.0;
        }
    }

    mutex.lock();

    if(matPsdSum.rows() == 0 || matPsdSum.cols() == 0) {
        matPsdSum = inputData.matPsd;
    } else {
        matPsdSum += inputData.matPsd;
    }

    mutex.unlock();

//    iTime = timer.elapsed();
//    qWarning() << QThread::currentThreadId() << "Coherency::compute timer - compute - Tapered spectra and PSD (summing):" << iTime;
//    timer.restart();

    // Compute CSD
    if(inputData.vecPairCsd.size() != iNRows) {
        inputData.vecPairCsd.clear();

        //MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);
        MatrixXcd matCsd = MatrixXcd(iNRows, m_iNumberBinAmount);

        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

        for (i = 0; i < iNRows; ++i) {
            for (j = i; j < iNRows; ++j) {
                // Compute CSD (average over tapers if necessary)
                matCsd.row(j) = inputData.vecTapSpectra.at(i).block(0,m_iNumberBinStart,inputData.vecTapSpectra.at(i).rows(),m_iNumberBinAmount).cwiseProduct(inputData.vecTapSpectra.at(j).block(0,m_iNumberBinStart,inputData.vecTapSpectra.at(j).rows(),m_iNumberBinAmount).conjugate()).colwise().sum() / denomCSD;

                // Divide first and last element by 2 due to half spectrum
                if(m_iNumberBinStart == 0) {
                    matCsd.row(j)(0) /= 2.0;
                }

                if(bNfftEven && m_iNumberBinStart + m_iNumberBinAmount >= iNFreqs) {
                    matCsd.row(j).tail(1) /= 2.0;
                }
            }

            inputData.vecPairCsd.append(QPair<int,MatrixXcd>(i,matCsd));
        }

        mutex.lock();

        if(vecPairCsdSum.isEmpty()) {
            vecPairCsdSum = inputData.vecPairCsd;
        } else {
            for (j = 0; j < vecPairCsdSum.size(); ++j) {
                vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
            }
        }

        mutex.unlock();
    }

//    iTime = timer.elapsed();
//    qWarning() << QThread::currentThreadId() << "Coherency::compute timer - compute - CSD summing:" << iTime;
//    timer.restart();

    //Do not store data to save memory
    if(!m_bStorageModeIsActive) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
    }

//    iTime = timer.elapsed();
//    qWarning() << QThread::currentThreadId() << "Coherency::compute timer - compute - Deleting data:" << iTime;
//    timer.restart();
}

//=============================================================================================================

void Coherency::computePSDCSDAbs(QMutex& mutex,
                                 Network& finalNetwork,
                                 const QPair<int,MatrixXcd>& pairInput,
                                 const MatrixXd& matPsdSum)
{
    MatrixXd matPSDtmp(matPsdSum.rows(), matPsdSum.cols());
    RowVectorXd rowPsdSum = matPsdSum.row(pairInput.first);

    for(int j = 0; j < matPSDtmp.rows(); ++j) {
        matPSDtmp.row(j) = rowPsdSum.cwiseProduct(matPsdSum.row(j));
    }

    // Average. Note that the number of trials cancel each other out.
    MatrixXcd matCohy = pairInput.second.cwiseQuotient(matPSDtmp.cwiseSqrt());

    QSharedPointer<NetworkEdge> pEdge;
    MatrixXd matWeight;
    int j;
    int i = pairInput.first;

    for(j = i; j < matCohy.rows(); ++j) {
        matWeight = matCohy.row(j).cwiseAbs().transpose();
        pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

        mutex.lock();
        finalNetwork.getNodeAt(i)->append(pEdge);
        finalNetwork.getNodeAt(j)->append(pEdge);
        finalNetwork.append(pEdge);
        mutex.unlock();
    }
}

//=============================================================================================================

void Coherency::computePSDCSDImag(QMutex& mutex,
                                  Network& finalNetwork,
                                  const QPair<int,MatrixXcd>& pairInput,
                                  const MatrixXd& matPsdSum)
{
    MatrixXd matPSDtmp(matPsdSum.rows(), matPsdSum.cols());
    RowVectorXd rowPsdSum = matPsdSum.row(pairInput.first);

    for(int j = 0; j < matPSDtmp.rows(); ++j) {
        matPSDtmp.row(j) = rowPsdSum.cwiseProduct(matPsdSum.row(j));
    }

    MatrixXcd matCohy = pairInput.second.cwiseQuotient(matPSDtmp.cwiseSqrt());

    QSharedPointer<NetworkEdge> pEdge;
    MatrixXd matWeight;
    int j;
    int i = pairInput.first;

    for(j = i; j < matCohy.rows(); ++j) {
        matWeight = matCohy.row(j).imag().transpose();
        pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

        mutex.lock();
        finalNetwork.getNodeAt(i)->append(pEdge);
        finalNetwork.getNodeAt(j)->append(pEdge);
        finalNetwork.append(pEdge);
        mutex.unlock();
    }
}
