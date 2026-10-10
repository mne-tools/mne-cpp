//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_correction.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Family-wise error and false-discovery-rate corrections for mass-univariate p-value maps.
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
 *
 * @snippet ex_sts_statistics/main.cpp stats_mc_correction_usage
 */
class STSSHARED_EXPORT StatsMcCorrection
{
public:
    //=========================================================================================================
    /**
     * Hypotheses rejected by a correction, same shape as the p-values.
     */
    using RejectMask = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>;

    /**
     * Dependence assumed by the FDR correction (mne's method "indep" / "negcorr").
     */
    enum class FdrMethod
    {
        Independent,         /**< Benjamini-Hochberg: independent or positively correlated tests. */
        NegativelyCorrelated /**< Benjamini-Yekutieli: arbitrary dependence. */
    };

    //=========================================================================================================
    /**
     * Bonferroni correction as mne.stats.bonferroni_correction: corrected_p = min(p * n, 1.0).
     *
     * @param[in]  pValues  Matrix of p-values.
     * @param[in]  alpha    Significance level for @p reject.
     * @param[out] reject   If not null, receives corrected_p < alpha.
     *
     * @return Corrected p-values (same shape as input).
     */
    static Eigen::MatrixXd bonferroni(const Eigen::MatrixXd& pValues, double alpha = 0.05, RejectMask* reject = nullptr);

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
     * False Discovery Rate correction as mne.stats.fdr_correction.
     *
     * @param[in]  pValues  Matrix of p-values.
     * @param[in]  alpha    False discovery rate for @p reject.
     * @param[in]  method   Benjamini-Hochberg or Benjamini-Yekutieli.
     * @param[out] reject   If not null, receives the hypotheses rejected by the step-up procedure.
     *
     * @return Corrected p-values (same shape as input).
     */
    static Eigen::MatrixXd fdr(const Eigen::MatrixXd& pValues,
                               double alpha = 0.05,
                               FdrMethod method = FdrMethod::Independent,
                               RejectMask* reject = nullptr);
};

} // namespace STSLIB

#endif // STS_CORRECTION_H
