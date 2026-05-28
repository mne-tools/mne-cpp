//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_signal_model.cpp
 * @since March 2026
 * @brief Implementation of the basic / advanced HPI signal-model builders and the cached pseudo-inverse.
 *
 * Implements the basic model (sin/cos pairs for the HPI drive
 * frequencies only), the advanced model (extra sin/cos pairs for the
 * line frequency and harmonics), the cache invalidator that detects
 * when the model has to be rebuilt, and the SVD-based pseudo-inverse
 * that projects raw MEG samples onto per-coil drive amplitudes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_hpi_model_parameters.h"
#include "inv_signal_model.h"
#include <math/linalg.h>
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

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd InvSignalModel::fitData(const InvHpiModelParameters& hpiModelParameters, const MatrixXd& matData)
{

    if(checkEmpty(hpiModelParameters)) {
        return MatrixXd();
    }

    const bool bParametersChanged = m_modelParameters != hpiModelParameters;
    const bool bDimensionsChanged = m_iCurrentModelCols != matData.cols();

    if(bDimensionsChanged || bParametersChanged) {
        m_iCurrentModelCols = matData.cols();
        m_modelParameters = hpiModelParameters;
        selectModelAndCompute();
    }

    return m_matInverseSignalModel * matData.transpose();
}

//=============================================================================================================

bool InvSignalModel::checkDataDimensions(const int iCols)
{
    bool bHasChanged = false;
    if(iCols != m_iCurrentModelCols) {
        m_iCurrentModelCols = iCols;
        bHasChanged = true;
    }
    return bHasChanged;
}

//=============================================================================================================

bool InvSignalModel::checkModelParameters(const InvHpiModelParameters& hpiModelParameters)
{
    bool bHasChanged = false;
    if((m_modelParameters.iSampleFreq() != hpiModelParameters.iSampleFreq()) ||
        (m_modelParameters.iLineFreq() != hpiModelParameters.iLineFreq()) ||
        (m_modelParameters.iNHpiCoils() != hpiModelParameters.iNHpiCoils()) ||
        (m_modelParameters.vecHpiFreqs() != hpiModelParameters.vecHpiFreqs()) ||
        (m_modelParameters.bBasic() != hpiModelParameters.bBasic())) {
        bHasChanged = true;
        m_modelParameters = hpiModelParameters;
    }
    return bHasChanged;
}

//=============================================================================================================

bool InvSignalModel::checkEmpty(const InvHpiModelParameters& hpiModelParameters)
{
    if(hpiModelParameters.vecHpiFreqs().empty()) {
        std::cout << "InvSignalModel::checkEmpty - no Hpi frequencies set" << std::endl;
        return true;
    } else if(hpiModelParameters.iSampleFreq() == 0) {
        std::cout << "InvSignalModel::checkEmpty - no sampling frequencies set" << std::endl;
        return true;
    }
    return false;
}

//=============================================================================================================

void InvSignalModel::selectModelAndCompute()
{
    if(m_modelParameters.bBasic()) {
        computeInverseBasicModel();
    } else {
        computeInverseAdvancedModel();
    }
}

//=============================================================================================================

void InvSignalModel::computeInverseBasicModel()
{
    const int iNumCoils = m_modelParameters.iNHpiCoils();
    MatrixXd matSimsig;
    const VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/m_modelParameters.iSampleFreq();

    // Generate simulated data Matrix
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*2);

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_modelParameters.vecHpiFreqs()[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_modelParameters.vecHpiFreqs()[i]*vecTime.array());
    }
    m_matInverseSignalModel = UTILSLIB::Linalg::pinv(matSimsig);
}

//=============================================================================================================

void InvSignalModel::computeInverseAdvancedModel()
{
    const int iNumCoils = m_modelParameters.iNHpiCoils();
    const int iSampleFreq = m_modelParameters.iSampleFreq();
    MatrixXd matSimsig;
    MatrixXd matSimsigInvTemp;

    const VectorXd vecTime = VectorXd::LinSpaced(m_iCurrentModelCols, 0, m_iCurrentModelCols-1) *1.0/iSampleFreq;

    // add linefreq + harmonics + DC part to model
    matSimsig.conservativeResize(m_iCurrentModelCols,iNumCoils*4+2);
    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*m_modelParameters.vecHpiFreqs()[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*m_modelParameters.vecHpiFreqs()[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*m_modelParameters.iLineFreq()*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*m_modelParameters.iLineFreq()*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(m_iCurrentModelCols, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);
    matSimsigInvTemp = UTILSLIB::Linalg::pinv(matSimsig);
    m_matInverseSignalModel = matSimsigInvTemp.block(0,0,iNumCoils*2,m_iCurrentModelCols);
}
