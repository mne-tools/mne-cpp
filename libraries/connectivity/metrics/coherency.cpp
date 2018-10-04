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

QVector<MatrixXcd> Coherency::computeCoherency(const QList<MatrixXd> &matDataList,
                                               int iNfft,
                                               const QString &sWindowType)
{
    fftw_init_threads();
    fftw_plan_with_nthreads(6);
    //fftw_make_planner_thread_safe();

//    QElapsedTimer timer;
//    int iTime = 0;
//    timer.start();

    if(matDataList.isEmpty()) {
        return QVector<MatrixXcd>();
    }

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

//    // Prepare parallel processing
//    QList<AbstractMetricInputData> lData;
//    AbstractMetricInputData dataTemp;
//    dataTemp.iNRows = iNRows;
//    dataTemp.iNFreqs = iNFreqs;
//    dataTemp.iNfft = iNfft;
//    dataTemp.tapers = tapers;

//    for (int i = 0; i < matDataList.size(); ++i) {
//        dataTemp.matInputData = matDataList.at(i);

//        lData.append(dataTemp);
//    }

    QSharedPointer<FFT<double> > fft = QSharedPointer<FFT<double> >::create();
    fft->SetFlag(fft->HalfSpectrum);

    std::function<AbstractMetricResultData(const MatrixXd&)> computeLambda = [&](const MatrixXd& matInputData) {
        return compute(matInputData,
                       iNRows,
                       iNFreqs,
                       iNfft,
                       fft,
                       tapers);
    };

    // Sequential
    AbstractMetricResultData finalResult;

    for (int i = 0; i < matDataList.length(); ++i) {
        reduce(finalResult, computeLambda(matDataList.at(i)));
    }

    // Parallel
//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Preparation:" << iTime;
//    timer.restart();

//    QFuture<AbstractMetricResultData> result = QtConcurrent::mappedReduced(matDataList,
//                                                                           computeLambda,
//                                                                           reduce);
//    result.waitForFinished();

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Parallel computation:" << iTime;
//    timer.restart();

//   AbstractMetricResultData finalResult = result.result();

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Result copy:" << iTime;
//    timer.restart();

    QVector<MatrixXcd> vecCoherency;
    finalResult.matPsdAvg = finalResult.matPsdAvg.cwiseSqrt();

    for (int i = 0; i < iNRows; ++i) {
        MatrixXd matPSDtmp = MatrixXd::Zero(iNRows, iNFreqs);
        for(int j = 0; j < iNRows; ++j){
            matPSDtmp.row(j) = finalResult.matPsdAvg.row(i).cwiseProduct(finalResult.matPsdAvg.row(j));
        }
        vecCoherency.append(finalResult.vecCsdAvg.at(i).cwiseQuotient(matPSDtmp));
    }

//    iTime = timer.elapsed();
//    qDebug() << "Coherency::computeCoherency timer - Cwise abs,produce, quotient:" << iTime;
//    timer.restart();

   fftw_cleanup_threads();

    return vecCoherency;
}


//*************************************************************************************************************

AbstractMetricResultData Coherency::compute(const MatrixXd& matInputData,
                                            int iNRows,
                                            int iNFreqs,
                                            int iNfft,
                                            QSharedPointer<FFT<double> > fft,
                                            const QPair<MatrixXd, VectorXd>& tapers)
{
    QElapsedTimer timer;
    int iTime = 0;
    timer.start();

    // Generate tapered spectra, PSD, and CSD
    AbstractMetricResultData resultData;
    resultData.matPsdAvg = MatrixXd::Zero(iNRows, iNFreqs);

    iTime = timer.elapsed();
    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Preparation:" << iTime;
    timer.restart();

    // Remove mean
    MatrixXd data = matInputData;
    for (int i = 0; i < data.rows(); ++i) {
        data.row(i).array() -= data.row(i).mean();
    }

    iTime = timer.elapsed();
    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Remove mean:" << iTime;
    timer.restart();

    // Compute tapered spectra. Note: Multithread option to false as default because nested multithreading is not permitted in qt.
    QVector<Eigen::MatrixXcd> vecTapSpectra;
    Spectral::computeTaperedSpectraMatrix(vecTapSpectra,
                                          data,
                                          tapers.first,
                                          iNfft,
                                          fft,
                                          false);

    iTime = timer.elapsed();
    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - Tapered spectra:" << iTime;
    timer.restart();

    // This part could be parallelized with QtConcurrent::mappedReduced
    for (int j = 0; j < iNRows; ++j) {
        RowVectorXd vecTmpPsd = Spectral::psdFromTaperedSpectra(vecTapSpectra.at(j),
                                                                tapers.second,
                                                                iNfft,
                                                                1.0);
        resultData.matPsdAvg.row(j) = vecTmpPsd;
    }

    iTime = timer.elapsed();
    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - PSD from spectra:" << iTime;
    timer.restart();

    // This part could be parallelized with QtConcurrent::mappedReduced
    MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);
    for (int j = 0; j < iNRows; ++j) {
        for (int k = j; k < iNRows; ++k) {
            matCsd.row(k) = Spectral::csdFromTaperedSpectra(vecTapSpectra.at(j),
                                                            vecTapSpectra.at(k),
                                                            tapers.second,
                                                            tapers.second,
                                                            iNfft,
                                                            1.0);
        }

        resultData.vecCsdAvg.append(matCsd);
    }

    iTime = timer.elapsed();
    qDebug() << QThread::currentThreadId() << "Coherency::compute timer - compute - CSD from spectra:" << iTime;
    timer.restart();

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
