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

CrossCorrelation::CrossCorrelation()
{
}


//*************************************************************************************************************

Network CrossCorrelation::calculate(ConnectivitySettings& connectivitySettings)
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    Network finalNetwork("Cross Correlation");

    if(connectivitySettings.m_dataList.empty()) {
        qDebug() << "CrossCorrelation::crossCorrelation - Input data is empty";
        return finalNetwork;
    }

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

    // Generate tapers
    int iSignalLength = connectivitySettings.m_dataList.first().matData.cols();
    int iNfft = connectivitySettings.m_iNfft;
    if (connectivitySettings.m_iNfft < iSignalLength) {
        iNfft = iSignalLength;
    }

    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iNfft, "Ones");

    // Compute the cross correlation in parallel
    QMutex mutex;
    MatrixXd matDist;

    std::function<void(ConnectivityTrialData&)> computeLambda = [&](ConnectivityTrialData& inputData) {
        compute(inputData,
                matDist,
                mutex,
                iNfft,
                tapers);
    };

    // Calculate connectivity matrix over epochs and average afterwards
    QFuture<void> resultMat = QtConcurrent::map(connectivitySettings.m_dataList,
                                                computeLambda);
    resultMat.waitForFinished();

    matDist /= connectivitySettings.size();

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

void CrossCorrelation::compute(ConnectivityTrialData& inputData,
                               MatrixXd& matDist,
                               QMutex& mutex,
                               int iNfft,
                               const QPair<MatrixXd, VectorXd>& tapers)
{

    // Calculate tapered spectra if not available already
    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecResultFreq;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    int i, j;

    if(inputData.vecTapSpectra.size() != inputData.matData.rows()) {
        inputData.vecTapSpectra.clear();

        // This code was copied and modified from Utils/Spectra since we do not want to call the function due to time loss.
        bool bNfftEven = false;
        if (iNfft % 2 == 0){
            bNfftEven = true;
        }

        double denom = tapers.second.sum() / 2.0;
        int iNFreqs = int(floor(iNfft / 2.0)) + 1;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        for (i = 0; i < inputData.matData.rows(); ++i) {
            rowData = inputData.matData.row(i).cwiseAbs();

            // FFT for freq domain returning the half spectrum and multiply taper weights
            for(j = 0; j < tapers.first.rows(); j++) {
                vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                fft.fwd(vecResultFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecResultFreq * tapers.second(j);
            }

            // Average over columns
            inputData.vecTapSpectra.append(matTapSpectrum.colwise().sum() / denom);
        }
    }

    // Perform multiplication and transform back to time domain to find max XCOR coefficient
    MatrixXd matDistTrial = MatrixXd::Zero(inputData.matData.rows(), inputData.matData.rows());
    RowVectorXd vecResultTime;
    RowVectorXcd vecRowFreq;
    int idx = 0;

    for(i = 0; i < inputData.vecTapSpectra.size(); ++i) {
        vecRowFreq = inputData.vecTapSpectra.at(i);

        for(j = i; j < inputData.vecTapSpectra.size(); ++j) {
            vecResultFreq = vecRowFreq.array() * inputData.vecTapSpectra.at(j).conjugate().array();

            fft.inv(vecResultTime, vecResultFreq, iNfft);

            vecResultTime.maxCoeff(&idx);

            matDistTrial(i,j) = vecResultTime(idx);
        }
    }

    // Sum up weights
    mutex.lock();

    if(matDist.rows() != matDistTrial.rows() || matDist.cols() != matDistTrial.cols()) {
        matDist.resize(matDistTrial.rows(), matDistTrial.cols());
        matDist.setZero();
    }

    matDist += matDistTrial;

    mutex.unlock();
}
