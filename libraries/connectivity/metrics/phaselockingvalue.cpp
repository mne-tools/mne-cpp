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

Network PhaseLockingValue::phaseLockingValue(const QList<MatrixXd> &matDataList, const MatrixX3f& matVert,
                                           int iNfft, const QString &sWindowType)
{
    Network finalNetwork("New Phase Locking Value");

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
        for(int j = 0; j < matDataList.at(0).rows(); ++j) {
            MatrixXd matWeight = vecPLV.at(i).row(j).transpose();

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork.getNodes()[i], finalNetwork.getNodes()[j], matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

QVector<MatrixXd> PhaseLockingValue::computePLV(const QList<MatrixXd> &matDataList, int iNfft,
                                                const QString &sWindowType)
{
    // Check that iNfft >= signal length
    int iSignalLength = matDataList.at(0).cols();
    if (iNfft < iSignalLength) {
        iNfft = iSignalLength;
    }

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, sWindowType);

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = matDataList.at(0).rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;
    QVector<MatrixXcd> vecCsdAvg;
    for (int j = 0; j < iNRows; ++j) {
        vecCsdAvg.append(MatrixXcd::Zero(iNRows, iNFreqs));
    }

    // Generate tapered spectra and CSD and sum over epoch
    // This part could be parallelized with QtConcurrent::mappedReduced
    for (int i = 0; i < matDataList.length(); ++i) {
        //Remove mean
        MatrixXd matInputData = matDataList.at(i);
        for (int i = 0; i < matInputData.rows(); ++i) {
            matInputData.row(i).array() -= matInputData.row(i).mean();
        }

        // This part could be parallelized with QtConcurrent::mapped
        QVector<MatrixXcd> vecTapSpectra;
        for (int j = 0; j < iNRows; ++j) {
            MatrixXcd matTmpSpectra = Spectral::computeTaperedSpectra(matInputData.row(j), tapers.first, iNfft);
            vecTapSpectra.append(matTmpSpectra);
        }

        // This part could be parallelized with QtConcurrent::mappedReduced
        for (int j = 0; j < iNRows; ++j) {
            MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);
            for (int k = 0; k < iNRows; ++k) {
                matCsd.row(k) = Spectral::csdFromTaperedSpectra(vecTapSpectra.at(j), vecTapSpectra.at(k),
                                                                tapers.second, tapers.second, iNfft, 1.0);
            }
            vecCsdAvg.replace(j, vecCsdAvg.at(j) + matCsd.cwiseQuotient(matCsd.cwiseAbs()));
        }
    }

    QVector<MatrixXd> vecPLV;
    for (int i = 0; i < iNRows; ++i) {
        vecPLV.append(vecCsdAvg.at(i).cwiseAbs() / matDataList.length());
    }
    return vecPLV;
}
