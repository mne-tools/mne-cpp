 //=============================================================================================================
/**
* @file     connectivitymeasures.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    ConnectivityMeasures class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitymeasures.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivityMeasures::ConnectivityMeasures()
{
}


//*************************************************************************************************************

Network ConnectivityMeasures::pearsonsCorrelationCoeff(const MatrixXd& matData, const MatrixX3f& matVert)
{
    Network finalNetwork("Pearson's Correlation Coefficient");

    //Create nodes
    for(int i = 0; i < matData.rows(); ++i) {
        RowVectorXf rowVert = RowVectorXf::Zero(3);

        if(matVert.rows() != 0 && i < matVert.rows()) {
            rowVert(0) = matVert.row(i)(0);
            rowVert(1) = matVert.row(i)(1);
            rowVert(2) = matVert.row(i)(2);
        }

        finalNetwork << NetworkNode::SPtr(new NetworkNode(i, rowVert));
    }

    //Create edges
    double pearsonsCoeff;

    for(int i = 0; i < matData.rows(); ++i) {
        for(int j = i; j < matData.rows(); ++j) {
            pearsonsCoeff = calcPearsonsCorrelationCoeff(matData.row(i), matData.row(j));

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork.getNodes()[i], finalNetwork.getNodes()[j], pearsonsCoeff));

            *finalNetwork.getNodeAt(i) << pEdge;
            finalNetwork << pEdge;
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

Network ConnectivityMeasures::crossCorrelation(const MatrixXd& matData, const MatrixX3f& matVert)
{
    Network finalNetwork("Cross Correlation");

    //Create nodes
    for(int i = 0; i < matData.rows(); ++i) {
        RowVectorXf rowVert = RowVectorXf::Zero(3);

        if(matVert.rows() != 0 && i < matVert.rows()) {
            rowVert(0) = matVert.row(i)(0);
            rowVert(1) = matVert.row(i)(1);
            rowVert(2) = matVert.row(i)(2);
        }

        finalNetwork << NetworkNode::SPtr(new NetworkNode(i, rowVert));
    }

    //Create edges
    QPair<int,double> crossCorrPair;

    for(int i = 0; i < matData.rows(); ++i) {
        for(int j = i; j < matData.rows(); ++j) {
            crossCorrPair = calcCrossCorrelation(matData.row(i), matData.row(j));

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork.getNodes()[i], finalNetwork.getNodes()[j], crossCorrPair.second));

            *finalNetwork.getNodeAt(i) << pEdge;
            finalNetwork << pEdge;
        }
    }

//    finalNetwork.scale();
//    matDist /= matDist.maxCoeff();

    return finalNetwork;
}


//*************************************************************************************************************

double ConnectivityMeasures::calcPearsonsCorrelationCoeff(const Eigen::RowVectorXd &vecFirst, const Eigen::RowVectorXd &vecSecond)
{
    if(vecFirst.cols() != vecSecond.cols()) {
        qDebug() << "ConnectivityMeasures::calcPearsonsCorrelationCoeff - Vectors length do not match!";
    }

    return (vecFirst.dot(vecSecond))/vecFirst.cols();
}


//*************************************************************************************************************

QPair<int,double> ConnectivityMeasures::calcCrossCorrelation(const RowVectorXd& vecFirst, const RowVectorXd& vecSecond)
{
    Eigen::FFT<double> fft;

    int N = std::max(vecFirst.cols(), vecSecond.cols());

    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
    int b = ceil(log2(2.0 * N - 1));
    int fftsize = pow(2,b);
//    int end = fftsize - 1;
//    int maxlag = N - 1;

    //Zero Padd
    RowVectorXd xCorrInputVecFirst = RowVectorXd::Zero(fftsize);
    xCorrInputVecFirst.head(vecFirst.cols()) = vecFirst;

    RowVectorXd xCorrInputVecSecond = RowVectorXd::Zero(fftsize);
    xCorrInputVecSecond.head(vecSecond.cols()) = vecSecond;

    //FFT for freq domain to both vectors
    RowVectorXcd freqvec;
    RowVectorXcd freqvec2;

    fft.fwd(freqvec, xCorrInputVecFirst);
    fft.fwd(freqvec2, xCorrInputVecSecond);

    //Create conjugate complex
    freqvec2.conjugate();

    //Main step of cross corr
    for (int i = 0; i < fftsize; i++) {
        freqvec[i] = freqvec[i] * freqvec2[i];
    }

    RowVectorXd result;
    fft.inv(result, freqvec);

    //Will get rid of extra zero padding
    RowVectorXd result2 = result;//.segment(maxlag, N);

    QPair<int,int> minMaxRange;
    int idx = 0;
    result2.minCoeff(&idx);
    minMaxRange.first = idx;
    result2.maxCoeff(&idx);
    minMaxRange.second = idx;

//    std::cout<<"result2(minMaxRange.first)"<<result2(minMaxRange.first)<<std::endl;
//    std::cout<<"result2(minMaxRange.second)"<<result2(minMaxRange.second)<<std::endl;
//    std::cout<<"b"<<b<<std::endl;
//    std::cout<<"fftsize"<<fftsize<<std::endl;
//    std::cout<<"end"<<end<<std::endl;
//    std::cout<<"maxlag"<<maxlag<<std::endl;

    //Return val
    int resultIndex = minMaxRange.second;
    double maxValue = result2(resultIndex);

    return QPair<int,double>(resultIndex, maxValue);
}
