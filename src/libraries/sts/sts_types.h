//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_types.h
 * @since April 2026
 * @brief Shared enumerations describing tail direction and multiple-comparison correction strategy.
 *
 * Centralising the @ref STSLIB::StatsTailType (left / right / both) and
 * @ref STSLIB::StatsCorrection (none, Bonferroni, FDR, cluster
 * permutation) enums in a header with no Qt or Eigen dependency lets
 * lightweight modules - notably @ref sts_ttest and @ref sts_ftest - take
 * the same tail argument as the cluster permutation entry points in
 * @ref sts_cluster without dragging the rest of STSLIB into client
 * translation units.
 */

#ifndef STS_TYPES_H
#define STS_TYPES_H

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB
{

//=============================================================================================================
/**
 * Tail type for statistical tests.
 *
 * @brief Direction of the alternative hypothesis for a t- or F-test (left, right, or two-sided).
 */
enum class StatsTailType {
    Left,       /**< Left-tailed test. */
    Right,      /**< Right-tailed test. */
    Both        /**< Two-tailed test. */
};

//=============================================================================================================
/**
 * Multiple comparison correction method.
 *
 * @brief Strategy used to control false positives when a test is repeated across many (channel, time) or (vertex, time) samples.
 */
enum class StatsCorrection {
    None,               /**< No correction. */
    Bonferroni,         /**< Bonferroni correction. */
    Fdr,                /**< False Discovery Rate (Benjamini-Hochberg). */
    ClusterPermutation  /**< Cluster-based permutation test. */
};

} // namespace STSLIB

#endif // STS_TYPES_H
