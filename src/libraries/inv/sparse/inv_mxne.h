//=============================================================================================================
/**
 * @file     inv_mxne.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    InvMxne class declaration.
 *
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
