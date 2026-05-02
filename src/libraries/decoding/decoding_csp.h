//=============================================================================================================
/**
 * @file     decoding_csp.h
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
 * @brief    DecodingCsp class declaration.
 *
 */

#ifndef DECODING_CSP_H
#define DECODING_CSP_H

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
 * @brief Common Spatial Patterns (CSP) for M/EEG signal decomposition.
 *
 * Mirrors the public API of mne.decoding.CSP from MNE-Python. The core
 * The core GED computation is implemented inline. This class adds
 * MNE-specific features:
 *
 * - transform_into modes: AveragePower (default) or CspSpace
 * - Log or z-score standardisation of band-power features
 * - Inverse transform from feature space back to sensor space
 * - Mean and standard deviation for z-score normalisation
 *
 * Input data convention: each epoch is (n_channels × n_times).
 *
 * @see mne.decoding.CSP in MNE-Python
 */
class DECODINGSHARED_EXPORT DecodingCsp
{
public:
    //=========================================================================================================
    /**
     * Transform mode for the CSP output.
     */
    enum class TransformMode {
        AveragePower,   /**< Return average band power per component (n_epochs × n_components). */
        CspSpace        /**< Return data projected into CSP space (n_epochs × n_components × n_times). */
    };

    //=========================================================================================================
    /**
     * Constructs a CSP decoder.
     *
     * @param[in] nComponents     Number of CSP components (split between classes). Default: 4.
     * @param[in] transformInto   Feature extraction mode. Default: AveragePower.
     * @param[in] useLog          If true (default) and transformInto == AveragePower, apply log transform;
     *                            otherwise z-score features using mean_ and std_.
     */
    explicit DecodingCsp(int nComponents = 4,
                         TransformMode transformInto = TransformMode::AveragePower,
                         bool useLog = true);

    //=========================================================================================================
    /**
     * Fit CSP from labelled epoch data (binary classification).
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @param[in] y       Class label for each epoch (must contain exactly 2 unique values).
     */
    void fit(const std::vector<Eigen::MatrixXd>& epochs,
             const Eigen::VectorXi& y);

    //=========================================================================================================
    /**
     * Transform epoch data using the fitted CSP filters.
     *
     * When transformInto == AveragePower, returns (n_epochs × n_components)
     * with log-transformed or z-scored mean band power.
     *
     * When transformInto == CspSpace, returns a matrix where each
     * n_components rows correspond to one epoch's CSP-space projection.
     * The shape is (n_epochs * n_components, n_times).
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @return Feature matrix.
     */
    Eigen::MatrixXd transform(const std::vector<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * Fit and transform in one step.
     *
     * @param[in] epochs  Epoch data.
     * @param[in] y       Class labels.
     * @return Feature matrix (same as transform output).
     */
    Eigen::MatrixXd fitTransform(const std::vector<Eigen::MatrixXd>& epochs,
                                 const Eigen::VectorXi& y);

    //=========================================================================================================
    /**
     * Project CSP power features back to sensor space.
     *
     * Only valid when transformInto == AveragePower.
     *
     * @param[in] X  Feature matrix (n_epochs × n_components).
     * @return Sensor-space projection (n_epochs × n_channels × n_components).
     *         Stored as (n_epochs, n_channels * n_components) flattened.
     */
    Eigen::MatrixXd inverseTransform(const Eigen::MatrixXd& X) const;

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
     * @return Standard deviation of band power per component (computed during fit).
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

    Eigen::MatrixXd m_filters;      /**< Spatial filters (n_components × n_channels). */
    Eigen::MatrixXd m_patterns;     /**< Spatial patterns (n_channels × n_components). */
    Eigen::VectorXd m_mean;         /**< Mean band power per component. */
    Eigen::VectorXd m_std;          /**< Std dev band power per component. */
    bool m_fitted = false;

    /**
     * Compute average band power features from filtered epochs.
     */
    Eigen::MatrixXd computePowerFeatures(
        const std::vector<Eigen::MatrixXd>& epochs) const;
};

} // namespace DECODINGLIB

#endif // DECODING_CSP_H
