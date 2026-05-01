//=============================================================================================================
/**
 * @file     ml_spoc.h
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
 * @brief    MlSpoc class — Source Power Comodulation (SPoC) spatial filter.
 *
 */

#ifndef ML_SPOC_H
#define ML_SPOC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB
{

//=============================================================================================================
/**
 * @brief Source Power Comodulation (SPoC) spatial filter.
 *
 * SPoC finds spatial filters whose power envelope covaries maximally
 * with a continuous target variable (e.g. reaction time, pain rating).
 * It solves the generalised eigenvalue problem:
 *
 *   C_z · w = λ · C · w
 *
 * where C is the average data covariance and C_z is the covariance
 * weighted by the (zero-mean, unit-variance) target variable z.
 *
 * @code
 *   MlSpoc spoc(3);  // 3 components
 *   QList<Eigen::MatrixXd> epochs = ...;
 *   Eigen::VectorXd target = ...;  // one value per epoch
 *   spoc.fit(epochs, target);
 *   Eigen::MatrixXd features = spoc.transform(epochs);
 * @endcode
 */
class MLSHARED_EXPORT MlSpoc
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] nComponents   Number of SPoC components to extract (default 4).
     */
    explicit MlSpoc(int nComponents = 4);

    //=========================================================================================================
    /**
     * Fit SPoC from epoched data and a continuous target variable.
     *
     * @param[in] epochs   List of epoch matrices, each (n_channels × n_times).
     * @param[in] target   Target variable, one value per epoch (length = epochs.size()).
     */
    void fit(const QList<Eigen::MatrixXd>& epochs,
             const Eigen::VectorXd& target);

    //=========================================================================================================
    /**
     * Transform epoched data into SPoC feature space (log-variance of
     * filtered signals).
     *
     * @param[in] epochs   List of epoch matrices (n_channels × n_times).
     * @return             Feature matrix (n_epochs × n_components).
     */
    Eigen::MatrixXd transform(const QList<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * Convenience: fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const QList<Eigen::MatrixXd>& epochs,
                                 const Eigen::VectorXd& target);

    //=========================================================================================================
    /**
     * Get the spatial filters (n_components × n_channels).
     */
    Eigen::MatrixXd filters() const { return m_matFilters; }

    //=========================================================================================================
    /**
     * Get the spatial patterns (n_components × n_channels).
     */
    Eigen::MatrixXd patterns() const { return m_matPatterns; }

    //=========================================================================================================
    /**
     * Get the eigenvalues (n_components).
     */
    Eigen::VectorXd eigenvalues() const { return m_vecEigenvalues; }

    //=========================================================================================================
    /**
     * Whether the model has been fitted.
     */
    bool isFitted() const { return m_bFitted; }

private:
    int             m_nComponents;
    bool            m_bFitted;
    Eigen::MatrixXd m_matFilters;       ///< n_components × n_channels
    Eigen::MatrixXd m_matPatterns;      ///< n_components × n_channels
    Eigen::VectorXd m_vecEigenvalues;   ///< n_components
};

} // namespace MLLIB

#endif // ML_SPOC_H
