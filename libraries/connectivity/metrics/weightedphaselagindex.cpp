//=============================================================================================================
/**
* @file     weightedphaselagindex.cpp
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
* @brief    WeightedPhaseLagIndex class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "weightedphaselagindex.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"
#include "../connectivitysettings.h"

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

WeightedPhaseLagIndex::WeightedPhaseLagIndex()
{
}


//*******************************************************************************************************

Network WeightedPhaseLagIndex::calculate(ConnectivitySettings& connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    Network finalNetwork("Phase Lag Index");

    if(connectivitySettings.m_dataList.empty()) {
        qDebug() << "WeightedPhaseLagIndex::calculate - Input data is empty";
        return finalNetwork;
    }

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    //Create nodes
    int rows = connectivitySettings.m_dataList.first().matData.rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        rowVert = RowVectorXf::Zero(3);

        if(connectivitySettings.m_matNodePositions.rows() != 0 && i < connectivitySettings.m_matNodePositions.rows()) {
            rowVert(0) = connectivitySettings.m_matNodePositions.row(i)(0);
            rowVert(1) = connectivitySettings.m_matNodePositions.row(i)(1);
            rowVert(2) = connectivitySettings.m_matNodePositions.row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    // Check that iNfft >= signal length
    int iSignalLength = connectivitySettings.m_dataList.at(0).matData.cols();
    int iNfft = connectivitySettings.m_iNfft;
    if (iNfft < iSignalLength) {
        iNfft = iSignalLength;
    }

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.m_sWindowType);

    // Initialize
    int iNRows = connectivitySettings.m_dataList.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    QMutex mutex;

    std::function<void(ConnectivityTrialData&)> computeLambda = [&](ConnectivityTrialData& inputData) {
        compute(inputData,
                connectivitySettings.data.vecPairCsdSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

    //    iTime = timer.elapsed();
    //    qDebug() << "WeightedPhaseLagIndex::calculate timer - Preparation:" << iTime;
    //    timer.restart();

    // Compute DSWPLV in parallel for all trials
    QFuture<void> result = QtConcurrent::map(connectivitySettings.m_dataList,
                                             computeLambda);
    result.waitForFinished();

//    iTime = timer.elapsed();
//    qDebug() << "WeightedPhaseLagIndex::calculate timer - Compute WPLI per trial:" << iTime;
//    timer.restart();

    // Compute WPLI
    computeWPLI(connectivitySettings,
               finalNetwork);

//    iTime = timer.elapsed();
//    qDebug() << "WeightedPhaseLagIndex::calculate timer - Compute WPLI, Network creation:" << iTime;
//    timer.restart();

    return finalNetwork;
}


//*************************************************************************************************************

void WeightedPhaseLagIndex::compute(ConnectivityTrialData& inputData,
                            QVector<QPair<int,MatrixXcd> >& vecPairCsdSum,
                            QMutex& mutex,
                            int iNRows,
                            int iNFreqs,
                            int iNfft,
                            const QPair<MatrixXd, VectorXd>& tapers)
{
    if(inputData.vecPairCsd.size() == iNRows) {
        //qDebug() << "WeightedPhaseLagIndex::compute - vecPairCsd was already computed for this trial.";
        return;
    }

    int i,j;

    inputData.vecPairCsd.clear();

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if(inputData.vecTapSpectra.size() != iNRows) {
        inputData.vecTapSpectra.clear();

        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        QVector<Eigen::MatrixXcd> vecTapSpectra;

        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra if not available already
            for(j = 0; j < tapers.first.rows(); j++) {
                vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.append(matTapSpectrum);
        }
    }

    // Compute CSD
    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

    MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);

    for (i = 0; i < iNRows; ++i) {
        for (j = i; j < iNRows; ++j) {
            // Compute CSD (average over tapers if necessary)
            matCsd.row(j) = inputData.vecTapSpectra.at(i).cwiseProduct(inputData.vecTapSpectra.at(j).conjugate()).colwise().sum() / denomCSD;

            // Divide first and last element by 2 due to half spectrum
            matCsd.row(j)(0) /= 2.0;
            if(bNfftEven) {
                matCsd.row(j).tail(1) /= 2.0;
            }
        }

        inputData.vecPairCsd.append(QPair<int,MatrixXcd>(i,matCsd));
    }

    mutex.lock();

    if(vecPairCsdSum.isEmpty()) {
        vecPairCsdSum = inputData.vecPairCsd;
    } else {
        for (int j = 0; j < vecPairCsdSum.size(); ++j) {
            vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
        }
    }

    mutex.unlock();
}


//*************************************************************************************************************

void WeightedPhaseLagIndex::computeWPLI(ConnectivitySettings &connectivitySettings,
                                        Network& finalNetwork)
{
    // Compute final WPLI and create Network
    MatrixXd matDenom, matNom;
    MatrixXd matWeight;
    QSharedPointer<NetworkEdge> pEdge;
    int j;

    for (int i = 0; i < connectivitySettings.data.vecPairCsdSum.size(); ++i) {
        matDenom = connectivitySettings.data.vecPairCsdSum.at(i).second.imag().cwiseAbs();
        matDenom = (matDenom.array() == 0.).select(INFINITY, matDenom);

        matNom = connectivitySettings.data.vecPairCsdSum.at(i).second.imag().cwiseAbs();

        matNom = matNom.cwiseQuotient(matDenom);

        for(j = i; j < matNom.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }
}

