//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_correction.h
 * @since April 2026
 * @brief Family-wise error and false-discovery-rate corrections for mass-univariate p-value maps.
 *
 * Provides the three multiple-comparison adjustments most commonly
 * reported in M/EEG papers: classical Bonferroni
 * (@f$p^* = \min(np, 1)@f$), the Holm-Bonferroni step-down procedure
 * that gives uniformly tighter family-wise error control than
 * Bonferroni while remaining distribution-free, and the
 * Benjamini-Hochberg FDR step-up procedure that controls the expected
 * proportion of false discoveries rather than the probability of any.
 *
 * All three routines operate on an Eigen matrix of raw p-values and
 * return a matrix of the same shape with corrected p-values; the caller
 * compares against the desired @f$\alpha@f$ as usual. Cluster-based
 * correction (the Maris-Oostenveld alternative to FDR for spatially
 * structured data) lives in @ref STSLIB::StatsCluster.
 *
 * References: Holm (1979), Scandinavian Journal of Statistics 6(2);
 * Benjamini & Hochberg (1995), JRSS-B 57(1).
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
 * @brief Bonferroni, Holm-Bonferroni and Benjamini-Hochberg FDR adjustments for mass-univariate p-value maps.
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
