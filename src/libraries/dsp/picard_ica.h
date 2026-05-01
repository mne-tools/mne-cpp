//=============================================================================================================
/**
 * @file     picard_ica.h
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
