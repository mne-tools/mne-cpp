//=============================================================================================================
/**
* @file     coherency.cpp
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
* - QtConcurrent can be used to speed up computation.
*
* @brief     Coherency class declaration.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
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

Coherency::Coherency()
{
}


//*************************************************************************************************************

QVector<MatrixXcd> Coherency::computeCoherency(const QList<MatrixXd> &matDataList,
                                               int iNfft,
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

    // Generate tapered spectra, PSD, and CSD and sum over epoch
    // This part could be parallelized with QtConcurrent::mappedReduced
    QList<AbstractMetricInputData> lData;
    for (int i = 0; i < matDataList.length(); ++i) {
        AbstractMetricInputData dataTemp;
        dataTemp.matInputData = matDataList.at(i);
        dataTemp.iNRows = iNRows;
        dataTemp.iNFreqs = iNFreqs;
        dataTemp.iNfft = iNfft;
        dataTemp.tapers = tapers;

        lData.append(dataTemp);
    }

    QFuture<AbstractMetricResultData> result = QtConcurrent::mappedReduced(lData, compute, reduce);
    result.waitForFinished();

    AbstractMetricResultData finalResult = result.result();

    QVector<MatrixXcd> vecCoherency;
    finalResult.matPsdAvg = finalResult.matPsdAvg.cwiseSqrt();

    for (int i = 0; i < iNRows; ++i) {
        MatrixXd matPSDtmp = MatrixXd::Zero(iNRows, iNFreqs);
        for(int j = 0; j < iNRows; ++j){
            matPSDtmp.row(j) = finalResult.matPsdAvg.row(i).cwiseProduct(finalResult.matPsdAvg.row(j));
        }
        vecCoherency.append(finalResult.vecCsdAvg.at(i).cwiseQuotient(matPSDtmp));
    }

    return vecCoherency;
}


//*************************************************************************************************************

AbstractMetricResultData Coherency::compute(const AbstractMetricInputData& inputData)
{
    MatrixXd data = inputData.matInputData;

    MatrixXd matPsdAvg = MatrixXd::Zero(inputData.iNRows, inputData.iNFreqs);

    QVector<MatrixXcd> vecCsdAvg;

    //Remove mean
    for (int i = 0; i < data.rows(); ++i) {
        data.row(i).array() -= data.row(i).mean();
    }

    // This part could be parallelized with QtConcurrent::mapped
    QVector<MatrixXcd> vecTapSpectra;
    for (int j = 0; j < inputData.iNRows; ++j) {
        MatrixXcd matTmpSpectra = Spectral::computeTaperedSpectra(data.row(j),
                                                                  inputData.tapers.first,
                                                                  inputData.iNfft);
        vecTapSpectra.append(matTmpSpectra);
    }

    // This part could be parallelized with QtConcurrent::mappedReduced
    for (int j = 0; j < inputData.iNRows; ++j) {
        RowVectorXd vecTmpPsd = Spectral::psdFromTaperedSpectra(vecTapSpectra.at(j),
                                                                inputData.tapers.second,
                                                                inputData.iNfft,
                                                                1.0);
        matPsdAvg.row(j) = vecTmpPsd;
    }

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

        vecCsdAvg.append(matCsd);
    }

    AbstractMetricResultData resultData;
    resultData.iNFreqs = inputData.iNFreqs;
    resultData.iNRows = inputData.iNRows;
    resultData.matPsdAvg = matPsdAvg;
    resultData.vecCsdAvg = vecCsdAvg;

    return resultData;
}


//*************************************************************************************************************

void Coherency::reduce(AbstractMetricResultData& finalData,
                       const AbstractMetricResultData& resultData)
{
    if(finalData.matPsdAvg.rows() == 0 || finalData.matPsdAvg.cols() == 0) {
        finalData.matPsdAvg = resultData.matPsdAvg;
    } else {
        finalData.matPsdAvg += resultData.matPsdAvg;
    }

    if(finalData.vecCsdAvg.isEmpty()) {
        finalData.vecCsdAvg = resultData.vecCsdAvg;
    } else {
        for (int j = 0; j < finalData.vecCsdAvg.size(); ++j) {
            finalData.vecCsdAvg[j] += resultData.vecCsdAvg.at(j);
        }
    }
}





