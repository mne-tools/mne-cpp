//=============================================================================================================
/**
 * @file     ml_pipeline.h
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
 * @brief    MlPipeline class declaration.
 *
 */

#ifndef ML_PIPELINE_H
#define ML_PIPELINE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_model.h"
#include "ml_scaler.h"

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB{

//=============================================================================================================
/**
 * @brief Simple scaler → model pipeline.
 */
class MLSHARED_EXPORT MlPipeline
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    MlPipeline();

    //=========================================================================================================
    /**
     * Set the feature scaler.
     *
     * @param[in] scaler   The scaler to use.
     */
    void setScaler(const MlScaler& scaler);

    //=========================================================================================================
    /**
     * Set the model.
     *
     * @param[in] model   Shared pointer to the model.
     */
    void setModel(MlModel::SPtr model);

    //=========================================================================================================
    /**
     * Fit the scaler on the given features (does not train the model).
     *
     * @param[in] X   Feature matrix.
     */
    void fitScaler(const MlTensor& X);

    //=========================================================================================================
    /**
     * Scale (if scaler set) and predict.
     *
     * @param[in] X   Feature matrix.
     * @return Prediction result.
     */
    MlTensor predict(const MlTensor& X) const;

private:
    MlScaler        m_scaler;           /**< Feature scaler.           */
    MlModel::SPtr   m_model;            /**< ML model.                 */
    bool            m_hasScaler = false; /**< Whether a scaler is set.  */
};

} // namespace MLLIB

#endif // ML_PIPELINE_H
