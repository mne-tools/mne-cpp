//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_mxne.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Mixed-Norm Estimate (MxNE) sparse inverse solver — block-sparse L21 minimisation for focal source recovery.
 *
 * @ref INVLIB::InvMxne implements the MxNE solver of Gramfort et al.,
 * @em Mixed-norm estimates for the M/EEG inverse problem using
 * accelerated gradient methods, Phys. Med. Biol. 57(7), 1937-1961
 * (2012). It minimises
 * @f$ \tfrac{1}{2}\,\|M - G X\|_F^{2} + \alpha\,\|X\|_{2,1} @f$
 * using iteratively-reweighted least squares; the @c L21 group penalty
 * zeros out entire source rows that do not contribute to the residual,
 * producing a focal solution with a small list of active vertices.
 * Outputs the sparse @ref InvSourceEstimate, the active-vertex list,
 * iteration count and final residual norm in an @ref InvMxneResult.
 */

#ifndef INV_MXNE_H
#define INV_MXNE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Result structure for the MxNE solver.
 */
struct INVSHARED_EXPORT InvMxneResult {
    InvSourceEstimate stc;
    QVector<int> activeVertices;
    int nIterations;
    double residualNorm;
};

//=============================================================================================================
/**
 * Mixed-Norm Estimate (MxNE) sparse inverse solver.
 *
 * Minimizes: ||M - G*X||^2_F + alpha * sum_i ||X_i||_2
 * using an Iteratively Reweighted Least Squares (IRLS) approach for the L21-norm (group lasso).
 *
 * @brief MxNE sparse inverse solver.
 */
class INVSHARED_EXPORT InvMxne
{
public:
    //=========================================================================================================
    /**
     * Compute the MxNE inverse solution.
     *
     * @param[in] matGain        Forward gain matrix (n_channels x n_sources).
     * @param[in] matData        Measurement data (n_channels x n_times).
     * @param[in] alpha          Regularization parameter.
     * @param[in] nIterations    Maximum number of IRLS iterations.
     * @param[in] tolerance      Convergence tolerance on weight change.
     *
     * @return The MxNE result containing the sparse source estimate.
     */
    static InvMxneResult compute(
        const Eigen::MatrixXd& matGain,
        const Eigen::MatrixXd& matData,
        double alpha,
        int nIterations = 50,
        double tolerance = 1e-6);
};

} // namespace INVLIB

#endif // INV_MXNE_H
