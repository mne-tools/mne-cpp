//=============================================================================================================
/**
* @file     weightedphaselagindex.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

    if(connectivitySettings.isEmpty()) {
        qDebug() << "WeightedPhaseLagIndex::calculate - Input data is empty";
        return finalNetwork;
    }

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

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

    // Check that iNfft >= signal length
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getNumberFFT();

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdImagAbsSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

    //    iTime = timer.elapsed();
    //    qDebug() << "WeightedPhaseLagIndex::calculate timer - Preparation:" << iTime;
    //    timer.restart();

    // Compute WPLI in parallel for all trials
    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
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

void WeightedPhaseLagIndex::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                                    QVector<QPair<int,MatrixXcd> >& vecPairCsdSum,
                                    QVector<QPair<int,MatrixXd> >& vecPairCsdImagAbsSum,
                                    QMutex& mutex,
                                    int iNRows,
                                    int iNFreqs,
                                    int iNfft,
                                    const QPair<MatrixXd, VectorXd>& tapers)
{
    if(inputData.vecPairCsd.size() == iNRows &&
       inputData.vecPairCsdImagAbs.size() == iNRows ) {
        //qDebug() << "WeightedPhaseLagIndex::compute - vecPairCsd and vecPairCsdImagAbs were already computed for this trial.";
        return;
    }

    int i,j;

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if(inputData.vecTapSpectra.size() != iNRows) {
        inputData.vecTapSpectra.clear();

        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

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

    if(inputData.vecPairCsd.isEmpty()) {
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
            inputData.vecPairCsdImagAbs.append(QPair<int,MatrixXd>(i,matCsd.imag().cwiseAbs()));
        }

        mutex.lock();

        if(vecPairCsdSum.isEmpty()) {
            vecPairCsdSum = inputData.vecPairCsd;
            vecPairCsdImagAbsSum = inputData.vecPairCsdImagAbs;
        } else {
            for (int j = 0; j < vecPairCsdSum.size(); ++j) {
                vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
                vecPairCsdImagAbsSum[j].second += inputData.vecPairCsdImagAbs.at(j).second;
            }
        }

        mutex.unlock();
    } else {
        if (inputData.vecPairCsdImagAbs.isEmpty()) {
            inputData.vecPairCsdImagAbs.clear();
            for (i = 0; i < inputData.vecPairCsd.size(); ++i) {
                inputData.vecPairCsdImagAbs.append(QPair<int,MatrixXd>(i,inputData.vecPairCsd.at(i).second.imag().cwiseAbs()));
            }

            mutex.lock();

            if(vecPairCsdImagAbsSum.isEmpty()) {
                vecPairCsdImagAbsSum = inputData.vecPairCsdImagAbs;
            } else {
                for (int j = 0; j < vecPairCsdImagAbsSum.size(); ++j) {
                    vecPairCsdImagAbsSum[j].second += inputData.vecPairCsdImagAbs.at(j).second;
                }
            }

            mutex.unlock();
        }
    }
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

    for (int i = 0; i < connectivitySettings.getIntermediateSumData().vecPairCsdSum.size(); ++i) {
        matDenom = connectivitySettings.getIntermediateSumData().vecPairCsdImagAbsSum.at(i).second;
        matDenom = (matDenom.array() == 0.).select(INFINITY, matDenom);

        matNom = connectivitySettings.getIntermediateSumData().vecPairCsdSum.at(i).second.imag().cwiseAbs().cwiseQuotient(matDenom);

        for(j = i; j < matNom.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }
}

