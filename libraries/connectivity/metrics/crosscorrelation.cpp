//=============================================================================================================
/**
* @file     crosscorrelation.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "crosscorrelation.h"
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

CrossCorrelation::CrossCorrelation()
{
}


//*************************************************************************************************************

Network CrossCorrelation::crossCorrelation(const QList<MatrixXd> &matDataList,
                                           const MatrixX3f& matVert)
{
    fftw_make_planner_thread_safe();

    Network finalNetwork("Cross Correlation");

    if(matDataList.empty()) {
        qDebug() << "CrossCorrelation::crossCorrelation - Input data is empty";
        return finalNetwork;
    }

    // Check that iNfft >= signal length
    int iNfft = matDataList.at(0).cols();

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

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iNfft, "Ones");

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = matDataList.at(0).rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Generate tapered spectra, PSD, and CSD and sum over epoch
    QList<AbstractMetricInputData> lData;
    for (int i = 0; i < matDataList.size(); ++i) {
        AbstractMetricInputData dataTemp;
        dataTemp.matInputData = matDataList.at(i).array().abs();
        dataTemp.iNRows = iNRows;
        dataTemp.iNFreqs = iNFreqs;
        dataTemp.iNfft = iNfft;
        dataTemp.tapers = tapers;

        lData.append(dataTemp);
    }

    // Calculate connectivity matrix over epochs and average afterwards
    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(lData,
                                                              calculate,
                                                              sum);
    resultMat.waitForFinished();

    MatrixXd matDist = resultMat.result();
    matDist /= matDataList.size();

    //Add edges to network
    for(int i = 0; i < matDist.rows(); ++i) {
        for(int j = i; j < matDist.cols(); ++j) {
            MatrixXd matWeight(1,1);
            matWeight << matDist(i,j);

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

MatrixXd CrossCorrelation::calculate(const AbstractMetricInputData& inputData)
{
    // Compute tapered spectra. Note: Multithread option to false as default because nested multithreading is not permitted in qt.
    QVector<Eigen::MatrixXcd> vecTapSpectra = Spectral::computeTaperedSpectraMatrix(inputData.matInputData,
                                                                                    inputData.tapers.first,
                                                                                    inputData.iNfft,
                                                                                    false);

    MatrixXd matDist(inputData.matInputData.rows(), inputData.matInputData.rows());
    matDist.setZero();

    for(int i = 0; i < vecTapSpectra.size(); ++i) {
        for(int j = i; j < vecTapSpectra.size(); ++j) {
            matDist(i,j) = calcCrossCorrelation(vecTapSpectra.at(i), vecTapSpectra.at(j)).second;
        }
    }

    return matDist;
}


//*************************************************************************************************************

void CrossCorrelation::sum(MatrixXd &resultData,
                           const MatrixXd &data)
{
    if(resultData.rows() != data.rows() || resultData.cols() != data.cols()) {
        resultData.resize(data.rows(), data.cols());
        resultData.setZero();
    }

    resultData += data;
}


//*************************************************************************************************************

QPair<int,double> CrossCorrelation::calcCrossCorrelation(const MatrixXcd &matDataFirst,
                                                         const MatrixXcd &matDataSecond)
{
    MatrixXcd matResultFreq = matDataSecond.array() * matDataFirst.conjugate().array();
    RowVectorXcd vecResultFreq = matResultFreq.colwise().sum();
    vecResultFreq /= matDataSecond.rows();
    RowVectorXd vecResultTime;

    Eigen::FFT<double> fft;
    fft.inv(vecResultTime, vecResultFreq);

    QPair<int,int> minMaxRange;
    int idx = 0;
    vecResultTime.minCoeff(&idx);
    minMaxRange.first = idx;
    vecResultTime.maxCoeff(&idx);
    minMaxRange.second = idx;

    //Return val
    int resultIndex = minMaxRange.second;
    double maxValue = vecResultTime(resultIndex);

    return QPair<int,double>(resultIndex, maxValue);
}

