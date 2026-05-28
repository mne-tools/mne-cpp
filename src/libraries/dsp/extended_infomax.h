//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     extended_infomax.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Extended Infomax independent component analysis (super- and sub-Gaussian sources).
 *
 * Extended Infomax (Lee, Girolami & Sejnowski, 1999) is a maximum-likelihood
 * ICA algorithm that maximises the joint entropy of a non-linearly
 * transformed mixture and automatically switches the score (non-linearity)
 * between a logistic form, suitable for super-Gaussian (sparse, peaky)
 * sources, and a cubic form, suitable for sub-Gaussian sources such as
 * line noise. The switch is driven by the sign of the per-component
 * stability test @c k_i = sign(⟨sech²(Wᵢx)⟩ − ⟨(Wᵢx)²⟩), recomputed every
 * iteration, which gives Extended Infomax a substantial robustness
 * advantage over the standard logistic-only Infomax on MEG / EEG data
 * where both source classes coexist.
 *
 * The implementation here uses the natural-gradient update with adaptive
 * step-size and optional block-wise random shuffling of samples, matching
 * the MNE-Python defaults so artifact decompositions are reproducible
 * across the two toolchains.
 */

#ifndef EXTENDED_INFOMAX_H
#define EXTENDED_INFOMAX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * Result structure for Extended Infomax ICA.
 */
struct DSPSHARED_EXPORT InfomaxResult {
    Eigen::MatrixXd matUnmixing;    /**< Unmixing matrix (n_components x n_channels). */
    Eigen::MatrixXd matMixing;      /**< Mixing matrix (n_channels x n_components). */
    Eigen::MatrixXd matSources;     /**< Source activations (n_components x n_times). */
    int nIterations;                /**< Number of iterations performed. */
    bool converged;                 /**< Whether the algorithm converged. */
};

//=============================================================================================================
/**
 * Extended Infomax ICA (Lee et al., 1999).
 *
 * Performs Independent Component Analysis using the extended infomax algorithm,
 * which can separate both super-Gaussian and sub-Gaussian sources.
 */
class DSPSHARED_EXPORT ExtendedInfomax {
public:
    /**
     * Compute ICA decomposition using the extended infomax algorithm.
     *
     * @param[in] matData        Input data matrix (n_channels x n_times), should be mean-removed.
     * @param[in] nComponents    Number of components to extract (-1 for n_channels).
     * @param[in] maxIterations  Maximum number of iterations.
     * @param[in] learningRate   Learning rate for weight updates.
     * @param[in] tolerance      Convergence tolerance.
     * @param[in] extendedMode   If true, use extended mode (sub- and super-Gaussian).
     * @param[in] seed           Random seed (0 for no seeding).
     *
     * @return InfomaxResult containing unmixing/mixing matrices and sources.
     */
    static InfomaxResult compute(
        const Eigen::MatrixXd& matData,
        int nComponents = -1,
        int maxIterations = 200,
        double learningRate = 0.001,
        double tolerance = 1e-7,
        bool extendedMode = true,
        unsigned int seed = 0);

private:
    /**
     * Estimate the sign vector based on excess kurtosis of each component.
     *
     * @param[in] matSources  Source matrix (n_components x n_times).
     *
     * @return Vector of +1 (super-Gaussian) or -1 (sub-Gaussian) per component.
     */
    static Eigen::VectorXd estimateSignVector(const Eigen::MatrixXd& matSources);
};

} // namespace UTILSLIB

#endif // EXTENDED_INFOMAX_H
