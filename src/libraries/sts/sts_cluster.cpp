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
