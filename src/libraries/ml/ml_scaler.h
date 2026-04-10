//=============================================================================================================
/**
 * @file     ml_scaler.h
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
 * @brief    MlScaler class declaration.
 *
 */

#ifndef ML_SCALER_H
#define ML_SCALER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_tensor.h"

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
 * @brief Feature scaler (StandardScaler or MinMaxScaler).
 */
class MLSHARED_EXPORT MlScaler
{
public:
    enum ScalerType { StandardScaler, MinMaxScaler };

    //=========================================================================================================
    /**
     * Construct a scaler.
     *
     * @param[in] type   Scaler strategy.
     */
    MlScaler(ScalerType type = StandardScaler);

    //=========================================================================================================
    /**
     * Compute statistics from the data.
     *
     * @param[in] data   Training data (samples x features).
     */
    void fit(const MlTensor& data);

    //=========================================================================================================
    /**
     * Apply the learned transform.
     *
     * @param[in] data   Data to transform.
     * @return The scaled tensor.
     */
    MlTensor transform(const MlTensor& data) const;

    //=========================================================================================================
    /**
     * Convenience: fit then transform.
     *
     * @param[in] data   Training data.
     * @return The scaled tensor.
     */
    MlTensor fitTransform(const MlTensor& data);

    //=========================================================================================================
    /**
     * Undo the scaling.
     *
     * @param[in] data   Scaled data.
     * @return The original-scale tensor.
     */
    MlTensor inverseTransform(const MlTensor& data) const;

private:
    ScalerType      m_type;     /**< Scaler strategy.            */
    Eigen::VectorXf m_mean;    /**< Per-feature mean  (Std).     */
    Eigen::VectorXf m_std;     /**< Per-feature stdev (Std).     */
    Eigen::VectorXf m_min;     /**< Per-feature min   (MinMax).  */
    Eigen::VectorXf m_range;   /**< Per-feature range (MinMax).  */
    bool            m_fitted = false;  /**< Whether fit() was called. */
};

} // namespace MLLIB

#endif // ML_SCALER_H
