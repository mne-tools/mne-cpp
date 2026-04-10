//=============================================================================================================
/**
 * @file     extended_infomax.h
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
 * @brief    ExtendedInfomax class declaration.
 *
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
