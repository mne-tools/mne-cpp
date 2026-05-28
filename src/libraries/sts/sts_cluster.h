//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_cluster.h
 * @since 2026
 * @date  April 2026
 * @brief Maris-Oostenveld cluster-mass permutation tests and Threshold-Free Cluster Enhancement for M/EEG inference.
 *
 * Mass-univariate t- or F-tests on dense (channel, time) or (vertex, time)
 * grids generate a severe multiple-comparison problem. This module
 * implements the standard fix: form an observed statistic map, threshold
 * it at an a-priori cluster-forming level, group the supra-threshold
 * samples into spatially (or spatio-temporally) connected clusters using
 * a sparse adjacency graph from @ref STSLIB::StatsAdjacency and assign
 * each cluster a cluster-mass test statistic equal to the sum of the
 * underlying t- or F-values.
 *
 * The null distribution of the maximum cluster-mass is then built by
 * Monte-Carlo permutation: condition labels are shuffled for the
 * two-sample test, signs are flipped for the one-sample test, and
 * groups are reassigned for the one-way ANOVA variant. The exchangeable
 * label / sign-flip framework controls family-wise error in the strong
 * sense at the cluster level. The module also exposes Threshold-Free
 * Cluster Enhancement (TFCE), which integrates cluster extent and height
 * over a range of thresholds and removes the arbitrary cluster-forming
 * threshold.
 *
 * References: Maris & Oostenveld (2007), J. Neurosci. Methods 164(1);
 * Smith & Nichols (2009), NeuroImage 44(1).
 */

#ifndef STS_CLUSTER_H
#define STS_CLUSTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"
#include "sts_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPair>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB
{

//=============================================================================================================
/**
 * Result structure for cluster permutation tests.
 *
 * @brief Per-call output of a cluster permutation test: observed statistic map, cluster masses, cluster p-values and cluster labels.
 */
struct STSSHARED_EXPORT StatsClusterResult {
    Eigen::MatrixXd matTObs;            /**< Observed t-statistic map (nChannels x nTimes). */
    QVector<double> vecClusterStats;    /**< Sum of t-values for each observed cluster. */
    QVector<double> vecClusterPvals;    /**< p-value for each observed cluster. */
    Eigen::MatrixXi matClusterIds;      /**< Cluster label at each (channel, time) point; 0 = not in cluster. */
    double clusterThreshold;            /**< t-threshold used for clustering. */
};

//=============================================================================================================
/**
 * Cluster-based permutation test for comparing two conditions.
 *
 * @brief Maris-Oostenveld cluster-mass permutation tests and Threshold-Free Cluster Enhancement on (channel,time) or (vertex,time) statistic maps.
 */
class STSSHARED_EXPORT StatsCluster
{
public:
    //=========================================================================================================
    /**
     * Cluster-based permutation test.
     *
     * @param[in] dataA           Per-subject data for condition A. Each matrix is nChannels x nTimes.
     * @param[in] dataB           Per-subject data for condition B. Each matrix is nChannels x nTimes.
     * @param[in] adjacency       Spatial adjacency matrix (nChannels x nChannels).
     * @param[in] nPermutations   Number of permutations (default 1024).
     * @param[in] clusterAlpha    Alpha level for initial thresholding (default 0.05).
     * @param[in] pThreshold      p-value threshold for reporting significant clusters (default 0.05).
     * @param[in] tail            Tail type for the test.
     *
     * @return StatsClusterResult with observed t-map, cluster statistics, and p-values.
     */
    static StatsClusterResult permutationTest(
        const QVector<Eigen::MatrixXd>& dataA,
        const QVector<Eigen::MatrixXd>& dataB,
        const Eigen::SparseMatrix<int>& adjacency,
        int nPermutations = 1024,
        double clusterAlpha = 0.05,
        double pThreshold = 0.05,
        StatsTailType tail = StatsTailType::Both);

    //=========================================================================================================
    /**
     * One-sample cluster-based permutation test.
     *
     * Tests whether the mean across subjects differs from zero at each vertex x time
     * point using sign-flip permutations.
     *
     * @param[in] data            Per-subject data. Each matrix is nVertices x nTimes.
     * @param[in] adjacency       Spatio-temporal adjacency matrix (nVertices*nTimes x nVertices*nTimes).
     * @param[in] threshold       Cluster-forming t-threshold.
     * @param[in] nPermutations   Number of sign-flip permutations.
     * @param[in] tail            Tail type for the test.
     *
     * @return StatsClusterResult with observed t-map, cluster statistics, and p-values.
     *
     * @since 2.2.0
     */
    static StatsClusterResult oneSamplePermutationTest(
        const QVector<Eigen::MatrixXd>& data,
        const Eigen::SparseMatrix<int>& adjacency,
        double threshold,
        int nPermutations,
        StatsTailType tail);

    //=========================================================================================================
    /**
     * F-test cluster-based permutation test for one-way ANOVA.
     *
     * Tests for differences across conditions by randomly reassigning condition
     * labels and computing max cluster F-statistics.
     *
     * @param[in] conditions      Vector of conditions, each containing per-subject matrices (nVertices x nTimes).
     * @param[in] adjacency       Spatio-temporal adjacency matrix (nVertices*nTimes x nVertices*nTimes).
     * @param[in] threshold       Cluster-forming F-threshold.
     * @param[in] nPermutations   Number of permutations.
     *
     * @return StatsClusterResult with observed F-map (in matTObs), cluster statistics, and p-values.
     *
     * @since 2.2.0
     */
    static StatsClusterResult fTestPermutationTest(
        const QVector<QVector<Eigen::MatrixXd>>& conditions,
        const Eigen::SparseMatrix<int>& adjacency,
        double threshold,
        int nPermutations);

    //=========================================================================================================
    /**
     * Threshold-Free Cluster Enhancement (TFCE).
     *
     * Enhances a statistic map by integrating cluster extent and height over a range
     * of thresholds (Smith & Nichols 2009). Handles both positive and negative values.
     *
     * @param[in] statMap     Statistic map (nVertices x nTimes).
     * @param[in] adjacency   Spatio-temporal adjacency matrix (nVertices*nTimes x nVertices*nTimes).
     * @param[in] E           Extent exponent (default 0.5).
     * @param[in] H           Height exponent (default 2.0).
     * @param[in] nSteps      Number of threshold steps (default 100).
     *
     * @return TFCE-enhanced score map (nVertices x nTimes).
     *
     * @since 2.2.0
     */
    static Eigen::MatrixXd tfce(
        const Eigen::MatrixXd& statMap,
        const Eigen::SparseMatrix<int>& adjacency,
        double E = 0.5,
        double H = 2.0,
        int nSteps = 100);

private:
    //=========================================================================================================
    /**
     * Compute a paired t-statistic map from two groups of per-subject matrices.
     */
    static Eigen::MatrixXd computeTMap(
        const QVector<Eigen::MatrixXd>& dataA,
        const QVector<Eigen::MatrixXd>& dataB);

    //=========================================================================================================
    /**
     * Find connected clusters of supra-threshold cells using BFS on spatial+temporal adjacency.
     *
     * @return Pair of (cluster ID matrix, vector of cluster statistic sums).
     */
    static QPair<Eigen::MatrixXi, QVector<double>> findClusters(
        const Eigen::MatrixXd& tMap,
        double threshold,
        const Eigen::SparseMatrix<int>& adjacency,
        StatsTailType tail);

    //=========================================================================================================
    /**
     * Perform one permutation: shuffle labels, compute t-map, return max cluster statistic.
     */
    static double permuteOnce(
        const QVector<Eigen::MatrixXd>& allData,
        int nA,
        const Eigen::SparseMatrix<int>& adjacency,
        double threshold,
        StatsTailType tail);

    //=========================================================================================================
    /**
     * Approximate the inverse t CDF (quantile function) for thresholding.
     */
    static double inverseTCdf(double p, int df);

    //=========================================================================================================
    /**
     * Compute a one-sample t-statistic map: t = mean / (std / sqrt(n)).
     */
    static Eigen::MatrixXd computeOneSampleTMap(
        const QVector<Eigen::MatrixXd>& data);

    //=========================================================================================================
    /**
     * Compute a one-way ANOVA F-statistic map across conditions.
     */
    static Eigen::MatrixXd computeFMap(
        const QVector<QVector<Eigen::MatrixXd>>& conditions);

    //=========================================================================================================
    /**
     * Find connected clusters on a flattened spatio-temporal adjacency matrix.
     *
     * @return Pair of (cluster ID matrix, vector of cluster statistic sums).
     */
    static QPair<Eigen::MatrixXi, QVector<double>> findClustersFlat(
        const Eigen::MatrixXd& statMap,
        double threshold,
        const Eigen::SparseMatrix<int>& adjacency,
        bool positiveOnly);

    //=========================================================================================================
    /**
     * Perform one sign-flip permutation for one-sample test.
     */
    static double permuteOnceOneSample(
        const QVector<Eigen::MatrixXd>& data,
        const Eigen::SparseMatrix<int>& adjacency,
        double threshold,
        StatsTailType tail);

    //=========================================================================================================
    /**
     * Perform one label-shuffle permutation for F-test.
     */
    static double permuteOnceFTest(
        const QVector<Eigen::MatrixXd>& allData,
        const QVector<int>& groupSizes,
        const Eigen::SparseMatrix<int>& adjacency,
        double threshold);
};

} // namespace STSLIB

#endif // STS_CLUSTER_H
