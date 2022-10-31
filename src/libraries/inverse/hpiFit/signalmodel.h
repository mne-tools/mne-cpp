//=============================================================================================================
/**
 * @file     SignalModel.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
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
 * @brief     SignalModel class declaration.
 *
 */

#ifndef SIGNALMODEL_H
#define SIGNALMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "hpimodelparameters.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVERSELIB
{
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================


//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT SignalModel
{

public:
    typedef QSharedPointer<SignalModel> SPtr;            /**< Shared pointer type for SignalModel. */
    typedef QSharedPointer<const SignalModel> ConstSPtr; /**< Const shared pointer type for SignalModel. */

    //=========================================================================================================
    /**
     * Constructs a SignalModel object.
     */
    explicit SignalModel() = default;

    //=========================================================================================================
    /**
     * Fit the data to the model constructed from given model parameters.
     *
     * @param[in] hpiModelParameters   The HpiModelParameters.
     * @param[in] matData           The data matrix.
     *
     * @return the fitted data
     *
     */
    Eigen::MatrixXd fitData(const HpiModelParameters& hpiModelParameters,
                            const Eigen::MatrixXd& matData);

private:
    //=========================================================================================================
    /**
     * Selects the model to compute and calls coresponding compute function.
     *
     */
    void selectModelAndCompute();

    //=========================================================================================================
    /**
     * Computes the model.
     *
     */
    void computeInverseBasicModel();
    void computeInverseAdvancedModel();

    //=========================================================================================================
    /**
     * Check if dimensions of input data match the model.
     *
     * @param[in] iCols     The number of Clumns to compare.
     * @return true if changed
     *
     */
    bool checkDataDimensions(const int iCols);

    //=========================================================================================================
    /**
     * Check if the HpiModelParameters changed.
     *
     * @param[in] hpiModelParameters     The model parameters.
     * @return true if changed
     */
    bool checkModelParameters(const HpiModelParameters& hpiModelParameters);

    //=========================================================================================================
    /**
     * Check if the HpiModelParameters are empty. HPI and sampling frequencies need to be set.
     *
     * @param[in] hpiModelParameters     The model parameters.
     * @return true if empty
     */
    bool checkEmpty(const HpiModelParameters& hpiModelParameters);

    Eigen::MatrixXd m_matInverseSignalModel{Eigen::MatrixXd(0,0)};
    int m_iCurrentModelCols{0};
    HpiModelParameters m_modelParameters{HpiModelParameters()};
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace INVERSELIB

#endif // SignalModel_H

