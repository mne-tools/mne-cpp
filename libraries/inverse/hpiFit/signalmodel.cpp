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
#include <iostream>

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

SignalModel::SignalModel()
{
    m_iCurrentModelCols = 0;
}

//=============================================================================================================

MatrixXd SignalModel::fitData(const ModelParameters& modelParameters,const MatrixXd& matData)
{

    if(checkEmpty(modelParameters)) {
        MatrixXd matTopo;
        return matTopo;
    }

    bool bParametersChanged = checkModelParameters(modelParameters);
    bool bDimensionsChanged = checkDataDimensions(matData.cols());

    if(bDimensionsChanged || bParametersChanged) {
        selectModelAndCompute();
    }

    MatrixXd matTopo = m_matInverseSignalModel * matData.transpose();
    return matTopo;
}

//=============================================================================================================

bool SignalModel::checkDataDimensions(const int iCols)
{
    bool bHasChanged = false;
    if(iCols != m_iCurrentModelCols) {
        m_iCurrentModelCols = iCols;
        bHasChanged = true;
    }
    return bHasChanged;
}

//=============================================================================================================

bool SignalModel::checkModelParameters(const ModelParameters& modelParameters)
{
    bool bHasChanged = false;
    if((m_modelParameters.iSampleFreq != modelParameters.iSampleFreq) ||
        (m_modelParameters.iLineFreq != modelParameters.iLineFreq) ||
        (m_modelParameters.vecHpiFreqs != modelParameters.vecHpiFreqs) ||
        (m_modelParameters.bBasic != modelParameters.bBasic)) {
        bHasChanged = true;
        m_modelParameters = modelParameters;
    }
    return bHasChanged;
}

//=============================================================================================================

bool SignalModel::checkEmpty(const ModelParameters& modelParameters)
{
    if(modelParameters.vecHpiFreqs.empty()) {
        std::cout << "SignalModel::checkEmpty - no Hpi frequencies set" << std::endl;
        return true;
    } else if(modelParameters.iSampleFreq == 0) {
        std::cout << "SignalModel::checkEmpty - no sampling frequencies set" << std::endl;
        return true;
    }
    return false;
}

//=============================================================================================================

void SignalModel::selectModelAndCompute()
{
    if(m_modelParameters.bBasic) {
        computeInverseBasicModel();
    } else {
        computeInverseAdvancedModel();
    }
}

//=============================================================================================================

void SignalModel::computeInverseBasicModel()
{
    int iNumCoils = m_modelParameters.vecHpiFreqs.size();
    MatrixXd matSimsig;
    VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/m_modelParameters.iSampleFreq;

    // Generate simulated data Matrix
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*2);

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_modelParameters.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_modelParameters.vecHpiFreqs[i]*vecTime.array());
    }
    m_matInverseSignalModel = UTILSLIB::MNEMath::pinv(matSimsig);
}

//=============================================================================================================

void SignalModel::computeInverseAdvancedModel()
{
    int iNumCoils = m_modelParameters.vecHpiFreqs.size();
    MatrixXd matSimsig;
    MatrixXd matSimsigInvTemp;

    VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/m_modelParameters.iSampleFreq;

    // add linefreq + harmonics + DC part to model
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*4+2);
    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_modelParameters.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_modelParameters.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*m_modelParameters.iLineFreq*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*m_modelParameters.iLineFreq*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(m_iCurrentModelCols, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);
    matSimsigInvTemp = UTILSLIB::MNEMath::pinv(matSimsig);
    m_matInverseSignalModel = matSimsigInvTemp.block(0,0,iNumCoils*2,m_iCurrentModelCols);
}
