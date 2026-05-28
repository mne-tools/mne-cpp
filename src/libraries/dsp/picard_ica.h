//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     picard_ica.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Declaration of the PicardIca class — Preconditioned ICA for Real Data.
 *
 * Algorithm: P. Ablin, J.-F. Cardoso, A. Gramfort (2018). "Faster Independent Component
 *            Analysis by Preconditioning with Hessian Approximations." IEEE TSP 66(15):4040–4049.
 *            Uses an approximate Hessian (L-BFGS-like) preconditioner for faster convergence.
 */

#ifndef PICARD_ICA_DSP_H
#define PICARD_ICA_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
#include "ica.h"  // Re-use IcaResult

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Independent Component Analysis using the Picard algorithm.
 *
 * Picard uses a preconditioned L-BFGS strategy to minimise the mutual information
 * of the components.  Compared to FastICA it typically converges in fewer iterations
 * and handles badly conditioned data more robustly.
 *
 * @code
 *   IcaResult res = PicardIca::run(matRawData, 20);
 *   QVector<int> exclude = {0, 3};
 *   MatrixXd clean = ICA::excludeComponents(matRawData, res, exclude);
 * @endcode
 */
class DSPSHARED_EXPORT PicardIca
{
public:
    //=========================================================================================================
    /**
     * Run the Picard ICA algorithm.
     *
     * @param[in] matData       Input data (n_channels x n_samples).
     * @param[in] nComponents   Number of components to extract (-1 = all channels).
     * @param[in] maxIter       Maximum number of iterations (default 200).
     * @param[in] tol           Convergence tolerance (default 1e-7).
     * @param[in] lbfgsMemory   L-BFGS memory length (default 7).
     * @param[in] randomSeed    Seed for initial weight randomisation (default 42).
     *
     * @return IcaResult with mixing/unmixing matrices and source time series.
     */
    static IcaResult run(const Eigen::MatrixXd& matData,
                         int    nComponents  = -1,
                         int    maxIter      = 200,
                         double tol          = 1e-7,
                         int    lbfgsMemory  = 7,
                         int    randomSeed   = 42);

private:
    PicardIca() = delete;
};

} // namespace UTILSLIB

#endif // PICARD_ICA_DSP_H
