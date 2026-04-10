//=============================================================================================================
/**
 * @file     sts_correction.h
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
 * @brief    StatsMcCorrection class declaration.
 *
 */

#ifndef STS_CORRECTION_H
#define STS_CORRECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB
{

//=============================================================================================================
/**
 * Multiple comparison correction methods.
 *
 * @brief Multiple comparison corrections (Bonferroni, Holm-Bonferroni, FDR).
 */
class STSSHARED_EXPORT StatsMcCorrection
{
public:
    //=========================================================================================================
    /**
     * Bonferroni correction: corrected_p = min(p * n, 1.0).
     *
     * @param[in] pValues  Matrix of p-values.
     *
     * @return Corrected p-values (same shape as input).
     */
    static Eigen::MatrixXd bonferroni(const Eigen::MatrixXd& pValues);

    //=========================================================================================================
    /**
     * Holm-Bonferroni step-down correction.
     *
     * @param[in] pValues  Matrix of p-values.
     *
     * @return Corrected p-values (same shape as input).
     */
    static Eigen::MatrixXd holmBonferroni(const Eigen::MatrixXd& pValues);

    //=========================================================================================================
    /**
     * False Discovery Rate (Benjamini-Hochberg) correction.
     *
     * @param[in] pValues  Matrix of p-values.
     * @param[in] alpha    Significance level (default 0.05, used for reference only; correction is applied regardless).
     *
     * @return Corrected p-values (same shape as input).
     */
    static Eigen::MatrixXd fdr(const Eigen::MatrixXd& pValues, double alpha = 0.05);
};

} // namespace STSLIB

#endif // STS_CORRECTION_H
