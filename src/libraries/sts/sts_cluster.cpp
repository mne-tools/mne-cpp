//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_cluster.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the cluster permutation tests and TFCE enhancement declared in sts_cluster.h.
 *
 * Each permutation entry point first computes the observed statistic map
 * (paired or one-sample t for the t-variants, one-way ANOVA F for the
 * @c fTestPermutationTest) and derives the cluster-forming threshold
 * from @c clusterAlpha through the inverse Student-t CDF. Connected
 * supra-threshold components are extracted with a queue-based
 * breadth-first search over the @ref mne::StatsAdjacency sparse
 * graph and each component is assigned a cluster-mass equal to the sum
 * of its t- or F-values.
 *
 * The null distribution of the maximum absolute cluster-mass is then
 * built by @c mne::ThreadPool::parallel_map over @c nPermutations
 * randomisations - exchangeable label shuffles for the two-sample test,
 * Rademacher sign flips for the one-sample test, and group-label
 * reassignments for the ANOVA test - with thread-local
 * @c std::mt19937 instances for reproducibility. Observed cluster
 * p-values count the observed statistic as one draw from that null, as
 * MNE-Python does: p = (1 + #exceedances) / (1 + nPermutations).
 *
 * TFCE follows Smith & Nichols (2009): the statistic map is swept by
 * @c nSteps thresholds below the largest absolute statistic, as MNE-Python
 * does, and each supra-threshold connected component
 * contributes @f$\text{extent}^E \cdot h^H \cdot \Delta h@f$ to every
 * sample it covers. Positive and negative tails are handled separately
 * and recombined so signed enhancement is preserved.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_cluster.h"
#include "sts_ttest.h"
#include "sts_ftest.h"

#include <base/thread_pool.h>

#include <cmath>
#include <algorithm>
#include <queue>
#include <random>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

StatsClusterResult StatsCluster::permutationTest(
    const std::vector<MatrixXd>& dataA,
    const std::vector<MatrixXd>& dataB,
    const SparseMatrix<int>& adjacency,
    int nPermutations,
    double clusterAlpha,
    double pThreshold,
    StatsTailType tail)
{
    static_cast<void>(pThreshold);

    const int nA = static_cast<int>(dataA.size());

    // Step 1: Compute the observed t-map
    MatrixXd tObs = computeTMap(dataA, dataB);

    // Step 2: Determine cluster-forming threshold from clusterAlpha and df.
    // inverseTCdf is two-tailed; a one-tailed test puts all of clusterAlpha in one tail (as mne-python).
    int df = nA - 1;
    double threshold = inverseTCdf(tail == StatsTailType::Both ? clusterAlpha : 2.0 * clusterAlpha, df);

    // Step 3: Find observed clusters
    auto [clusterIds, clusterStats] = findClusters(tObs, threshold, adjacency, tail);

    // Step 4: Combine all data for permutation
    std::vector<MatrixXd> allData;
    allData.reserve(nA + dataB.size());
    for (const auto& m : dataA)
        allData.push_back(m);
    for (const auto& m : dataB)
        allData.push_back(m);

    // Step 5: Build null distribution via permutations on the thread pool
    // Perform permutations in parallel
    std::vector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnce(allData, nA, adjacency, threshold, tail);
    };

    std::vector<double> nullDist = mne::ThreadPool::global().parallel_map(permIndices, permuteFunc);

    // Sort null distribution for percentile computation
    std::sort(nullDist.begin(), nullDist.end());

    // Step 6: Compute cluster p-values
    std::vector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < static_cast<int>(clusterStats.size()); ++c) {
        double obsStat = std::fabs(clusterStats[c]);
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count + 1) / static_cast<double>(nPermutations + 1);
    }

    StatsClusterResult result;
    result.matTObs = tObs;
    result.vecClusterStats = clusterStats;
    result.vecClusterPvals = clusterPvals;
    result.matClusterIds = clusterIds;
    result.clusterThreshold = threshold;
    return result;
}

//=============================================================================================================

MatrixXd StatsCluster::computeTMap(const std::vector<MatrixXd>& dataA, const std::vector<MatrixXd>& dataB)
{
    const int nSubjects = static_cast<int>(dataA.size());
    const int nChannels = static_cast<int>(dataA[0].rows());
    const int nTimes = static_cast<int>(dataA[0].cols());

    // Compute difference for each subject
    // Then compute paired t-statistic at each (channel, time)
    MatrixXd sumDiff = MatrixXd::Zero(nChannels, nTimes);
    MatrixXd sumDiffSq = MatrixXd::Zero(nChannels, nTimes);

    for (int s = 0; s < nSubjects; ++s) {
        MatrixXd diff = dataA[s] - dataB[s];
        sumDiff += diff;
        sumDiffSq += diff.cwiseProduct(diff);
    }

    double n = static_cast<double>(nSubjects);
    MatrixXd mean = sumDiff / n;
    MatrixXd variance = (sumDiffSq - sumDiff.cwiseProduct(sumDiff) / n) / (n - 1.0);

    // Clamp variance to avoid division by zero
    variance = variance.cwiseMax(1.0e-30);

    MatrixXd tMap = mean.array() / (variance.array().sqrt() / std::sqrt(n));
    return tMap;
}

//=============================================================================================================

std::pair<MatrixXi, std::vector<double>> StatsCluster::findClusters(
    const MatrixXd& tMap,
    double threshold,
    const SparseMatrix<int>& adjacency,
    StatsTailType tail)
{
    const int nChannels = static_cast<int>(tMap.rows());
    const int nTimes = static_cast<int>(tMap.cols());

    MatrixXi clusterIds = MatrixXi::Zero(nChannels, nTimes);
    std::vector<double> clusterStats;
    int currentClusterId = 0;

    // Helper lambda: BFS on combined spatial+temporal adjacency for one polarity
    auto bfsClusters = [&](bool positive) {
        // Build a visited mask
        MatrixXi visited = MatrixXi::Zero(nChannels, nTimes);

        for (int ch = 0; ch < nChannels; ++ch) {
            for (int t = 0; t < nTimes; ++t) {
                double val = tMap(ch, t);
                bool suprathreshold = positive ? (val > threshold) : (val < -threshold);

                if (!suprathreshold || visited(ch, t))
                    continue;

                // Start BFS for a new cluster
                currentClusterId++;
                double clusterSum = 0.0;
                std::queue<std::pair<int, int>> queue;
                queue.push({ch, t});
                visited(ch, t) = 1;

                while (!queue.empty()) {
                    auto [curCh, curT] = queue.front();
                    queue.pop();

                    clusterIds(curCh, curT) = positive ? currentClusterId : -currentClusterId;
                    clusterSum += tMap(curCh, curT);

                    // Temporal neighbors (consecutive time points)
                    for (int dt = -1; dt <= 1; dt += 2) {
                        int nt = curT + dt;
                        if (nt < 0 || nt >= nTimes)
                            continue;
                        double nval = tMap(curCh, nt);
                        bool nSupra = positive ? (nval > threshold) : (nval < -threshold);
                        if (nSupra && !visited(curCh, nt)) {
                            visited(curCh, nt) = 1;
                            queue.push({curCh, nt});
                        }
                    }

                    // Spatial neighbors at the same time point
                    for (SparseMatrix<int>::InnerIterator it(adjacency, curCh); it; ++it) {
                        int nCh = static_cast<int>(it.row());
                        double nval = tMap(nCh, curT);
                        bool nSupra = positive ? (nval > threshold) : (nval < -threshold);
                        if (nSupra && !visited(nCh, curT)) {
                            visited(nCh, curT) = 1;
                            queue.push({nCh, curT});
                        }
                    }
                }

                clusterStats.push_back(clusterSum);
            }
        }
    };

    if (tail == StatsTailType::Both || tail == StatsTailType::Right) {
        bfsClusters(true);
    }
    if (tail == StatsTailType::Both || tail == StatsTailType::Left) {
        bfsClusters(false);
    }

    return {clusterIds, clusterStats};
}

//=============================================================================================================

double StatsCluster::permuteOnce(
    const std::vector<MatrixXd>& allData,
    int nA,
    const SparseMatrix<int>& adjacency,
    double threshold,
    StatsTailType tail)
{
    const int nTotal = static_cast<int>(allData.size());

    // Generate a random permutation of indices
    std::vector<int> indices(nTotal);
    std::iota(indices.begin(), indices.end(), 0);

    // Use thread-safe random generator
    std::mt19937 rng(std::random_device{}());
    for (int i = nTotal - 1; i > 0; --i) {
        int j = std::uniform_int_distribution<int>(0, i)(rng);
        std::swap(indices[i], indices[j]);
    }

    // Split into permuted groups
    std::vector<MatrixXd> permA, permB;
    permA.reserve(nA);
    permB.reserve(nTotal - nA);
    for (int i = 0; i < nTotal; ++i) {
        if (i < nA) {
            permA.push_back(allData[indices[i]]);
        } else {
            permB.push_back(allData[indices[i]]);
        }
    }

    // Compute t-map and find clusters
    MatrixXd tMap = computeTMap(permA, permB);
    auto [clusterIds, clusterStats] = findClusters(tMap, threshold, adjacency, tail);

    // Return the maximum absolute cluster statistic
    double maxStat = 0.0;
    for (double s : clusterStats) {
        double absS = std::fabs(s);
        if (absS > maxStat) {
            maxStat = absS;
        }
    }
    return maxStat;
}

//=============================================================================================================

double StatsCluster::inverseTCdf(double p, int df)
{
    // Approximate inverse t CDF for two-tailed threshold.
    // We want the t-value such that P(|T| > t) = p, i.e., P(T > t) = p/2.
    // This means we want the (1 - p/2) quantile.

    // Use a simple bisection on the tCdf function.
    double targetCdf = 1.0 - p / 2.0;

    double lo = 0.0;
    double hi = 100.0;

    for (int iter = 0; iter < 100; ++iter) {
        double mid = (lo + hi) / 2.0;
        double cdf = StatsTtest::tCdf(mid, df);
        if (cdf < targetCdf) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return (lo + hi) / 2.0;
}

//=============================================================================================================

MatrixXd StatsCluster::computeOneSampleTMap(const std::vector<MatrixXd>& data)
{
    const int nSubjects = static_cast<int>(data.size());
    const int nVertices = static_cast<int>(data[0].rows());
    const int nTimes = static_cast<int>(data[0].cols());

    MatrixXd sum = MatrixXd::Zero(nVertices, nTimes);
    MatrixXd sumSq = MatrixXd::Zero(nVertices, nTimes);

    for (int s = 0; s < nSubjects; ++s) {
        sum += data[s];
        sumSq += data[s].cwiseProduct(data[s]);
    }

    double n = static_cast<double>(nSubjects);
    MatrixXd mean = sum / n;
    MatrixXd variance = (sumSq - sum.cwiseProduct(sum) / n) / (n - 1.0);
    variance = variance.cwiseMax(1.0e-30);

    MatrixXd tMap = mean.array() / (variance.array().sqrt() / std::sqrt(n));
    return tMap;
}

//=============================================================================================================

MatrixXd StatsCluster::computeFMap(const std::vector<std::vector<MatrixXd>>& conditions)
{
    const int nConditions = static_cast<int>(conditions.size());
    const int nVertices = static_cast<int>(conditions[0][0].rows());
    const int nTimes = static_cast<int>(conditions[0][0].cols());

    // Count total subjects and build per-condition means
    int nTotal = 0;
    for (int c = 0; c < nConditions; ++c) {
        nTotal += static_cast<int>(conditions[c].size());
    }

    // Grand mean
    MatrixXd grandSum = MatrixXd::Zero(nVertices, nTimes);
    for (int c = 0; c < nConditions; ++c) {
        for (int s = 0; s < static_cast<int>(conditions[c].size()); ++s) {
            grandSum += conditions[c][s];
        }
    }
    MatrixXd grandMean = grandSum / static_cast<double>(nTotal);

    // SS between and SS within
    MatrixXd ssBetween = MatrixXd::Zero(nVertices, nTimes);
    MatrixXd ssWithin = MatrixXd::Zero(nVertices, nTimes);

    for (int c = 0; c < nConditions; ++c) {
        const auto nc = static_cast<int>(conditions[c].size());
        MatrixXd condSum = MatrixXd::Zero(nVertices, nTimes);
        for (int s = 0; s < nc; ++s) {
            condSum += conditions[c][s];
        }
        MatrixXd condMean = condSum / static_cast<double>(nc);

        MatrixXd diff = condMean - grandMean;
        ssBetween += static_cast<double>(nc) * diff.cwiseProduct(diff);

        for (int s = 0; s < nc; ++s) {
            MatrixXd residual = conditions[c][s] - condMean;
            ssWithin += residual.cwiseProduct(residual);
        }
    }

    int dfBetween = nConditions - 1;
    int dfWithin = nTotal - nConditions;

    MatrixXd msBetween = ssBetween / static_cast<double>(dfBetween);
    MatrixXd msWithin = ssWithin / static_cast<double>(dfWithin);
    msWithin = msWithin.cwiseMax(1.0e-30);

    MatrixXd fMap = msBetween.array() / msWithin.array();
    return fMap;
}

//=============================================================================================================

std::pair<MatrixXi, std::vector<double>> StatsCluster::findClustersFlat(
    const MatrixXd& statMap,
    double threshold,
    const SparseMatrix<int>& adjacency,
    bool positiveOnly)
{
    const int nVertices = static_cast<int>(statMap.rows());
    const int nTimes = static_cast<int>(statMap.cols());
    const int nTotal = nVertices * nTimes;

    MatrixXi clusterIds = MatrixXi::Zero(nVertices, nTimes);
    std::vector<double> clusterStats;
    int currentClusterId = 0;

    // BFS on the flat spatio-temporal adjacency
    std::vector<bool> visited(nTotal, false);

    for (int idx = 0; idx < nTotal; ++idx) {
        int v = idx / nTimes;
        int t = idx % nTimes;
        double val = statMap(v, t);

        bool suprathreshold = positiveOnly ? (val > threshold) : (val < -threshold);
        if (!suprathreshold || visited[idx])
            continue;

        currentClusterId++;
        double clusterSum = 0.0;
        std::queue<int> queue;
        queue.push(idx);
        visited[idx] = true;

        while (!queue.empty()) {
            int curIdx = queue.front();
            queue.pop();

            int curV = curIdx / nTimes;
            int curT = curIdx % nTimes;

            clusterIds(curV, curT) = positiveOnly ? currentClusterId : -currentClusterId;
            clusterSum += statMap(curV, curT);

            // Iterate over adjacency neighbors
            for (SparseMatrix<int>::InnerIterator it(adjacency, curIdx); it; ++it) {
                int nIdx = static_cast<int>(it.row());
                if (visited[nIdx])
                    continue;
                int nV = nIdx / nTimes;
                int nT = nIdx % nTimes;
                double nval = statMap(nV, nT);
                bool nSupra = positiveOnly ? (nval > threshold) : (nval < -threshold);
                if (nSupra) {
                    visited[nIdx] = true;
                    queue.push(nIdx);
                }
            }
        }

        clusterStats.push_back(clusterSum);
    }

    return {clusterIds, clusterStats};
}

//=============================================================================================================

double StatsCluster::permuteOnceOneSample(
    const std::vector<MatrixXd>& data,
    const SparseMatrix<int>& adjacency,
    double threshold,
    StatsTailType tail)
{
    const int nSubjects = static_cast<int>(data.size());

    // Generate random sign flips
    std::mt19937 rng(std::random_device{}());
    std::vector<int> signs(nSubjects);
    for (int s = 0; s < nSubjects; ++s) {
        signs[s] = std::uniform_int_distribution<int>(0, 1)(rng) == 0 ? 1 : -1;
    }

    // Apply sign flips
    std::vector<MatrixXd> flipped(nSubjects);
    for (int s = 0; s < nSubjects; ++s) {
        flipped[s] = data[s] * static_cast<double>(signs[s]);
    }

    // Compute t-map and find clusters
    MatrixXd tMap = computeOneSampleTMap(flipped);

    double maxStat = 0.0;

    if (tail == StatsTailType::Both || tail == StatsTailType::Right) {
        auto [ids, stats] = findClustersFlat(tMap, threshold, adjacency, true);
        for (double s : stats) {
            if (std::fabs(s) > maxStat)
                maxStat = std::fabs(s);
        }
    }
    if (tail == StatsTailType::Both || tail == StatsTailType::Left) {
        auto [ids, stats] = findClustersFlat(tMap, threshold, adjacency, false);
        for (double s : stats) {
            if (std::fabs(s) > maxStat)
                maxStat = std::fabs(s);
        }
    }

    return maxStat;
}

//=============================================================================================================

double StatsCluster::permuteOnceFTest(
    const std::vector<MatrixXd>& allData,
    const std::vector<int>& groupSizes,
    const SparseMatrix<int>& adjacency,
    double threshold)
{
    const int nTotal = static_cast<int>(allData.size());

    // Random permutation of indices
    std::vector<int> indices(nTotal);
    std::iota(indices.begin(), indices.end(), 0);

    std::mt19937 rng(std::random_device{}());
    for (int i = nTotal - 1; i > 0; --i) {
        int j = std::uniform_int_distribution<int>(0, i)(rng);
        std::swap(indices[i], indices[j]);
    }

    // Rebuild condition groups
    std::vector<std::vector<MatrixXd>> permConditions;
    int offset = 0;
    for (int c = 0; c < static_cast<int>(groupSizes.size()); ++c) {
        std::vector<MatrixXd> group;
        group.reserve(groupSizes[c]);
        for (int s = 0; s < groupSizes[c]; ++s) {
            group.push_back(allData[indices[offset + s]]);
        }
        permConditions.push_back(group);
        offset += groupSizes[c];
    }

    // Compute F-map and find clusters (F is always positive)
    MatrixXd fMap = computeFMap(permConditions);
    auto [ids, stats] = findClustersFlat(fMap, threshold, adjacency, true);

    double maxStat = 0.0;
    for (double s : stats) {
        if (s > maxStat)
            maxStat = s;
    }
    return maxStat;
}

//=============================================================================================================

StatsClusterResult StatsCluster::oneSamplePermutationTest(
    const std::vector<MatrixXd>& data,
    const SparseMatrix<int>& adjacency,
    double threshold,
    int nPermutations,
    StatsTailType tail)
{
    // Step 1: Compute observed one-sample t-map
    MatrixXd tObs = computeOneSampleTMap(data);

    // Step 2: Find observed clusters
    MatrixXi clusterIdsPos, clusterIdsNeg;
    std::vector<double> clusterStats;

    if (tail == StatsTailType::Both || tail == StatsTailType::Right) {
        auto [ids, stats] = findClustersFlat(tObs, threshold, adjacency, true);
        clusterIdsPos = ids;
        clusterStats.insert(clusterStats.end(), stats.begin(), stats.end());
    }
    if (tail == StatsTailType::Both || tail == StatsTailType::Left) {
        auto [ids, stats] = findClustersFlat(tObs, threshold, adjacency, false);
        clusterIdsNeg = ids;
        clusterStats.insert(clusterStats.end(), stats.begin(), stats.end());
    }

    // Merge cluster ID maps
    MatrixXi clusterIds = MatrixXi::Zero(tObs.rows(), tObs.cols());
    if (clusterIdsPos.size() > 0)
        clusterIds += clusterIdsPos;
    if (clusterIdsNeg.size() > 0)
        clusterIds += clusterIdsNeg;

    // Step 3: Build null distribution via sign-flip permutations
    std::vector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnceOneSample(data, adjacency, threshold, tail);
    };

    std::vector<double> nullDist = mne::ThreadPool::global().parallel_map(permIndices, permuteFunc);

    std::sort(nullDist.begin(), nullDist.end());

    // Step 4: Compute cluster p-values
    std::vector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < static_cast<int>(clusterStats.size()); ++c) {
        double obsStat = std::fabs(clusterStats[c]);
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count + 1) / static_cast<double>(nPermutations + 1);
    }

    StatsClusterResult result;
    result.matTObs = tObs;
    result.vecClusterStats = clusterStats;
    result.vecClusterPvals = clusterPvals;
    result.matClusterIds = clusterIds;
    result.clusterThreshold = threshold;
    return result;
}

//=============================================================================================================

StatsClusterResult StatsCluster::fTestPermutationTest(
    const std::vector<std::vector<MatrixXd>>& conditions,
    const SparseMatrix<int>& adjacency,
    double threshold,
    int nPermutations)
{
    // Step 1: Compute observed F-map
    MatrixXd fObs = computeFMap(conditions);

    // Step 2: Find observed clusters (F is always positive, one-tailed)
    auto [clusterIds, clusterStats] = findClustersFlat(fObs, threshold, adjacency, true);

    // Step 3: Flatten all data and record group sizes
    std::vector<MatrixXd> allData;
    std::vector<int> groupSizes;
    for (int c = 0; c < static_cast<int>(conditions.size()); ++c) {
        groupSizes.push_back(static_cast<int>(conditions[c].size()));
        for (int s = 0; s < static_cast<int>(conditions[c].size()); ++s) {
            allData.push_back(conditions[c][s]);
        }
    }

    // Step 4: Build null distribution via label-shuffle permutations
    std::vector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnceFTest(allData, groupSizes, adjacency, threshold);
    };

    std::vector<double> nullDist = mne::ThreadPool::global().parallel_map(permIndices, permuteFunc);

    std::sort(nullDist.begin(), nullDist.end());

    // Step 5: Compute cluster p-values
    std::vector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < static_cast<int>(clusterStats.size()); ++c) {
        double obsStat = clusterStats[c];
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count + 1) / static_cast<double>(nPermutations + 1);
    }

    StatsClusterResult result;
    result.matTObs = fObs;
    result.vecClusterStats = clusterStats;
    result.vecClusterPvals = clusterPvals;
    result.matClusterIds = clusterIds;
    result.clusterThreshold = threshold;
    return result;
}

//=============================================================================================================

MatrixXd StatsCluster::tfce(
    const MatrixXd& statMap,
    const SparseMatrix<int>& adjacency,
    double E,
    double H,
    int nSteps)
{
    const int nVertices = static_cast<int>(statMap.rows());
    const int nTimes = static_cast<int>(statMap.cols());
    const int nTotal = nVertices * nTimes;

    MatrixXd tfceMap = MatrixXd::Zero(nVertices, nTimes);

    // Like mne's threshold=dict(start=dh, step=dh): both signs share the nSteps levels below the largest |statistic|
    const double dh = statMap.cwiseAbs().maxCoeff() / static_cast<double>(nSteps + 1);
    if (dh <= 0.0)
        return tfceMap;

    // Helper: run TFCE on one polarity
    auto runTfce = [&](const MatrixXd& absMap, bool isPositive) {
        for (int step = 1; step <= nSteps; ++step) {
            double h = dh * static_cast<double>(step);

            // Find connected components above threshold h
            std::vector<bool> visited(nTotal, false);

            for (int idx = 0; idx < nTotal; ++idx) {
                int v = idx / nTimes;
                int t = idx % nTimes;

                if (absMap(v, t) <= h || visited[idx])
                    continue;

                // BFS to find cluster
                std::vector<int> cluster;
                std::queue<int> queue;
                queue.push(idx);
                visited[idx] = true;

                while (!queue.empty()) {
                    int curIdx = queue.front();
                    queue.pop();
                    cluster.push_back(curIdx);

                    for (SparseMatrix<int>::InnerIterator it(adjacency, curIdx); it; ++it) {
                        int nIdx = static_cast<int>(it.row());
                        if (visited[nIdx])
                            continue;
                        int nV = nIdx / nTimes;
                        int nT = nIdx % nTimes;
                        if (absMap(nV, nT) > h) {
                            visited[nIdx] = true;
                            queue.push(nIdx);
                        }
                    }
                }

                // Compute contribution: e^E * h^H * dh
                double extent = static_cast<double>(cluster.size());
                double contribution = std::pow(extent, E) * std::pow(h, H) * dh;

                for (int cIdx : cluster) {
                    int cv = cIdx / nTimes;
                    int ct = cIdx % nTimes;
                    if (isPositive) {
                        tfceMap(cv, ct) += contribution;
                    } else {
                        tfceMap(cv, ct) -= contribution;
                    }
                }
            }
        }
    };

    // Positive values
    MatrixXd posMap = statMap.cwiseMax(0.0);
    runTfce(posMap, true);

    // Negative values (use absolute values, then negate contributions)
    MatrixXd negMap = (-statMap).cwiseMax(0.0);
    runTfce(negMap, false);

    return tfceMap;
}
