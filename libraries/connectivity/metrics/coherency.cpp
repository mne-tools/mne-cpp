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

void Coherency::computeCoherency(QVector<MatrixXcd>& vecCoherency,
                                 const QList<MatrixXd> &matDataList,
                                 int iNfft,
                                 const QString &sWindowType)
{
//    QElapsedTimer timer;
//    int iTime = 0;
//    timer.start();

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

    MatrixXd matPSDtmp = MatrixXd::Zero(iNRows, iNFreqs);
    RowVectorXd vecPsdAvg;
    int j;
    for(int i = 0; i < iNRows; ++i) {
        vecPsdAvg = finalResult.matPsdAvg.row(i);

        for(j = 0; j < iNRows; ++j) {
            matPSDtmp.row(j) = vecPsdAvg.cwiseProduct(finalResult.matPsdAvg.row(j));
        }

        vecCoherency.append(finalResult.vecCsdAvg.at(i).cwiseQuotient(matPSDtmp));
    }

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

    // Generate tapered spectra, PSD, and CSD
    AbstractMetricResultData resultData;
    resultData.matPsdAvg = MatrixXd::Zero(iNRows, iNFreqs);

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Preparation:" << iTime;
//    timer.restart();

    // Remove mean
    MatrixXd data = matInputData;
    for (int i = 0; i < data.rows(); ++i) {
        data.row(i).array() -= data.row(i).mean();
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Remove mean:" << iTime;
//    timer.restart();

    // Compute tapered spectra. Note: Multithread option to false as default because nested multithreading is not permitted in qt.
    QVector<Eigen::MatrixXcd> vecTapSpectra;
    Spectral::computeTaperedSpectraMatrix(vecTapSpectra,
                                          data,
                                          tapers.first,
                                          iNfft,
                                          false);

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Tapered spectra:" << iTime;
//    timer.restart();

    // This part could be parallelized with QtConcurrent::mappedReduced
    double denomPSD = tapers.second.cwiseAbs2().sum() / 2.0;
    double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

    MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);
    int k;

    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    for (int j = 0; j < iNRows; ++j) {
        // Multiply tapers
        vecTapSpectra[j] = tapers.second.asDiagonal() * vecTapSpectra.at(j);

        //Compute PSD (average over tapers if necessary)
        resultData.matPsdAvg.row(j) = vecTapSpectra.at(j).cwiseAbs2().colwise().sum() / denomPSD;

        resultData.matPsdAvg.row(j)(0) /= 2.0;
        if(bNfftEven) {
            resultData.matPsdAvg.row(j).tail(1) /= 2.0;
        }

        // Compute PSD (average over tapers if necessary)
        for (k = j; k < iNRows; ++k) {
            // Compute PSD (average over tapers if necessary)
            matCsd.row(k) = vecTapSpectra.at(j).cwiseProduct(vecTapSpectra.at(k).conjugate()).colwise().sum() / denomCSD;

            //multiply first and last element by 2 due to half spectrum
            matCsd.row(k)(0) /= 2.0;
            if(bNfftEven) {
                matCsd.row(k).tail(1) /= 2.0;
            }
        }

        resultData.vecCsdAvg.append(matCsd);
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - PSD/CSD from spectra:" << iTime;
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

    if(finalData.vecCsdAvg.isEmpty()) {
        finalData.vecCsdAvg = resultData.vecCsdAvg;
    } else {
        for (int j = 0; j < finalData.vecCsdAvg.size(); ++j) {
            finalData.vecCsdAvg[j] += resultData.vecCsdAvg.at(j);
        }
    }
}
