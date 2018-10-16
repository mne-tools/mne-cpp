//=============================================================================================================
/**
* @file     debiasedsquaredweightedphaselagindex.cpp
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
*
*
* @brief    DebiasedSquaredWeightedPhaseLagIndex class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "debiasedsquaredweightedphaselagindex.h"
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

DebiasedSquaredWeightedPhaseLagIndex::DebiasedSquaredWeightedPhaseLagIndex()
{
}


//*******************************************************************************************************

Network DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex(const QList<MatrixXd> &matDataList,
                                                                                   const MatrixX3f& matVert,
                                                                                   int iNfft,
                                                                                   const QString &sWindowType)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    Network finalNetwork("Debiased Squared Weighted Phase Lag Index");

    if(matDataList.empty()) {
        qDebug() << "DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex - Input data is empty";
        return finalNetwork;
    }

    //Create nodes
    int rows = matDataList.first().rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        if(matVert.rows() != 0 && i < matVert.rows()) {
            rowVert(0) = matVert.row(i)(0);
            rowVert(1) = matVert.row(i)(1);
            rowVert(2) = matVert.row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

//    iTime = timer.elapsed();
//    qDebug() << "DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex timer - Preparation:" << iTime;
//    timer.restart();

    //Calculate all-to-all coherence matrix over epochs
    QVector<MatrixXd> vecDebiasedSquaredWPLI;
    DebiasedSquaredWeightedPhaseLagIndex::computeDebiasedSquaredWPLI(vecDebiasedSquaredWPLI,
                                                                     matDataList,
                                                                     iNfft,
                                                                     sWindowType);

//    iTime = timer.elapsed();
//    qDebug() << "DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex timer - Actual computation:" << iTime;
//    timer.restart();

    //Add edges to network
    MatrixXd matWeight;
    QSharedPointer<NetworkEdge> pEdge;
    int j;

    for(int i = 0; i < vecDebiasedSquaredWPLI.length(); ++i) {
        for(j = i; j < matDataList.at(0).rows(); ++j) {
            matWeight = vecDebiasedSquaredWPLI.at(i).row(j).transpose();

            pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

//    iTime = timer.elapsed();
//    qDebug() << "DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex timer - Network creation:" << iTime;
//    timer.restart();

    return finalNetwork;
}


//*************************************************************************************************************

void DebiasedSquaredWeightedPhaseLagIndex::computeDebiasedSquaredWPLI(QVector<MatrixXd>& vecDebiasedSquaredWPLI,
                                                                      const QList<MatrixXd> &matDataList,
                                                                      int iNfft,
                                                                      const QString &sWindowType)
{
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

    // Initialize
    int iNRows = matDataList.at(0).rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    //    // Sequential
    //    AbstractMetricResultData finalResult;

    //    for (int i = 0; i < matDataList.length(); ++i) {
    //        reduce(finalResult, computeLambda(matDataList.at(i)));
    //    }

    std::function<AbstractMetricResultData(const MatrixXd&)> computeLambda = [&](const MatrixXd& matInputData) {
        return compute(matInputData,
                       iNRows,
                       iNFreqs,
                       iNfft,
                       tapers);
    };

    // Parallel
    QFuture<AbstractMetricResultData> result = QtConcurrent::mappedReduced(matDataList,
                                                                           computeLambda,
                                                                           reduce);
    result.waitForFinished();

    AbstractMetricResultData finalResult = result.result();

    MatrixXd matNom, matDenom;

    for (int j = 0; j < iNRows; ++j) {
        matNom = finalResult.vecCsdAvgImag.at(j).array().square();
        matNom -= finalResult.vecSquaredCsdAvgImag.at(j);

        matDenom = finalResult.vecCsdAbsAvgImag.at(j).array().square();
        matDenom -= finalResult.vecSquaredCsdAvgImag.at(j);
        matDenom = (matDenom.array() == 0.).select(INFINITY, matDenom);

        vecDebiasedSquaredWPLI.append(matNom.cwiseQuotient(matDenom));
    }
}


//*************************************************************************************************************

AbstractMetricResultData DebiasedSquaredWeightedPhaseLagIndex::compute(const MatrixXd& matInputData,
                                                                       int iNRows,
                                                                       int iNFreqs,
                                                                       int iNfft,
                                                                       const QPair<MatrixXd, VectorXd>& tapers)
{
    // Subtract mean, generate tapered spectra and CSD
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecTmpFreq;

    MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

    QVector<Eigen::MatrixXcd> vecTapSpectra;

    int i,j;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    for (i = 0; i < iNRows; ++i) {
        // Substract mean
        rowData.array() = matInputData.row(i).array() - matInputData.row(i).mean();

        // FFT for freq domain returning the half spectrum and multiply taper weights
        for(j = 0; j < tapers.first.rows(); j++) {
            vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
            fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
            matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
        }

        vecTapSpectra.append(matTapSpectrum);
    }

    // Compute CSD
    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

    MatrixXd matCsd(iNRows, iNFreqs);

    AbstractMetricResultData resultData;

    for (i = 0; i < iNRows; ++i) {
        for (j = i; j < iNRows; ++j) {
            // Compute CSD (average over tapers if necessary)
            matCsd.row(j) = vecTapSpectra.at(i).cwiseProduct(vecTapSpectra.at(j).conjugate()).colwise().sum().imag() / denomCSD;

            // Divide first and last element by 2 due to half spectrum
            matCsd.row(j)(0) /= 2.0;
            if(bNfftEven) {
                matCsd.row(j).tail(1) /= 2.0;
            }
        }

        resultData.vecCsdAvgImag.append(matCsd);
        resultData.vecCsdAbsAvgImag.append(matCsd.cwiseAbs());
        resultData.vecSquaredCsdAvgImag.append(matCsd.array().square());
    }

    return resultData;
}


//*************************************************************************************************************

void DebiasedSquaredWeightedPhaseLagIndex::reduce(AbstractMetricResultData& finalData,
                                                  const AbstractMetricResultData& resultData)
{
    // Sum over epoch
    if(finalData.vecCsdAvgImag.isEmpty()) {
        finalData.vecCsdAvgImag = resultData.vecCsdAvgImag;
    } else {
        for (int j = 0; j < finalData.vecCsdAvgImag.size(); ++j) {
            finalData.vecCsdAvgImag[j] += resultData.vecCsdAvgImag.at(j);
        }
    }

    if(finalData.vecSquaredCsdAvgImag.isEmpty()) {
        finalData.vecSquaredCsdAvgImag = resultData.vecSquaredCsdAvgImag;
    } else {
        for (int j = 0; j < finalData.vecSquaredCsdAvgImag.size(); ++j) {
            finalData.vecSquaredCsdAvgImag[j] += resultData.vecSquaredCsdAvgImag.at(j);
        }
    }

    if(finalData.vecCsdAbsAvgImag.isEmpty()) {
        finalData.vecCsdAbsAvgImag = resultData.vecCsdAbsAvgImag;
    } else {
        for (int j = 0; j < finalData.vecCsdAbsAvgImag.size(); ++j) {
            finalData.vecCsdAbsAvgImag[j] += resultData.vecCsdAbsAvgImag.at(j);
        }
    }
}


