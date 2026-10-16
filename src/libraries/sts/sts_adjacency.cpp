//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_adjacency.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the sensor- and source-space adjacency builders declared in sts_adjacency.h.
 *
 * The channel-position constructor extracts the @c r0 entries from
 * @ref FIFFLIB::FiffInfo, computes the per-channel nearest-neighbour
 * distance and adds an edge whenever the pairwise Euclidean distance is
 * within three times the median nearest-neighbour distance - the same
 * heuristic MNE-Python uses for sensor-space cluster tests.
 *
 * The source-space constructors walk the triangle list once, marking the
 * three vertex pairs of each triangle as neighbours; the spatio-temporal
 * variant additionally links every vertex to itself at @c t-1 and @c t+1
 * so cluster growth can span both cortex and time without the caller
 * having to build a Kronecker product by hand.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_adjacency.h"

#include <fiff/fiff_ch_info.h>

#include <algorithm>
#include <vector>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================

SparseMatrix<int> StatsAdjacency::chain(int n)
{
    std::vector<Triplet<int>> triplets;
    triplets.reserve(2 * static_cast<std::size_t>(std::max(n - 1, 0)));
    for (int i = 0; i + 1 < n; ++i) {
        triplets.emplace_back(i, i + 1, 1);
        triplets.emplace_back(i + 1, i, 1);
    }
    SparseMatrix<int> adjacency(std::max(n, 0), std::max(n, 0));
    adjacency.setFromTriplets(triplets.begin(), triplets.end());
    return adjacency;
}

//=============================================================================================================

SparseMatrix<int> StatsAdjacency::combine(const QList<SparseMatrix<int>>& dims)
{
    // Adapted from mne.stats.combine_adjacency (MNE-Python, BSD-3-Clause).
    Index total = 1;
    for (const SparseMatrix<int>& dim : dims)
        total *= dim.rows();
    std::vector<Triplet<int>> triplets;
    triplets.reserve(static_cast<std::size_t>(total));
    for (Index k = 0; k < total; ++k)
        triplets.emplace_back(static_cast<int>(k), static_cast<int>(k), 1);

    // stride of dimension d = product of the sizes after it
    Index stride = total;
    for (const SparseMatrix<int>& dim : dims) {
        const Index n = dim.rows();
        stride /= n;
        const Index outer = total / (n * stride);
        for (int c = 0; c < dim.outerSize(); ++c) {
            for (SparseMatrix<int>::InnerIterator it(dim, c); it; ++it) {
                if (it.row() == it.col() || it.value() == 0)
                    continue;
                for (Index o = 0; o < outer; ++o) {
                    for (Index s = 0; s < stride; ++s) {
                        const Index base = o * n * stride + s;
                        triplets.emplace_back(static_cast<int>(base + it.row() * stride), static_cast<int>(base + it.col() * stride), 1);
                    }
                }
            }
        }
    }
    SparseMatrix<int> combined(total, total);
    combined.setFromTriplets(triplets.begin(), triplets.end(), [](int a, int) { return a; });
    return combined;
}

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

SparseMatrix<int> StatsAdjacency::fromChannelPositions(const FiffInfo& info, const QStringList& picks)
{
    // Gather channel indices and 3D positions
    std::vector<int> chIdx;
    if (picks.isEmpty()) {
        for (int i = 0; i < info.chs.size(); ++i) {
            chIdx.push_back(i);
        }
    } else {
        for (int i = 0; i < info.chs.size(); ++i) {
            if (picks.contains(info.chs[i].ch_name)) {
                chIdx.push_back(i);
            }
        }
    }

    const int nCh = static_cast<int>(chIdx.size());
    if (nCh == 0) {
        return SparseMatrix<int>(0, 0);
    }

    // Extract 3D positions
    MatrixXd pos(nCh, 3);
    for (int i = 0; i < nCh; ++i) {
        const Vector3f& r0 = info.chs[chIdx[i]].chpos.r0;
        pos(i, 0) = static_cast<double>(r0(0));
        pos(i, 1) = static_cast<double>(r0(1));
        pos(i, 2) = static_cast<double>(r0(2));
    }

    // Compute pairwise distances and find nearest-neighbor distance for each channel
    std::vector<double> nnDist(nCh, std::numeric_limits<double>::max());
    MatrixXd distMat(nCh, nCh);
    for (int i = 0; i < nCh; ++i) {
        distMat(i, i) = 0.0;
        for (int j = i + 1; j < nCh; ++j) {
            double d = (pos.row(i) - pos.row(j)).norm();
            distMat(i, j) = d;
            distMat(j, i) = d;
            if (d < nnDist[i])
                nnDist[i] = d;
            if (d < nnDist[j])
                nnDist[j] = d;
        }
    }

    // Median nearest-neighbor distance
    std::vector<double> sortedNN = nnDist;
    std::sort(sortedNN.begin(), sortedNN.end());
    double medianNN;
    if (nCh % 2 == 0) {
        medianNN = (sortedNN[nCh / 2 - 1] + sortedNN[nCh / 2]) / 2.0;
    } else {
        medianNN = sortedNN[nCh / 2];
    }

    double threshold = 3.0 * medianNN;

    // Build adjacency
    std::vector<Triplet<int>> triplets;
    for (int i = 0; i < nCh; ++i) {
        for (int j = i + 1; j < nCh; ++j) {
            if (distMat(i, j) <= threshold) {
                triplets.emplace_back(i, j, 1);
                triplets.emplace_back(j, i, 1);
            }
        }
    }

    SparseMatrix<int> adj(nCh, nCh);
    adj.setFromTriplets(triplets.begin(), triplets.end());
    return adj;
}

//=============================================================================================================

SparseMatrix<int> StatsAdjacency::fromSourceSpace(const MatrixX3i& tris, int nVertices)
{
    std::vector<Triplet<int>> triplets;

    for (int t = 0; t < tris.rows(); ++t) {
        int v0 = tris(t, 0);
        int v1 = tris(t, 1);
        int v2 = tris(t, 2);

        // Mark all 3 pairs as adjacent (symmetric)
        triplets.emplace_back(v0, v1, 1);
        triplets.emplace_back(v1, v0, 1);
        triplets.emplace_back(v0, v2, 1);
        triplets.emplace_back(v2, v0, 1);
        triplets.emplace_back(v1, v2, 1);
        triplets.emplace_back(v2, v1, 1);
    }

    SparseMatrix<int> adj(nVertices, nVertices);
    adj.setFromTriplets(triplets.begin(), triplets.end());
    return adj;
}

//=============================================================================================================

SparseMatrix<int> StatsAdjacency::fromSourceSpaceTemporal(
    const MatrixX3i& tris, int nVertices, int nTimes)
{
    // Build spatial adjacency first
    SparseMatrix<int> spatialAdj = fromSourceSpace(tris, nVertices);

    const int nTotal = nVertices * nTimes;
    std::vector<Triplet<int>> triplets;

    // Reserve approximate capacity: spatial edges * nTimes + temporal edges
    triplets.reserve(static_cast<size_t>(spatialAdj.nonZeros()) * nTimes + static_cast<size_t>(nVertices) * (nTimes - 1) * 2);

    // Spatial neighbors repeated for each time point
    // Linear index: v * nTimes + t
    for (int t = 0; t < nTimes; ++t) {
        for (int k = 0; k < spatialAdj.outerSize(); ++k) {
            for (SparseMatrix<int>::InnerIterator it(spatialAdj, k); it; ++it) {
                int row = static_cast<int>(it.row()) * nTimes + t;
                int col = static_cast<int>(it.col()) * nTimes + t;
                triplets.emplace_back(row, col, 1);
            }
        }
    }

    // Temporal neighbors: vertex v at time t adjacent to v at t-1 and t+1
    for (int v = 0; v < nVertices; ++v) {
        for (int t = 0; t < nTimes - 1; ++t) {
            int idx0 = v * nTimes + t;
            int idx1 = v * nTimes + t + 1;
            triplets.emplace_back(idx0, idx1, 1);
            triplets.emplace_back(idx1, idx0, 1);
        }
    }

    SparseMatrix<int> adj(nTotal, nTotal);
    adj.setFromTriplets(triplets.begin(), triplets.end());
    return adj;
}
