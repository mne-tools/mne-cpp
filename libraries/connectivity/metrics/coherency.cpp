//=============================================================================================================
/**
* @file     coherency.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"

#include <utils/spectral.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Coherency::Coherency()
{
}


//*************************************************************************************************************

void Coherency::computeCoherency(QVector<QPair<int,MatrixXcd> >& vecCoherency,
                                 const QList<MatrixXd> &matDataList,
                                 int iNfft,
                                 const QString &sWindowType)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    // Check that iNfft >= signal length
    int iSignalLength = matDataList.at(0).cols();
    if (iNfft < iSignalLength) {
        iNfft = iSignalLength;
    }

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, sWindowType);

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = matDataList.at(0).rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    std::function<AbstractMetricResultData(const MatrixXd&)> computeLambda = [&](const MatrixXd& matInputData) {
        return compute(matInputData,
                       iNRows,
                       iNFreqs,
                       iNfft,
                       tapers);
    };

//    // Sequential
//    AbstractMetricResultData finalResult;

//    for (int i = 0; i < matDataList.length(); ++i) {
//        reduce(finalResult, computeLambda(matDataList.at(i)));
//    }

    // Parallel
//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Preparation:" << iTime;
//    timer.restart();

    QFuture<AbstractMetricResultData> result = QtConcurrent::mappedReduced(matDataList,
                                                                           computeLambda,
                                                                           reduce);
    result.waitForFinished();

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Parallel computation:" << iTime;
//    timer.restart();

    AbstractMetricResultData finalResult = result.result();

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Result copy:" << iTime;
//    timer.restart();

    finalResult.matPsdAvg = finalResult.matPsdAvg.cwiseSqrt();

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Cwise sqrt:" << iTime;
//    timer.restart();

    // Compute CSD/(PSD_X * PSD_Y)
    std::function<void(QPair<int,MatrixXcd>&)> computePSDCSDLambda = [&](QPair<int,MatrixXcd>& pairInput) {
        computePSDCSD(pairInput,
                      finalResult.matPsdAvg);
    };

    QFuture<void> resultCSDPSD = QtConcurrent::map(finalResult.vecPairCsdAvg,
                                                   computePSDCSDLambda);
    resultCSDPSD.waitForFinished();

    vecCoherency = finalResult.vecPairCsdAvg;

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Cwise abs,produce, quotient:" << iTime;
//    timer.restart();
}


//*************************************************************************************************************

AbstractMetricResultData Coherency::compute(const MatrixXd& matInputData,
                                            int iNRows,
                                            int iNFreqs,
                                            int iNfft,
                                            const QPair<MatrixXd, VectorXd>& tapers)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    // Remove mean
    MatrixXd data = matInputData;
    int i;

    for (i = 0; i < data.rows(); ++i) {
        data.row(i).array() -= data.row(i).mean();
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Remove mean:" << iTime;
//    timer.restart();

    // Compute tapered spectra and PSD
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    QVector<Eigen::MatrixXcd> vecTapSpectra;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    double denomPSD = tapers.second.cwiseAbs2().sum() / 2.0;

    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecTmpFreq;

    MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

    int j;

    AbstractMetricResultData resultData;
    resultData.matPsdAvg = MatrixXd(iNRows, iNFreqs);

    for (i = 0; i < iNRows; ++i) {
        rowData = data.row(i);

        // FFT for freq domain returning the half spectrum and multiply taper weights
        for(j = 0; j < tapers.first.rows(); j++) {
            vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
            fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
            matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
        }

        vecTapSpectra.append(matTapSpectrum);

        // Compute PSD (average over tapers if necessary).
        resultData.matPsdAvg.row(i) = matTapSpectrum.cwiseAbs2().colwise().sum() / denomPSD;

        // Divide first and last element by 2 due to half spectrum
        resultData.matPsdAvg.row(i)(0) /= 2.0;
        if(bNfftEven) {
            resultData.matPsdAvg.row(i).tail(1) /= 2.0;
        }
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Tapered spectra and PSD:" << iTime;
//    timer.restart();

    // Compute CSD
    // This code was copied and modified from Utils/Spectra since we do not want to call the function due to time loss.
    double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

    MatrixXcd matCsd(iNRows, iNFreqs);

    for (i = 0; i < iNRows; ++i) {
        for (j = i; j < iNRows; ++j) {
            // Compute CSD (average over tapers if necessary)
            matCsd.row(j) = vecTapSpectra.at(i).cwiseProduct(vecTapSpectra.at(j).conjugate()).colwise().sum() / denomCSD;

            // Divide first and last element by 2 due to half spectrum
            matCsd.row(j)(0) /= 2.0;
            if(bNfftEven) {
                matCsd.row(j).tail(1) /= 2.0;
            }
        }

        resultData.vecPairCsdAvg.append(QPair<int,MatrixXcd>(i,matCsd));
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - CSD from spectra:" << iTime;
//    timer.restart();

    return resultData;
}


//*************************************************************************************************************

void Coherency::reduce(AbstractMetricResultData& finalData,
                       const AbstractMetricResultData& resultData)
{
    // Sum over epoch
    if(finalData.matPsdAvg.rows() == 0 || finalData.matPsdAvg.cols() == 0) {
        finalData.matPsdAvg = resultData.matPsdAvg;
    } else {
        finalData.matPsdAvg += resultData.matPsdAvg;
    }

    if(finalData.vecPairCsdAvg.isEmpty()) {
        finalData.vecPairCsdAvg = resultData.vecPairCsdAvg;
    } else {
        for (int j = 0; j < finalData.vecPairCsdAvg.size(); ++j) {
            finalData.vecPairCsdAvg[j].second += resultData.vecPairCsdAvg.at(j).second;
        }
    }
}



//*************************************************************************************************************

void Coherency::computePSDCSD(QPair<int,MatrixXcd>& pairInput,
                              const MatrixXd& matPsdAvg)
{
    MatrixXd matPSDtmp(matPsdAvg.rows(), matPsdAvg.cols());
    RowVectorXd vecPsdAvg = matPsdAvg.row(pairInput.first);

    for(int j = 0; j < matPSDtmp.rows(); ++j) {
        matPSDtmp.row(j) = vecPsdAvg.cwiseProduct(matPsdAvg.row(j));
    }

    pairInput.second = pairInput.second.cwiseQuotient(matPSDtmp);
}
