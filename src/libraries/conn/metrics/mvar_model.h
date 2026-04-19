//=============================================================================================================
/**
 * @file     mvar_model.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief     MvarModel class declaration.
 *
 */

#ifndef MVARMODEL_H
#define MVARMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Multivariate autoregressive (MVAR) model fitting and spectral decomposition.
 *
 * Fits an MVAR model of order p to multi-channel time-series data and provides
 * the transfer function H(f) and spectral matrix S(f) used by directed
 * connectivity metrics (Granger Causality, DTF, PDC).
 *
 * @brief MVAR model fitting for directed connectivity metrics.
 * @since 2.2.0
 */
class CONNSHARED_EXPORT MvarModel
{

public:
    //=========================================================================================================
    /**
     * Constructs a default MvarModel object.
     */
    MvarModel() = default;

    //=========================================================================================================
    /**
     * Fits an MVAR model of order p to multi-channel data.
     *
     * @param[in] data   The input data matrix (nChannels x nSamples).
     * @param[in] p      The model order. If 0, the order is auto-selected via BIC.
     *
     * @since 2.2.0
     */
    void fit(const Eigen::MatrixXd& data, int p = 0);

    //=========================================================================================================
    /**
     * Returns the fitted coefficient matrices A_1 .. A_p (each nChannels x nChannels).
     *
     * @return The MVAR coefficient matrices.
     *
     * @since 2.2.0
     */
    QVector<Eigen::MatrixXd> coefficients() const;

    //=========================================================================================================
    /**
     * Returns the noise covariance matrix (nChannels x nChannels).
     *
     * @return The noise covariance matrix.
     *
     * @since 2.2.0
     */
    Eigen::MatrixXd noiseCov() const;

    //=========================================================================================================
    /**
     * Returns the fitted model order.
     *
     * @return The model order p.
     *
     * @since 2.2.0
     */
    int order() const;

    //=========================================================================================================
    /**
     * Computes the transfer function H(f) at the given normalized frequencies.
     *
     * H(f) = (I - sum_{k=1}^{p} A_k * exp(-2*pi*i*f*k))^{-1}
     *
     * @param[in] freqs   Vector of normalized frequencies in [0, 0.5].
     *
     * @return Vector of nChannels x nChannels complex matrices, one per frequency.
     *
     * @since 2.2.0
     */
    QVector<Eigen::MatrixXcd> transferFunction(const Eigen::VectorXd& freqs) const;

    //=========================================================================================================
    /**
     * Computes the spectral matrix S(f) = H(f) * Sigma * H(f)^H.
     *
     * @param[in] freqs   Vector of normalized frequencies in [0, 0.5].
     *
     * @return Vector of nChannels x nChannels complex matrices, one per frequency.
     *
     * @since 2.2.0
     */
    QVector<Eigen::MatrixXcd> spectralMatrix(const Eigen::VectorXd& freqs) const;

private:
    //=========================================================================================================
    /**
     * Fits the MVAR model using Levinson-Durbin recursion (Yule-Walker equations).
     *
     * @param[in] data   The input data matrix (nChannels x nSamples).
     * @param[in] p      The model order.
     */
    void fitLevinsonDurbin(const Eigen::MatrixXd& data, int p);

    //=========================================================================================================
    /**
     * Selects the optimal model order via Bayesian Information Criterion.
     *
     * @param[in] data       The input data matrix.
     * @param[in] maxOrder   The maximum order to test.
     *
     * @return The order that minimizes BIC.
     */
    int selectOrderBIC(const Eigen::MatrixXd& data, int maxOrder = 20) const;

    QVector<Eigen::MatrixXd>    m_coeffs;       /**< Coefficient matrices A_1..A_p. */
    Eigen::MatrixXd             m_noiseCov;     /**< Noise covariance matrix. */
    int                         m_order = 0;    /**< Model order. */
    int                         m_nChannels = 0;/**< Number of channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // MVARMODEL_H
