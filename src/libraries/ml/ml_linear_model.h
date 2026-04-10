//=============================================================================================================
/**
 * @file     ml_linear_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MlLinearModel class declaration.
 *
 */

#ifndef ML_LINEAR_MODEL_H
#define ML_LINEAR_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_model.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB{

//=============================================================================================================
/**
 * @brief Ridge regression / logistic regression built-in model.
 */
class MLSHARED_EXPORT MlLinearModel : public MlModel
{
public:
    //=========================================================================================================
    /**
     * Construct a linear model.
     *
     * @param[in] type              Task type (Regression or Classification).
     * @param[in] regularization    Ridge / L2 regularization strength (lambda).
     */
    MlLinearModel(MlTaskType type = MlTaskType::Regression, double regularization = 1.0);

    //=========================================================================================================
    MlTensor predict(const MlTensor& input) const override;
    bool save(const QString& path) const override;
    bool load(const QString& path) override;
    QString modelType() const override;
    MlTaskType taskType() const override;

    //=========================================================================================================
    /**
     * @return Const reference to the weight matrix (n_features x n_outputs).
     */
    const Eigen::MatrixXf& weights() const;

    //=========================================================================================================
    /**
     * @return Const reference to the bias vector (n_outputs).
     */
    const Eigen::VectorXf& bias() const;

private:
    MlTaskType      m_taskType;             /**< Task type.                       */
    double          m_regularization;       /**< L2 regularization strength.      */
    Eigen::MatrixXf m_weights;              /**< Weight matrix (n_features x n_outputs). */
    Eigen::VectorXf m_bias;                 /**< Bias vector   (n_outputs).       */
    bool            m_trained = false;      /**< Whether the model has been trained. */
};

} // namespace MLLIB

#endif // ML_LINEAR_MODEL_H
