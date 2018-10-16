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
    int iNfft = matDataList.at(0).cols();

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

    return finalNetwork;
}


//*************************************************************************************************************

MatrixXd CrossCorrelation::calculate(const MatrixXd& matInputData,
                                     int iNfft,
                                     const QPair<MatrixXd, VectorXd>& tapers)
{
    // Compute tapered spectra
    // This code was copied and modified from Utils/Spectra since we do not want to call the function due to time loss.
    bool bNfftEven = false;
    if (iNfft % 2 == 0){
        bNfftEven = true;
    }

    QVector<Eigen::MatrixXcd> vecTapSpectra;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    double denom = tapers.second.sum() / 2.0;
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecResultFreq;

    MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

    int i, j;

    for (i = 0; i < matInputData.rows(); ++i) {
        rowData = matInputData.row(i).cwiseAbs();

        // FFT for freq domain returning the half spectrum and multiply taper weights
        for(j = 0; j < tapers.first.rows(); j++) {
            vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
            fft.fwd(vecResultFreq, vecInputFFT, iNfft);
            matTapSpectrum.row(j) = vecResultFreq * tapers.second(j);
        }

        // Average over columns
        vecTapSpectra.append(matTapSpectrum.colwise().sum() / denom);
    }

    // Perform multiplication and transform back to time domain to find max XCOR coefficient
    MatrixXd matDist = MatrixXd::Zero(matInputData.rows(), matInputData.rows());
    RowVectorXd vecResultTime;
    RowVectorXcd vecRowFreq;
    int idx = 0;

    for(i = 0; i < vecTapSpectra.size(); ++i) {
        vecRowFreq = vecTapSpectra.at(i);

        for(j = i; j < vecTapSpectra.size(); ++j) {
            vecResultFreq = vecRowFreq.array() * vecTapSpectra.at(j).conjugate().array();

            fft.inv(vecResultTime, vecResultFreq, iNfft);

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

