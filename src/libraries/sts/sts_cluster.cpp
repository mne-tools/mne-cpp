//=============================================================================================================
/**
 * @file     sts_cluster.cpp
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
 * @brief    StatsCluster class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_cluster.h"
#include "sts_ttest.h"
#include "sts_ftest.h"

#include <QtConcurrent>
#include <QRandomGenerator>

#include <cmath>
#include <algorithm>
#include <queue>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

StatsClusterResult StatsCluster::permutationTest(
    const QVector<MatrixXd>& dataA,
    const QVector<MatrixXd>& dataB,
    const SparseMatrix<int>& adjacency,
    int nPermutations,
    double clusterAlpha,
    double pThreshold,
    StatsTailType tail)
{
    Q_UNUSED(pThreshold);

    const int nA = dataA.size();

    // Step 1: Compute the observed t-map
    MatrixXd tObs = computeTMap(dataA, dataB);

    // Step 2: Determine cluster-forming threshold from clusterAlpha and df
    int df = nA - 1;
    double threshold = inverseTCdf(clusterAlpha, df);

    // Step 3: Find observed clusters
    auto [clusterIds, clusterStats] = findClusters(tObs, threshold, adjacency, tail);

    // Step 4: Combine all data for permutation
    QVector<MatrixXd> allData;
    allData.reserve(nA + dataB.size());
    for (const auto& m : dataA) allData.append(m);
    for (const auto& m : dataB) allData.append(m);

    // Step 5: Build null distribution via permutations (using QtConcurrent)
    QVector<double> nullDist(nPermutations);

    // Perform permutations in parallel
    QVector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnce(allData, nA, adjacency, threshold, tail);
    };

    QFuture<double> future = QtConcurrent::mapped(permIndices, permuteFunc);
    future.waitForFinished();

    for (int i = 0; i < nPermutations; ++i) {
        nullDist[i] = future.resultAt(i);
    }

    // Sort null distribution for percentile computation
    std::sort(nullDist.begin(), nullDist.end());

    // Step 6: Compute cluster p-values
    QVector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < clusterStats.size(); ++c) {
        double obsStat = std::fabs(clusterStats[c]);
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count) / static_cast<double>(nPermutations);
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

MatrixXd StatsCluster::computeTMap(const QVector<MatrixXd>& dataA, const QVector<MatrixXd>& dataB)
{
    const int nSubjects = dataA.size();
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

QPair<MatrixXi, QVector<double>> StatsCluster::findClusters(
    const MatrixXd& tMap,
    double threshold,
    const SparseMatrix<int>& adjacency,
    StatsTailType tail)
{
    const int nChannels = static_cast<int>(tMap.rows());
    const int nTimes = static_cast<int>(tMap.cols());

    MatrixXi clusterIds = MatrixXi::Zero(nChannels, nTimes);
    QVector<double> clusterStats;
    int currentClusterId = 0;

    // Helper lambda: BFS on combined spatial+temporal adjacency for one polarity
    auto bfsClusters = [&](bool positive) {
        // Build a visited mask
        MatrixXi visited = MatrixXi::Zero(nChannels, nTimes);

        for (int ch = 0; ch < nChannels; ++ch) {
            for (int t = 0; t < nTimes; ++t) {
                double val = tMap(ch, t);
                bool suprathreshold = positive ? (val > threshold) : (val < -threshold);

                if (!suprathreshold || visited(ch, t)) continue;

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
                        if (nt < 0 || nt >= nTimes) continue;
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

                clusterStats.append(clusterSum);
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
    const QVector<MatrixXd>& allData,
    int nA,
    const SparseMatrix<int>& adjacency,
    double threshold,
    StatsTailType tail)
{
    const int nTotal = allData.size();

    // Generate a random permutation of indices
    std::vector<int> indices(nTotal);
    std::iota(indices.begin(), indices.end(), 0);

    // Use thread-safe random generator
    QRandomGenerator rng(QRandomGenerator::global()->generate());
    for (int i = nTotal - 1; i > 0; --i) {
        int j = static_cast<int>(rng.bounded(i + 1));
        std::swap(indices[i], indices[j]);
    }

    // Split into permuted groups
    QVector<MatrixXd> permA, permB;
    permA.reserve(nA);
    permB.reserve(nTotal - nA);
    for (int i = 0; i < nTotal; ++i) {
        if (i < nA) {
            permA.append(allData[indices[i]]);
        } else {
            permB.append(allData[indices[i]]);
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

MatrixXd StatsCluster::computeOneSampleTMap(const QVector<MatrixXd>& data)
{
    const int nSubjects = data.size();
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

MatrixXd StatsCluster::computeFMap(const QVector<QVector<MatrixXd>>& conditions)
{
    const int nConditions = conditions.size();
    const int nVertices = static_cast<int>(conditions[0][0].rows());
    const int nTimes = static_cast<int>(conditions[0][0].cols());

    // Count total subjects and build per-condition means
    int nTotal = 0;
    for (int c = 0; c < nConditions; ++c) {
        nTotal += conditions[c].size();
    }

    // Grand mean
    MatrixXd grandSum = MatrixXd::Zero(nVertices, nTimes);
    for (int c = 0; c < nConditions; ++c) {
        for (int s = 0; s < conditions[c].size(); ++s) {
            grandSum += conditions[c][s];
        }
    }
    MatrixXd grandMean = grandSum / static_cast<double>(nTotal);

    // SS between and SS within
    MatrixXd ssBetween = MatrixXd::Zero(nVertices, nTimes);
    MatrixXd ssWithin = MatrixXd::Zero(nVertices, nTimes);

    for (int c = 0; c < nConditions; ++c) {
        int nc = conditions[c].size();
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

QPair<MatrixXi, QVector<double>> StatsCluster::findClustersFlat(
    const MatrixXd& statMap,
    double threshold,
    const SparseMatrix<int>& adjacency,
    bool positiveOnly)
{
    const int nVertices = static_cast<int>(statMap.rows());
    const int nTimes = static_cast<int>(statMap.cols());
    const int nTotal = nVertices * nTimes;

    MatrixXi clusterIds = MatrixXi::Zero(nVertices, nTimes);
    QVector<double> clusterStats;
    int currentClusterId = 0;

    // BFS on the flat spatio-temporal adjacency
    std::vector<bool> visited(nTotal, false);

    for (int idx = 0; idx < nTotal; ++idx) {
        int v = idx / nTimes;
        int t = idx % nTimes;
        double val = statMap(v, t);

        bool suprathreshold = positiveOnly ? (val > threshold) : (val < -threshold);
        if (!suprathreshold || visited[idx]) continue;

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
                if (visited[nIdx]) continue;
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

        clusterStats.append(clusterSum);
    }

    return {clusterIds, clusterStats};
}

//=============================================================================================================

double StatsCluster::permuteOnceOneSample(
    const QVector<MatrixXd>& data,
    const SparseMatrix<int>& adjacency,
    double threshold,
    StatsTailType tail)
{
    const int nSubjects = data.size();

    // Generate random sign flips
    QRandomGenerator rng(QRandomGenerator::global()->generate());
    QVector<int> signs(nSubjects);
    for (int s = 0; s < nSubjects; ++s) {
        signs[s] = rng.bounded(2) == 0 ? 1 : -1;
    }

    // Apply sign flips
    QVector<MatrixXd> flipped(nSubjects);
    for (int s = 0; s < nSubjects; ++s) {
        flipped[s] = data[s] * static_cast<double>(signs[s]);
    }

    // Compute t-map and find clusters
    MatrixXd tMap = computeOneSampleTMap(flipped);

    double maxStat = 0.0;

    if (tail == StatsTailType::Both || tail == StatsTailType::Right) {
        auto [ids, stats] = findClustersFlat(tMap, threshold, adjacency, true);
        for (double s : stats) {
            if (std::fabs(s) > maxStat) maxStat = std::fabs(s);
        }
    }
    if (tail == StatsTailType::Both || tail == StatsTailType::Left) {
        auto [ids, stats] = findClustersFlat(tMap, threshold, adjacency, false);
        for (double s : stats) {
            if (std::fabs(s) > maxStat) maxStat = std::fabs(s);
        }
    }

    return maxStat;
}

//=============================================================================================================

double StatsCluster::permuteOnceFTest(
    const QVector<MatrixXd>& allData,
    const QVector<int>& groupSizes,
    const SparseMatrix<int>& adjacency,
    double threshold)
{
    const int nTotal = allData.size();

    // Random permutation of indices
    std::vector<int> indices(nTotal);
    std::iota(indices.begin(), indices.end(), 0);

    QRandomGenerator rng(QRandomGenerator::global()->generate());
    for (int i = nTotal - 1; i > 0; --i) {
        int j = static_cast<int>(rng.bounded(i + 1));
        std::swap(indices[i], indices[j]);
    }

    // Rebuild condition groups
    QVector<QVector<MatrixXd>> permConditions;
    int offset = 0;
    for (int c = 0; c < groupSizes.size(); ++c) {
        QVector<MatrixXd> group;
        group.reserve(groupSizes[c]);
        for (int s = 0; s < groupSizes[c]; ++s) {
            group.append(allData[indices[offset + s]]);
        }
        permConditions.append(group);
        offset += groupSizes[c];
    }

    // Compute F-map and find clusters (F is always positive)
    MatrixXd fMap = computeFMap(permConditions);
    auto [ids, stats] = findClustersFlat(fMap, threshold, adjacency, true);

    double maxStat = 0.0;
    for (double s : stats) {
        if (s > maxStat) maxStat = s;
    }
    return maxStat;
}

//=============================================================================================================

StatsClusterResult StatsCluster::oneSamplePermutationTest(
    const QVector<MatrixXd>& data,
    const SparseMatrix<int>& adjacency,
    double threshold,
    int nPermutations,
    StatsTailType tail)
{
    // Step 1: Compute observed one-sample t-map
    MatrixXd tObs = computeOneSampleTMap(data);

    // Step 2: Find observed clusters
    MatrixXi clusterIdsPos, clusterIdsNeg;
    QVector<double> clusterStats;

    if (tail == StatsTailType::Both || tail == StatsTailType::Right) {
        auto [ids, stats] = findClustersFlat(tObs, threshold, adjacency, true);
        clusterIdsPos = ids;
        clusterStats.append(stats);
    }
    if (tail == StatsTailType::Both || tail == StatsTailType::Left) {
        auto [ids, stats] = findClustersFlat(tObs, threshold, adjacency, false);
        clusterIdsNeg = ids;
        clusterStats.append(stats);
    }

    // Merge cluster ID maps
    MatrixXi clusterIds = MatrixXi::Zero(tObs.rows(), tObs.cols());
    if (clusterIdsPos.size() > 0) clusterIds += clusterIdsPos;
    if (clusterIdsNeg.size() > 0) clusterIds += clusterIdsNeg;

    // Step 3: Build null distribution via sign-flip permutations
    QVector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnceOneSample(data, adjacency, threshold, tail);
    };

    QFuture<double> future = QtConcurrent::mapped(permIndices, permuteFunc);
    future.waitForFinished();

    QVector<double> nullDist(nPermutations);
    for (int i = 0; i < nPermutations; ++i) {
        nullDist[i] = future.resultAt(i);
    }
    std::sort(nullDist.begin(), nullDist.end());

    // Step 4: Compute cluster p-values
    QVector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < clusterStats.size(); ++c) {
        double obsStat = std::fabs(clusterStats[c]);
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count) / static_cast<double>(nPermutations);
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
    const QVector<QVector<MatrixXd>>& conditions,
    const SparseMatrix<int>& adjacency,
    double threshold,
    int nPermutations)
{
    // Step 1: Compute observed F-map
    MatrixXd fObs = computeFMap(conditions);

    // Step 2: Find observed clusters (F is always positive, one-tailed)
    auto [clusterIds, clusterStats] = findClustersFlat(fObs, threshold, adjacency, true);

    // Step 3: Flatten all data and record group sizes
    QVector<MatrixXd> allData;
    QVector<int> groupSizes;
    for (int c = 0; c < conditions.size(); ++c) {
        groupSizes.append(conditions[c].size());
        for (int s = 0; s < conditions[c].size(); ++s) {
            allData.append(conditions[c][s]);
        }
    }

    // Step 4: Build null distribution via label-shuffle permutations
    QVector<int> permIndices(nPermutations);
    std::iota(permIndices.begin(), permIndices.end(), 0);

    std::function<double(int)> permuteFunc = [&](int) -> double {
        return permuteOnceFTest(allData, groupSizes, adjacency, threshold);
    };

    QFuture<double> future = QtConcurrent::mapped(permIndices, permuteFunc);
    future.waitForFinished();

    QVector<double> nullDist(nPermutations);
    for (int i = 0; i < nPermutations; ++i) {
        nullDist[i] = future.resultAt(i);
    }
    std::sort(nullDist.begin(), nullDist.end());

    // Step 5: Compute cluster p-values
    QVector<double> clusterPvals(clusterStats.size());
    for (int c = 0; c < clusterStats.size(); ++c) {
        double obsStat = clusterStats[c];
        int count = 0;
        for (int p = 0; p < nPermutations; ++p) {
            if (nullDist[p] >= obsStat) {
                count++;
            }
        }
        clusterPvals[c] = static_cast<double>(count) / static_cast<double>(nPermutations);
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

    // Helper: run TFCE on one polarity
    auto runTfce = [&](const MatrixXd& absMap, bool isPositive) {
        double maxVal = absMap.maxCoeff();
        if (maxVal <= 0.0) return;

        double dh = maxVal / static_cast<double>(nSteps);

        for (int step = 1; step <= nSteps; ++step) {
            double h = dh * static_cast<double>(step);

            // Find connected components above threshold h
            std::vector<bool> visited(nTotal, false);

            for (int idx = 0; idx < nTotal; ++idx) {
                int v = idx / nTimes;
                int t = idx % nTimes;

                if (absMap(v, t) < h || visited[idx]) continue;

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
                        if (visited[nIdx]) continue;
                        int nV = nIdx / nTimes;
                        int nT = nIdx % nTimes;
                        if (absMap(nV, nT) >= h) {
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
