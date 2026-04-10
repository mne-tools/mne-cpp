//=============================================================================================================
/**
 * @file     sts_adjacency.cpp
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
 * @brief    StatsAdjacency class definition.
 *
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
            if (d < nnDist[i]) nnDist[i] = d;
            if (d < nnDist[j]) nnDist[j] = d;
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
