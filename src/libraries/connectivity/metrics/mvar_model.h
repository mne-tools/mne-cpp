//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mvar_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Multivariate autoregressive (MVAR) model fit and its frequency-domain decomposition; backbone of the directed connectivity metrics.
 *
 * The MVAR(p) model expresses each channel as a linear combination of the
 * last @c p samples of all channels plus white innovation noise,
 *
 *   X[t] = sum_{k=1}^{p} A_k * X[t - k] + E[t],   E ~ N(0, Sigma)
 *
 * Taking the @c z-transform with @c z = exp(-2*pi*i*f) gives the spectral
 * representation @c X(f) = H(f) * E(f) with transfer matrix
 *
 *   H(f) = ( I - sum_{k=1}^{p} A_k * exp(-2*pi*i*f*k) )^{-1}
 *
 * and spectral matrix @c S(f) = H(f) * Sigma * H(f)^H. @ref H and @ref S
 * are the only two quantities the directed-connectivity metrics in this
 * library actually need: spectral Granger Causality (@ref GrangerCausality)
 * is a ratio of diagonal entries of @c S before and after conditioning,
 * the Directed Transfer Function (@ref DirectedTransferFunction) is a row-
 * normalised @c |H_{ij}(f)|^2, and Partial Directed Coherence
 * (@ref PartialDirectedCoherence) is a column-normalised @c |A_{ij}(f)|.
 *
 * The coefficient matrices @c A_1..A_p and the innovation covariance
 * @c Sigma are estimated from the Yule-Walker equations via Levinson-
 * Durbin recursion (numerically stable, O(p^2 * n^2)); the model order
 * defaults to a Bayesian Information Criterion search over @c [1, 20]
 * when the caller passes @c p = 0.
 */

#ifndef MVARMODEL_H
#define MVARMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

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
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Multivariate autoregressive (MVAR) model fit and its frequency-domain
 * decomposition into transfer function @c H(f) and spectral matrix @c S(f).
 *
 * The model order @c p is either supplied by the caller or selected
 * automatically by Bayesian Information Criterion over @c [1, 20]; the
 * coefficient matrices are estimated by Levinson-Durbin recursion on the
 * Yule-Walker equations. The resulting @c H and @c S are consumed by the
 * three directed-connectivity metrics in this library (Granger Causality,
 * DTF, PDC) and are exposed via @ref transferFunction and
 * @ref spectralMatrix at arbitrary normalised frequencies.
 *
 * @brief MVAR model fit; provides H(f) and S(f) for Granger Causality, DTF and PDC.
 * @since 2.2.0
 */
class CONNECTIVITYSHARED_EXPORT MvarModel
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
} // namespace CONNECTIVITYLIB

#endif // MVARMODEL_H
