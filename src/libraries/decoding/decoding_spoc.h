//=============================================================================================================
/**
 * @file     decoding_spoc.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    DecodingSpoc class declaration.
 *
 */

#ifndef DECODING_SPOC_H
#define DECODING_SPOC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE DECODINGLIB
//=============================================================================================================

namespace DECODINGLIB{

//=============================================================================================================
/**
 * @brief Source Power Comodulation (SPoC) for M/EEG signal decomposition.
 *
 * Mirrors mne.decoding.SPoC from MNE-Python. SPoC finds spatial filters
 * whose band-power envelope maximally covaries with a continuous target
 * variable. Delegates core GED to Skigen::SPoC.
 *
 * MNE-specific additions:
 * - transform_into modes: AveragePower (default) or CspSpace
 * - Log or z-score standardisation of band-power features
 * - Mean and standard deviation for z-score normalisation
 *
 * Input data: each epoch is (n_channels × n_times).
 *
 * @see mne.decoding.SPoC in MNE-Python
 */
class DECODINGSHARED_EXPORT DecodingSpoc
{
public:
    //=========================================================================================================
    /**
     * Transform mode for the SPoC output.
     */
    enum class TransformMode {
        AveragePower,   /**< Return average band power per component. */
        CspSpace        /**< Return data projected into SPoC space. */
    };

    //=========================================================================================================
    /**
     * Constructs a SPoC decoder.
     *
     * @param[in] nComponents     Number of components. Default: 4.
     * @param[in] transformInto   Feature extraction mode. Default: AveragePower.
     * @param[in] useLog          If true and transformInto == AveragePower, apply log transform.
     */
    explicit DecodingSpoc(int nComponents = 4,
                          TransformMode transformInto = TransformMode::AveragePower,
                          bool useLog = true);

    //=========================================================================================================
    /**
     * Fit SPoC from epoch data and a continuous target variable.
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @param[in] y       Continuous target variable (one value per epoch).
     */
    void fit(const std::vector<Eigen::MatrixXd>& epochs,
             const Eigen::VectorXd& y);

    //=========================================================================================================
    /**
     * Transform epoch data using the fitted SPoC filters.
     *
     * @param[in] epochs  Vector of epoch matrices.
     * @return Feature matrix.
     */
    Eigen::MatrixXd transform(const std::vector<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * Fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const std::vector<Eigen::MatrixXd>& epochs,
                                 const Eigen::VectorXd& y);

    //=========================================================================================================
    /**
     * @return Spatial filters (n_components × n_channels).
     */
    const Eigen::MatrixXd& filters() const;

    //=========================================================================================================
    /**
     * @return Spatial patterns (n_channels × n_components).
     */
    const Eigen::MatrixXd& patterns() const;

    //=========================================================================================================
    /**
     * @return Mean band power per component (computed during fit).
     */
    const Eigen::VectorXd& mean() const;

    //=========================================================================================================
    /**
     * @return Standard deviation of band power per component.
     */
    const Eigen::VectorXd& stddev() const;

    //=========================================================================================================
    /**
     * @return True if the model has been fitted.
     */
    bool isFitted() const;

private:
    int m_nComponents;
    TransformMode m_transformInto;
    bool m_useLog;

    Eigen::MatrixXd m_filters;
    Eigen::MatrixXd m_patterns;
    Eigen::VectorXd m_mean;
    Eigen::VectorXd m_std;
    bool m_fitted = false;

    Eigen::MatrixXd computePowerFeatures(
        const std::vector<Eigen::MatrixXd>& epochs) const;
};

} // namespace DECODINGLIB

#endif // DECODING_SPOC_H
