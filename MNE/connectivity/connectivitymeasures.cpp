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

using namespace CONNECTLIB;
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

Eigen::MatrixXd ConnectivityMeasures::crossCorrelation(const Eigen::MatrixXd& matDataIn)
{
    MatrixXd matDist(matDataIn.rows(), matDataIn.rows());

    for(int i = 0; i < matDist.rows(); ++i)
    {
        for(int j = 0; j < matDist.rows(); ++j)
        {
            QPair<double, double> crossCorrPair = eigenCrossCorrelation(matDist.row(i), matDist.row(j));
            matDist(i,j) = crossCorrPair.second;
        }
    }

    return matDist;
}


//*************************************************************************************************************

QPair<double, double> ConnectivityMeasures::eigenCrossCorrelation(const RowVectorXd& xCorrInputVecFirstIn, const RowVectorXd& xCorrInputVecSecondIn)
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

    RowVectorXd xCorrInputVecFirst = xCorrInputVecFirstIn;
    RowVectorXd xCorrInputVecSecond = xCorrInputVecSecondIn;

    Eigen::FFT<double> fft;
    int N = std::max(xCorrInputVecFirst.cols(), xCorrInputVecSecond.cols());

    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
    int b = ceil(log2(2.0 * N - 1));
    int fftsize = pow(2,b);
    int end = fftsize - 1;
    int maxlag = N - 1;
    int firstSize = xCorrInputVecFirst.cols();
    int secondSize = xCorrInputVecSecond.cols();

    //Zero Padd
    for (int i = xCorrInputVecFirst.cols(); i < fftsize; ++i)
    {
        xCorrInputVecFirst[i] = 0;
    }

    for (int i = xCorrInputVecSecond.cols(); i < fftsize; ++i)
    {
        xCorrInputVecSecond[i] = 0;
    }

    VectorXcd freqvec;
    VectorXcd freqvec2;

    //FFT for freq domain to both vectors
    fft.fwd( freqvec,xCorrInputVecFirst);
    fft.fwd( freqvec2,xCorrInputVecSecond);

    //Create conjugate complex
    freqvec2.conjugate();

    //Main step of cross corr
    for (int i = 0; i < fftsize; i++)
    {
        freqvec[i] = freqvec[i] * freqvec2[i];
    }

    VectorXd result;
    fft.inv(result, freqvec);

    //Will get rid of extra zero padding and move minus lags to beginning without copy
    VectorXd result2 = result.segment( end - maxlag + 1, maxlag);

//            (std::make_move_iterator(result.begin() + end - maxlag + 1),
//                                std::make_move_iterator(result.end()));

//    result2.insert(result2.end(), make_move_iterator(result.begin())
//                   , make_move_iterator(result.begin()+maxlag));

    QPair<int,int> minMaxRange;
    int idx = 0;
    result2.minCoeff(&idx);
    minMaxRange.first = idx;
    result2.maxCoeff(&idx);
    minMaxRange.second = idx;

    //Will take back the changes which made in input vector
    if (xCorrInputVecFirst.cols() != firstSize)
    {
        xCorrInputVecFirst.resize(firstSize);
    }

    if (xCorrInputVecSecond.cols() != secondSize)
    {
        xCorrInputVecSecond.resize(secondSize);
    }

    //Return val
    auto resultIndex = minMaxRange.second;
    auto maxValue = result2(minMaxRange.second);

    return QPair<int,int>(resultIndex, maxValue);
}






