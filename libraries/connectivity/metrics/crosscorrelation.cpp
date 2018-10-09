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
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

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

    std::function<MatrixXd(const MatrixXd&)> calculateLambda = [&](const MatrixXd& matInputData) {
        return calculate(matInputData,
                       iNfft,
                       tapers);
    };

    // Calculate connectivity matrix over epochs and average afterwards
    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(matDataList,
                                                              calculateLambda,
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

MatrixXd CrossCorrelation::calculate(const MatrixXd& matInputData,
                                     int iNfft,
                                     const QPair<MatrixXd, VectorXd>& tapers)
{
    // Compute tapered spectra. Note: Multithread option to false as default because nested multithreading is not permitted in qt.
    QVector<Eigen::MatrixXcd> vecTapSpectra = Spectral::computeTaperedSpectraMatrix(matInputData,
                                                                                    tapers.first,
                                                                                    iNfft,
                                                                                    false);
    QVector<Eigen::RowVectorXcd> vecAvrSpectra;
    RowVectorXcd temp;
    double denom = tapers.second.sum() / 2.0;

    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    for(int i = 0; i < vecTapSpectra.size(); ++i) {
        temp = (tapers.second.asDiagonal() * vecTapSpectra.at(i)).colwise().sum() / denom;

        temp(0) /= 2.0;
        if(bNfftEven) {
            temp.tail(1) /= 2.0;
        }

        vecAvrSpectra << temp;
    }

    MatrixXd matDist(matInputData.rows(), matInputData.rows());
    matDist.setZero();
    RowVectorXcd vecResultFreq;
    RowVectorXd vecResultTime;
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);
    int idx = 0;
    int j;

    for(int i = 0; i < vecTapSpectra.size(); ++i) {
        idx = 0;

        for(j = i; j < vecTapSpectra.size(); ++j) {
            vecResultFreq = vecAvrSpectra.at(i).array() * vecAvrSpectra.at(j).conjugate().array();

            fft.inv(vecResultTime, vecResultFreq);

            vecResultTime.maxCoeff(&idx);

            matDist(i,j) = vecResultTime(idx);
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

