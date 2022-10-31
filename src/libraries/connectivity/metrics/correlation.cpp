//=============================================================================================================
/**
 * @file     correlation.cpp
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
 * @brief    Correlation class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "correlation.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QElapsedTimer>
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

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Correlation::Correlation()
{
}

//=============================================================================================================

Network Correlation::calculate(ConnectivitySettings& connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    Network finalNetwork("COR");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "Correlation::calculate - Input data is empty";
        return finalNetwork;
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

//    iTime = timer.elapsed();
//    qWarning() << "Preparation" << iTime;
//    timer.restart();

    // Calculate connectivity matrix over epochs and average afterwards
//    double dScalingStep = 1.0/matDataList.size();
//    dataTemp.matInputData = dScalingStep * (i+1) * matDataList.at(i);

    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(connectivitySettings.getTrialData(),
                                                              compute,
                                                              reduce);
    resultMat.waitForFinished();

    MatrixXd matDist = resultMat.result();

//    MatrixXd matDist;

//    for(int i = 0; i < connectivitySettings.getTrialData().size(); ++i) {
//        reduce(matDist, compute(connectivitySettings.getTrialData().at(i)));
//    }

//    matDist /= connectivitySettings.size();

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

MatrixXd Correlation::compute(const ConnectivitySettings::IntermediateTrialData& inputData)
{
    MatrixXd matDist = MatrixXd::Zero(inputData.matData.rows(), inputData.matData.rows());
    RowVectorXd vecRow;
    int j;

    matDist = inputData.matData * inputData.matData.transpose();

//    for(int i = 0; i < inputData.matData.rows(); ++i) {
//        vecRow = inputData.matData.row(i);

//        for(j = i; j < inputData.matData.rows(); ++j) {
//            matDist(i,j) += (vecRow.dot(inputData.matData.row(j))/vecRow.cols());
//        }
//    }

    return matDist;
}

//=============================================================================================================

void Correlation::reduce(MatrixXd &resultData,
                         const MatrixXd &data)
{
    if(resultData.rows() != data.rows() || resultData.cols() != data.cols()) {
        resultData.resize(data.rows(), data.cols());
        resultData.setZero();
    }

    resultData += data;
}
