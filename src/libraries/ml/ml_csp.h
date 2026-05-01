//=============================================================================================================
/**
 * @file     ml_csp.h
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
 * @brief    Common Spatial Patterns (CSP) for BCI decoding.
 *
 * Equivalent to MNE-Python's mne.decoding.CSP.
 */

#ifndef ML_CSP_H
#define ML_CSP_H

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
 * @brief Common Spatial Patterns (CSP) for two-class BCI decoding.
 *
 * CSP finds spatial filters that maximize the variance ratio between
 * two conditions. The resulting features (log-variance of filtered signals)
 * are suitable for classification with LDA, SVM, or logistic regression.
 *
 * Usage:
 * @code
 *   MlCsp csp(6);  // 6 components (3 per class)
 *   csp.fit(epochsClass1, epochsClass2);
 *   Eigen::MatrixXd features = csp.transform(testEpochs);
 * @endcode
 */
class MLSHARED_EXPORT MlCsp
{
public:
    //=========================================================================================================
    /**
     * @brief Construct CSP with given number of components.
     *
     * @param[in] nComponents  Number of CSP components (should be even; default 6 = 3 per class).
     */
    explicit MlCsp(int nComponents = 6);

    //=========================================================================================================
    /**
     * @brief Fit CSP filters from two-class epoch data.
     *
     * @param[in] epochsClass1  List of epoch matrices (n_channels × n_times) for class 1.
     * @param[in] epochsClass2  List of epoch matrices (n_channels × n_times) for class 2.
     */
    void fit(const QList<Eigen::MatrixXd>& epochsClass1,
             const QList<Eigen::MatrixXd>& epochsClass2);

    //=========================================================================================================
    /**
     * @brief Transform epochs into CSP feature space.
     *
     * For each epoch, applies spatial filters and returns log-variance features.
     *
     * @param[in] epochs  List of epoch matrices (n_channels × n_times).
     *
     * @return Feature matrix (n_epochs × n_components), log-variance features.
     */
    Eigen::MatrixXd transform(const QList<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * @brief Fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const QList<Eigen::MatrixXd>& epochsClass1,
                                  const QList<Eigen::MatrixXd>& epochsClass2);

    //=========================================================================================================
    /**
     * @brief Get the spatial filters matrix.
     *
     * @return CSP spatial filters (n_components × n_channels).
     */
    const Eigen::MatrixXd& filters() const { return m_filters; }

    //=========================================================================================================
    /**
     * @brief Get the spatial patterns matrix.
     *
     * @return CSP spatial patterns (n_channels × n_components).
     */
    const Eigen::MatrixXd& patterns() const { return m_patterns; }

    //=========================================================================================================
    /**
     * @brief Get the eigenvalues (variance ratios).
     */
    const Eigen::VectorXd& eigenvalues() const { return m_eigenvalues; }

    //=========================================================================================================
    /**
     * @brief Check if CSP has been fitted.
     */
    bool isFitted() const { return m_bFitted; }

    //=========================================================================================================
    /**
     * @brief Get number of components.
     */
    int nComponents() const { return m_nComponents; }

private:
    int             m_nComponents;
    bool            m_bFitted = false;
    Eigen::MatrixXd m_filters;       /**< Spatial filters (n_components × n_channels). */
    Eigen::MatrixXd m_patterns;      /**< Spatial patterns (n_channels × n_components). */
    Eigen::VectorXd m_eigenvalues;   /**< Eigenvalues / variance ratios. */
};

} // namespace MLLIB

#endif // ML_CSP_H
