//=============================================================================================================
/**
 * @file     signalmodel.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief    signalModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "signalmodel.h"
#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qmath.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SignalModel::SignalModel(const Frequencies frequencies, bool bBasicModel)
{
    m_iCurrentModelCols = 0;
    m_frequencies = frequencies;
    m_bBasicModel = bBasicModel;
}

//=============================================================================================================

MatrixXd SignalModel::fitData(const MatrixXd& matData)
{
    bool bDimensionsChanged = checkDataDimensions(matData);
    if(bDimensionsChanged) {
        selectModelAndCompute(m_bBasicModel);
    }
    MatrixXd matTopo = m_matInverseSignalModel * matData.transpose();
    return matTopo;
}

//=============================================================================================================

void SignalModel::selectModelAndCompute(bool bBasicModel)
{
    if(bBasicModel) {
        computeInverseBasicModel();
    } else {
        computeInverseAdvancedModel();
    }
}

//=============================================================================================================

void SignalModel::computeInverseBasicModel()
{
    int iNumCoils = m_frequencies.vecHpiFreqs.size();
    MatrixXd matSimsig;
    VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/m_frequencies.iSampleFreq;

    // Generate simulated data Matrix
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*2);

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_frequencies.vecHpiFreqs[i]*vecTime.array());
    }
    m_matInverseSignalModel = UTILSLIB::MNEMath::pinv(matSimsig);
}

//=============================================================================================================

void SignalModel::computeInverseAdvancedModel()
{
    int iNumCoils = m_frequencies.vecHpiFreqs.size();
    MatrixXd matSimsig;
    MatrixXd matSimsigInvTemp;

    VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/m_frequencies.iSampleFreq;

    // add linefreq + harmonics + DC part to model
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*4+2);
    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*m_frequencies.iLineFreq*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*m_frequencies.iLineFreq*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(m_iCurrentModelCols, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);
    matSimsigInvTemp = UTILSLIB::MNEMath::pinv(matSimsig);
    m_matInverseSignalModel = matSimsigInvTemp.block(0,0,iNumCoils*2,m_iCurrentModelCols);
}

//=============================================================================================================

bool SignalModel::checkDataDimensions(const MatrixXd& matData)
{
    bool bHasChanged = false;
    if(matData.cols() != m_iCurrentModelCols) {
        m_iCurrentModelCols = matData.cols();
        bHasChanged = true;
    }
    return bHasChanged;
}

//=============================================================================================================

void SignalModel::setModelType(const bool bBasicModel)
{
    if(m_bBasicModel != bBasicModel)
    {
        m_bBasicModel = bBasicModel;
        selectModelAndCompute(bBasicModel);
    }
}

//=============================================================================================================

void SignalModel::updateFrequencies(const Frequencies frequencies)
{
    bool bHasChanged = checkFrequencies(frequencies);
    if(bHasChanged)
    {
        m_frequencies.iSampleFreq = frequencies.iSampleFreq;
        m_frequencies.iLineFreq = frequencies.iLineFreq;
        m_frequencies.vecHpiFreqs = frequencies.vecHpiFreqs;
        selectModelAndCompute(m_bBasicModel);
    }
}

//=============================================================================================================

bool SignalModel::checkFrequencies(const Frequencies frequencies)
{
    bool bHasChanged = false;
    if((m_frequencies.iSampleFreq != frequencies.iSampleFreq) ||
       (m_frequencies.iLineFreq != frequencies.iLineFreq) ||
       (m_frequencies.vecHpiFreqs != frequencies.vecHpiFreqs)) {
        bHasChanged = true;
    }
    return bHasChanged;
}
