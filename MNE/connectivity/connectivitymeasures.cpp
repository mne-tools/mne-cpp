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
* Copyright (C) 2016, Your name and Matti Hamalainen. All rights reserved.
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

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


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

Network::SPtr ConnectivityMeasures::crossCorrelation(const MatrixXd& matData, const MatrixX3f& matVert)
{
    Network::SPtr finalNetwork  = Network::SPtr(new Network());

    //Create nodes
    for(int i = 0; i < matData.rows(); ++i)
    {
        RowVectorXf rowVert = RowVectorXf::Zero(3);
        if(matVert.rows() != 0) {
            rowVert(0) = matVert.row(i)(0);
            rowVert(1) = matVert.row(i)(1);
            rowVert(2) = matVert.row(i)(2);
        }

        *finalNetwork << NetworkNode::SPtr(new NetworkNode(i, rowVert));
    }

    //Create edges
    for(int i = 0; i < matData.rows(); ++i)
    {
        for(int j = i; j < matData.rows(); ++j)
        {
            QPair<int,double> crossCorrPair = eigenCrossCorrelation(matData.row(i), matData.row(j));

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork->getNodes()[i], finalNetwork->getNodes()[j], crossCorrPair.second));

            *finalNetwork->getNodes()[i] << pEdge;
            *finalNetwork << pEdge;
        }
    }

//    finalNetwork.scale();
//    matDist /= matDist.maxCoeff();

    return finalNetwork;
}


//*************************************************************************************************************

Eigen::MatrixXd ConnectivityMeasures::crossCorrelation(const MatrixXd& matData)
{
    MatrixXd matDist(matData.rows(), matData.rows());
    matDist.setZero();

    for(int i = 0; i < matDist.rows(); ++i)
    {
        QPair<int,double> crossCorrPair = eigenCrossCorrelation(matData.row(i), matData.row(i));

        double dScaling = crossCorrPair.second;

        for(int j = i; j < matDist.rows(); ++j)
        {
            QPair<int,double> crossCorrPair = eigenCrossCorrelation(matData.row(i), matData.row(j));
            matDist(i,j) = crossCorrPair.second/* / dScaling*/;
        }
    }

    matDist /= matDist.maxCoeff();

    return matDist;
}


//*************************************************************************************************************

QPair<int,double> ConnectivityMeasures::eigenCrossCorrelation(const RowVectorXd& xCorrInputVecFirstIn, const RowVectorXd& xCorrInputVecSecondIn)
//std::pair<double, double> ConnectivityMeasures::eigenCrossCorrelation(std::vector<double>& xCorrInputVecFirst, std::vector<double>& xCorrInputVecSecond)
{
//    Eigen::FFT<double> fft;
//    int N = std::max(xCorrInputVecFirst.size(), xCorrInputVecSecond.size());

//    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
//    int b = ceil(log2(2.0 * N - 1));
//    int fftsize = pow(2,b);
//    int end = fftsize - 1;
//    int maxlag = N - 1;
//    size_t firstSize = xCorrInputVecFirst.size();
//    size_t secondSize = xCorrInputVecSecond.size();

//    //Zero Padd
//    for (int i = xCorrInputVecFirst.size(); i < fftsize; i++)
//    {
//        xCorrInputVecFirst.push_back(0);
//    }

//    for (int i = xCorrInputVecSecond.size(); i < fftsize; i++)
//    {
//        xCorrInputVecSecond.push_back(0);
//    }

//    std::vector<std::complex<double> > freqvec;
//    std::vector<std::complex<double> > freqvec2;

//    //FFT for freq domain to both vectors
//    fft.fwd( freqvec,xCorrInputVecFirst);
//    fft.fwd( freqvec2,xCorrInputVecSecond);

//    //Main step of cross corr
//    for (int i = 0; i < fftsize; i++)
//    {
//        freqvec[i] = freqvec[i] * std::conj(freqvec2[i]);
//    }

//    std::vector<double> result;
//    fft.inv(result, freqvec);

//    //Will get rid of extra zero padding and move minus lags to beginning without copy
//    std::vector<double> result2(std::make_move_iterator(result.begin() + end - maxlag + 1),
//                                std::make_move_iterator(result.end()));

//    result2.insert(result2.end(), make_move_iterator(result.begin())
//                   , make_move_iterator(result.begin()+maxlag));


//    auto minMaxRange = std::minmax_element(result2.begin(),
//                        result2.end());

//    //Will take back the changes which made in input vector
//    if (xCorrInputVecFirst.size() != firstSize)
//        xCorrInputVecFirst.resize(firstSize);
//    if (xCorrInputVecSecond.size() != secondSize)
//        xCorrInputVecSecond.resize(secondSize);


//    //Return val
//    auto resultIndex = ((minMaxRange.second - result2.begin()) - N + 1);

//    auto maxValue = result[minMaxRange.second - result.begin()];
//    return std::make_pair (resultIndex, maxValue);

    Eigen::FFT<double> fft;

    int N = std::max(xCorrInputVecFirstIn.cols(), xCorrInputVecSecondIn.cols());

    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
    int b = ceil(log2(2.0 * N - 1));
    int fftsize = pow(2,b);
    int end = fftsize - 1;
    int maxlag = N - 1;

    //Zero Padd
    RowVectorXd xCorrInputVecFirst(fftsize);
    xCorrInputVecFirst.setZero();
    xCorrInputVecFirst.head(xCorrInputVecFirstIn.cols()) = xCorrInputVecFirstIn;

    RowVectorXd xCorrInputVecSecond(fftsize);
    xCorrInputVecSecond.setZero();
    xCorrInputVecSecond.head(xCorrInputVecSecondIn.cols()) = xCorrInputVecSecondIn;

    //FFT for freq domain to both vectors
    RowVectorXcd freqvec;
    RowVectorXcd freqvec2;

    fft.fwd(freqvec, xCorrInputVecFirst);
    fft.fwd(freqvec2, xCorrInputVecSecond);

    //Create conjugate complex
    freqvec2.conjugate();

    //Main step of cross corr
    for (int i = 0; i < fftsize; i++)
    {
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
