//=============================================================================================================
/**
* @file     phaselagindex.cpp
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
* @brief    PhaseLagIndex class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselagindex.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PhaseLagIndex::PhaseLagIndex()
{
}


//*******************************************************************************************************

Network PhaseLagIndex::phaseLagIndex(const QList<MatrixXd> &matDataList, const MatrixX3f& matVert)
{
    Network finalNetwork("Phase Lag Index");

    if(matDataList.empty()) {
        qDebug() << "PhaseLagIndex::phaseLagIndex - Input data is empty";
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

    //Calculate connectivity matrix over epochs and average afterwards
    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(matDataList, calculate, sum);
    resultMat.waitForFinished();

    MatrixXd matDist = resultMat.result();
    matDist /= matDataList.size();

    //Add edges to network
    for(int i = 0; i < matDist.rows(); ++i) {
        for(int j = i; j < matDist.cols(); ++j) {            
            MatrixXd matWeight(1,1);
            matWeight << abs(matDist(i,j));

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork.getNodes()[i], finalNetwork.getNodes()[j], matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

int PhaseLagIndex::calcPhaseLagIndex(const RowVectorXd& vecFirst, const RowVectorXd& vecSecond)
{
    Eigen::FFT<double> fft;

    int N = std::max(vecFirst.cols(), vecSecond.cols());

    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
    int b = ceil(log2(2.0 * N - 1));
    int fftsize = pow(2,b);

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
    freqvec = freqvec.array() * freqvec2.array();

    std::complex<double> cdCSD = freqvec.mean();

    int iSignResult = 0.0;
    for (int i = 0; i < freqvec.cols(); i++) {
        //signum and addition of all values
        if (cdCSD.imag() > 0.0) {
            iSignResult = 1.0;
        } else if (cdCSD.imag() == 0.0) {
            iSignResult = 0.0;
        } else {
            iSignResult = -1.0;
        }
    }

    return iSignResult;

//    //Hilbert function
//    Eigen::FFT<double> fft;

//    int N = std::max(vecFirst.cols(), vecSecond.cols());

//    //Compute the FFT size as the "next power of 2" of the input vector's length (max)
//    int b = ceil(log2(2.0 * N - 1));
//    int fftsize = pow(2,b);

//    //Zero Padd
//    RowVectorXd pLagInputVecFirst = RowVectorXd::Zero(fftsize);
//    pLagInputVecFirst.head(vecFirst.cols()) = vecFirst;

//    RowVectorXd pLagInputVecSecond = RowVectorXd::Zero(fftsize);
//    pLagInputVecSecond.head(vecSecond.cols()) = vecSecond;

//    //FFT for freq domain to both vectors
//    RowVectorXcd freqvec;
//    RowVectorXcd freqvec2;

//    fft.fwd(freqvec, pLagInputVecFirst);
//    fft.fwd(freqvec2, pLagInputVecSecond);

//    //removing the negative frequencies
//    RowVectorXcd freqpos;
//    RowVectorXcd freqpos2;

//    freqpos = freqvec;
//    freqpos2 = freqvec2;

//    //inverse FFT of the results
//    RowVectorXd invfreq;
//    RowVectorXd invfreq2;

//    fft.inv(invfreq, freqpos);
//    fft.inv(invfreq2, freqpos2);

//    //Hilbert function end
//    //Phase calculation
//    RowVectorXd phase;
//    RowVectorXd phase2;
//    RowVectorXd phasediff;

//    phase =  (invfreq.cwiseQuotient(pLagInputVecFirst));
//    phase2 = (invfreq2.cwiseQuotient(pLagInputVecSecond));

//    for(int i = 0; i<phase.cols(); ++i) {
//        phase[i] = atan(phase[i]);
//    }

//    for(int i = 0; i<phase2.cols(); ++i) {
//        phase2[i] = atan(phase2[i]);
//    }
//    phasediff = phase - phase2;

//    //std::cout << phasediff << std::endl;

//    //Main Phase Lag Index calculation
//    RowVectorXd signResult(phasediff.cols());

//    for (int i = 0; i < phasediff.cols(); i++) {
//        //signum and addition of all values
//        if (phasediff[i] > 0) {
//            signResult[i] = 1.0;
//        } else if (phasediff[i] == 0) {
//            signResult[i] = 0.0;
//        } else {
//            signResult[i] = -1.0;
//        }
//    }

//    return signResult;
}


//*************************************************************************************************************

MatrixXd PhaseLagIndex::calculate(const MatrixXd &data)
{
    MatrixXd matDist(data.rows(), data.rows());
    matDist.setZero();

    for(int i = 0; i < data.rows(); ++i) {
        for(int j = i; j < data.rows(); ++j) {
            matDist(i,j) += calcPhaseLagIndex(data.row(i), data.row(j));
        }
    }

    return matDist;
}


//*************************************************************************************************************

void PhaseLagIndex::sum(MatrixXd &resultData, const MatrixXd &data)
{
    if(resultData.rows() != data.rows() || resultData.cols() != data.cols()) {
        resultData.resize(data.rows(), data.cols());
        resultData.setZero();
    }

    resultData += data;
}
