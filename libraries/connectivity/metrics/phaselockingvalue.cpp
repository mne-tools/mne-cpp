//=============================================================================================================
/**
* @file     phaselockingvalue.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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
* @brief    PhaseLockingValue class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselockingvalue.h"
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

PhaseLockingValue::PhaseLockingValue()
{
}


//*******************************************************************************************************

Network PhaseLockingValue::phaseLockingValue(const QList<MatrixXd> &matDataList,
                                             const MatrixX3f& matVert,
                                             int iNfft,
                                             const QString &sWindowType)
{
    Network finalNetwork("Phase Locking Value");

    if(matDataList.empty()) {
        qDebug() << "PhaseLockingValue::phaseLockingValue - Input data is empty";
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

    //Calculate all-to-all coherence matrix over epochs
    QVector<MatrixXd> vecPLV = PhaseLockingValue::computePLV(matDataList, iNfft, sWindowType);

    //Add edges to network
    for(int i = 0; i < vecPLV.length(); ++i) {
        for(int j = i; j < matDataList.at(0).rows(); ++j) {
            MatrixXd matWeight = vecPLV.at(i).row(j).transpose();

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

QVector<MatrixXd> PhaseLockingValue::computePLV(const QList<MatrixXd> &matDataList,
                                                int iNfft,
                                                const QString &sWindowType)
{
    if(matDataList.isEmpty()) {
        return QVector<MatrixXd>();
    }

    // Check that iNfft >= signal length
    int iSignalLength = matDataList.at(0).cols();
    if (iNfft < iSignalLength) {
        iNfft = iSignalLength;
    }

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, sWindowType);

    int iNRows = matDataList.at(0).rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Prepare parallel processing
    QList<AbstractMetricInputData> lData;
    AbstractMetricInputData dataTemp;
    dataTemp.iNRows = iNRows;
    dataTemp.iNFreqs = iNFreqs;
    dataTemp.iNfft = iNfft;
    dataTemp.tapers = tapers;

    for (int i = 0; i < matDataList.size(); ++i) {
        dataTemp.matInputData = matDataList.at(i);

        lData.append(dataTemp);
    }

//    // Sequential
//    AbstractMetricResultData finalResult;

//    for (int i = 0; i < lData.length(); ++i) {
//        reduce(finalResult, compute(lData.at(i)));
//    }

    // Parallel
    QFuture<AbstractMetricResultData> result = QtConcurrent::mappedReduced(lData,
                                                                           compute,
                                                                           reduce);
    result.waitForFinished();

    AbstractMetricResultData finalResult = result.result();

    QVector<MatrixXd> vecPLV;
    for (int i = 0; i < finalResult.vecCsdAvg.size(); ++i) {
        vecPLV.append(finalResult.vecCsdAvg.at(i).cwiseAbs() / matDataList.length());
    }

    return vecPLV;
}

//*************************************************************************************************************

AbstractMetricResultData PhaseLockingValue::compute(const AbstractMetricInputData& inputData)
{
    // Initialize vecCsdAvg
    QVector<MatrixXcd> vecCsdAvg;

    // Generate tapered spectra and CSD
    // Remove mean
    MatrixXd matInputData = inputData.matInputData;
    for (int i = 0; i < matInputData.rows(); ++i) {
        matInputData.row(i).array() -= matInputData.row(i).mean();
    }

    // This part could be parallelized with QtConcurrent::mapped
    QVector<MatrixXcd> vecTapSpectra = Spectral::computeTaperedSpectraMatrix(matInputData,
                                                                             inputData.tapers.first,
                                                                             inputData.iNfft,
                                                                             false);

    // This part could be parallelized with QtConcurrent::mappedReduced
    for (int j = 0; j < inputData.iNRows; ++j) {
        MatrixXcd matCsd = MatrixXcd(inputData.iNRows, inputData.iNFreqs);
        for (int k = 0; k < inputData.iNRows; ++k) {
            matCsd.row(k) = Spectral::csdFromTaperedSpectra(vecTapSpectra.at(j),
                                                            vecTapSpectra.at(k),
                                                            inputData.tapers.second,
                                                            inputData.tapers.second,
                                                            inputData.iNfft,
                                                            1.0);
        }

        vecCsdAvg.append(matCsd.cwiseQuotient(matCsd.cwiseAbs()));
    }

    AbstractMetricResultData resultData;
    resultData.iNFreqs = inputData.iNFreqs;
    resultData.iNRows = inputData.iNRows;
    resultData.vecCsdAvg = vecCsdAvg;

    return resultData;
}


//*************************************************************************************************************

void PhaseLockingValue::reduce(AbstractMetricResultData& finalData,
                               const AbstractMetricResultData& resultData)
{
    // Sum over epoch
    if(finalData.vecCsdAvg.isEmpty()) {
        finalData.vecCsdAvg = resultData.vecCsdAvg;
    } else {
        for (int j = 0; j < finalData.vecCsdAvg.size(); ++j) {
            finalData.vecCsdAvg[j] += resultData.vecCsdAvg.at(j);
        }
    }
}
