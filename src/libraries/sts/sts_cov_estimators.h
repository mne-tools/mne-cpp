//=============================================================================================================
/**
 * @file     sts_cov_estimators.h
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
 * @brief    StsCovEstimators class declaration.
 *
 */

#ifndef STS_COV_ESTIMATORS_H
#define STS_COV_ESTIMATORS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB {

//=============================================================================================================
/**
 * Covariance matrix estimators.
 *
 * @brief Covariance matrix estimators including shrinkage methods.
 */
class STSSHARED_EXPORT StsCovEstimators
{
public:
    /**
     * @brief Ledoit-Wolf optimal shrinkage covariance estimator.
     *
     * Computes the shrinkage coefficient analytically using the formula from
     * Ledoit & Wolf (2004) "A well-conditioned estimator for large-dimensional
     * covariance matrices" (Journal of Multivariate Analysis, 88(2), 365-411).
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     *
     * @return std::pair containing the shrunk covariance matrix (n_channels x n_channels)
     *         and the shrinkage coefficient alpha in [0,1].
     */
    static std::pair<Eigen::MatrixXd, double> ledoitWolf(const Eigen::MatrixXd& matData);
};

} // namespace STSLIB

#endif // STS_COV_ESTIMATORS_H
