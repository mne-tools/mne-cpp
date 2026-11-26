//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     geometryinfo.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     May 2017
 * @brief    Multi-source Dijkstra (SCDC), sensor-to-mesh projection and sparse-matrix fusion implementations.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryinfo.h"

#include <mne/base/log.h>
#include <mne/base/thread_pool.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::shared_ptr<MatrixXd> GeometryInfo::scdc(const MatrixX3f& matVertices,
                                             const std::vector<VectorXi>& vecNeighborVertices,
                                             VectorXi& vecVertSubset,
                                             double dCancelDist)
{
    // create matrix and check for empty subset:
    std::int32_t iCols = static_cast<std::int32_t>(vecVertSubset.size());
    if (vecVertSubset.size() == 0) {
        mne::Log::warning("SCDC received empty subset, calculating full distance table, make sure you have enough memory !");
        vecVertSubset = VectorXi::LinSpaced(matVertices.rows(), 0, static_cast<int>(matVertices.rows()) - 1);
        iCols = static_cast<std::int32_t>(matVertices.rows());
    }

    std::shared_ptr<MatrixXd> returnMat = std::make_shared<MatrixXd>(matVertices.rows(), iCols);

    // distribute calculation on cores
    int iCores = static_cast<int>(std::thread::hardware_concurrency());
    if (iCores <= 0) {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    // Cap parallel threads to avoid exhausting the Emscripten pthread pool.
    iCores = std::min(iCores, 2);
#endif

    std::int32_t iSubArraySize = static_cast<int>(static_cast<double>(vecVertSubset.size()) / static_cast<double>(iCores));
    ThreadPool::global().parallel_for(0, static_cast<std::size_t>(iCores), [&](std::size_t i) {
        const std::int32_t iBegin = static_cast<std::int32_t>(i) * iSubArraySize;
        const std::int32_t iEnd = (static_cast<int>(i) == iCores - 1) ? static_cast<std::int32_t>(vecVertSubset.size())
                                                                      : iBegin + iSubArraySize;
        iterativeDijkstra(returnMat, matVertices, vecNeighborVertices, vecVertSubset, iBegin, iEnd, dCancelDist);
    });

    return returnMat;
}

//=============================================================================================================

std::shared_ptr<SparseMatrix<float>> GeometryInfo::scdcInterpolationMat(
    const MatrixX3f& matVertices,
    const std::vector<VectorXi>& vecNeighborVertices,
    const VectorXi& vecVertSubset,
    double (*interpolationFunction)(double),
    double dCancelDist,
    std::function<void(int, int)> progressCallback,
    const std::atomic<bool>* cancelledFlag)
{
    const std::int32_t nVerts = static_cast<std::int32_t>(matVertices.rows());
    const std::int32_t nSources = static_cast<std::int32_t>(vecVertSubset.size());
    const std::int32_t nAdj = static_cast<std::int32_t>(vecNeighborVertices.size());

    // Build a set of source vertex indices for O(1) lookup
    // (vertices that ARE source vertices get weight=1 on themselves)
    std::unordered_map<std::int32_t, std::int32_t> vertexToSourceIdx;
    for (std::int32_t s = 0; s < nSources; ++s) {
        vertexToSourceIdx.insert_or_assign(vecVertSubset[s], s);
    }

    // For each source vertex, run Dijkstra from that source.
    // Collect distances to all reachable mesh vertices within cancelDist.
    // Store as: perVertex[meshVertexIdx] -> list of (sourceIdx, geodesicDist)
    // We process source-by-source since each Dijkstra only touches a small neighborhood.

    // Thread-local storage: each chunk produces triplets
    struct VertexWeight
    {
        std::int32_t sourceIdx;
        float dist;
    };
    struct WeightTriple
    {
        std::int32_t vertex;
        std::int32_t sourceIdx;
        float dist;
    };

    // Parallelize the per-source Dijkstra loop. Sources are independent;
    // each worker keeps its own thread-local Dijkstra scratch buffers and
    // emits a flat list of (vertex, source, dist) triples. A final
    // single-threaded merge pass groups them per mesh vertex. This recovers
    // the multi-core scaling that the legacy scdc() path used to provide.
    int iCores = static_cast<int>(std::thread::hardware_concurrency());
    if (iCores <= 0) {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    iCores = std::min(iCores, 2);
#endif
    iCores = std::min(iCores, std::max(1, static_cast<int>(nSources)));

    std::atomic<bool> internalCancel(false);
    const std::atomic<bool>* cancelView = cancelledFlag ? cancelledFlag : &internalCancel;

    std::atomic<std::int32_t> progressCounter(0);

    std::vector<std::vector<WeightTriple>> perThreadTriples(iCores);

    auto worker = [&](int threadIdx, std::int32_t sBegin, std::int32_t sEnd) {
        std::vector<WeightTriple>& out = perThreadTriples[threadIdx];
        // Heuristic reserve: average ~32 reachable vertices per source.
        out.reserve(static_cast<size_t>(sEnd - sBegin) * 32);

        // Thread-local Dijkstra scratch.
        std::vector<float> vecMinDists(nAdj, FLOAT_INFINITY);
        // Track which entries we touched so we only have to reset those
        // between sources (avoids O(nVerts) fill() per source).
        std::vector<std::int32_t> touched;
        touched.reserve(1024);

        // Lazy-deletion priority queue: push (dist, vertex); skip stale entries
        // when popping. ~3-5x faster than std::set for Dijkstra.
        using QueueEntry = std::pair<float, std::int32_t>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> vertexQ;

        const float fCancelDist = static_cast<float>(dCancelDist);

        for (std::int32_t s = sBegin; s < sEnd; ++s) {
            // Cooperative cancellation; only check every 16 sources to keep
            // the atomic load out of the hot inner loop.
            if (((s - sBegin) & 0xF) == 0 && cancelView->load(std::memory_order_relaxed)) {
                return;
            }

            const std::int32_t iRoot = vecVertSubset[s];

            // Reset only previously-touched slots.
            for (std::int32_t idx : touched)
                vecMinDists[idx] = FLOAT_INFINITY;
            touched.clear();
            while (!vertexQ.empty())
                vertexQ.pop();

            vecMinDists[iRoot] = 0.0f;
            touched.push_back(iRoot);
            vertexQ.emplace(0.0f, iRoot);

            while (!vertexQ.empty()) {
                const float fDist = vertexQ.top().first;
                const std::int32_t u = vertexQ.top().second;
                vertexQ.pop();

                // Stale entry from lazy deletion?
                if (fDist > vecMinDists[u])
                    continue;
                if (fDist > fCancelDist)
                    continue;

                const VectorXi& vecNeighbours = vecNeighborVertices[u];
                const float ux = matVertices(u, 0);
                const float uy = matVertices(u, 1);
                const float uz = matVertices(u, 2);

                for (Eigen::Index ne = 0; ne < vecNeighbours.size(); ++ne) {
                    const std::int32_t v = vecNeighbours[ne];
                    const float dx = ux - matVertices(v, 0);
                    const float dy = uy - matVertices(v, 1);
                    const float dz = uz - matVertices(v, 2);
                    const float fDistWithU = fDist + std::sqrt(dx * dx + dy * dy + dz * dz);

                    if (fDistWithU < vecMinDists[v]) {
                        if (vecMinDists[v] == FLOAT_INFINITY)
                            touched.push_back(v);
                        vecMinDists[v] = fDistWithU;
                        if (fDistWithU <= fCancelDist)
                            vertexQ.emplace(fDistWithU, v);
                    }
                }
            }

            // Collect reachable vertices for this source.
            for (std::int32_t idx : touched) {
                const float d = vecMinDists[idx];
                if (d < fCancelDist) {
                    out.push_back({idx, s, d});
                }
            }

            if (progressCallback) {
                const std::int32_t done = progressCounter.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((done % 100) == 0) {
                    progressCallback(done, nSources);
                }
            }
        }
    };

    if (iCores <= 1) {
        worker(0, 0, nSources);
    } else {
        const std::int32_t chunk = nSources / iCores;
        ThreadPool::global().parallel_for(0, static_cast<std::size_t>(iCores), [&](std::size_t i) {
            const int t = static_cast<int>(i);
            const std::int32_t sBegin = t * chunk;
            const std::int32_t sEnd = (t == iCores - 1) ? nSources : (sBegin + chunk);
            worker(t, sBegin, sEnd);
        });
    }

    if (cancelView->load(std::memory_order_relaxed))
        return std::shared_ptr<SparseMatrix<float>>();

    // Merge thread-local triples into the per-vertex list.
    std::vector<std::vector<VertexWeight>> perVertexWeights(nVerts);
    {
        // Pre-size each per-vertex bucket to avoid repeated reallocations.
        std::vector<size_t> counts(nVerts, 0);
        for (const auto& chunk : perThreadTriples) {
            for (const auto& t : chunk)
                ++counts[t.vertex];
        }
        for (std::int32_t v = 0; v < nVerts; ++v) {
            if (counts[v])
                perVertexWeights[v].reserve(counts[v]);
        }
        for (const auto& chunk : perThreadTriples) {
            for (const auto& t : chunk) {
                perVertexWeights[t.vertex].push_back({t.sourceIdx, t.dist});
            }
        }
    }

    // Build the sparse interpolation matrix from the collected distances

    std::vector<Eigen::Triplet<float>> vecTriplets;
    // Estimate ~10-20 sources per vertex on average
    vecTriplets.reserve(nVerts * 10);

    for (std::int32_t r = 0; r < nVerts; ++r) {
        if (const auto source = vertexToSourceIdx.find(r); source != vertexToSourceIdx.end()) {
            // Source vertex: identity mapping (weight = 1)
            vecTriplets.push_back(Eigen::Triplet<float>(r, source->second, 1.0f));
        } else {
            const auto& weights = perVertexWeights[r];
            if (weights.empty())
                continue;

            // Apply interpolation function and normalize weights
            float dWeightsSum = 0.0f;
            std::vector<std::pair<std::int32_t, float>> vecBelowThresh;
            vecBelowThresh.reserve(weights.size());

            for (const auto& w : weights) {
                const float dValueWeight = std::fabs(1.0f / static_cast<float>(interpolationFunction(w.dist)));
                dWeightsSum += dValueWeight;
                vecBelowThresh.emplace_back(w.sourceIdx, dValueWeight);
            }

            for (const auto& qp : vecBelowThresh) {
                vecTriplets.push_back(Eigen::Triplet<float>(r, qp.first, qp.second / dWeightsSum));
            }
        }
    }

    auto interpMat = std::make_shared<SparseMatrix<float>>(nVerts, nSources);
    interpMat->setFromTriplets(vecTriplets.begin(), vecTriplets.end());

    return interpMat;
}

//=============================================================================================================

VectorXi GeometryInfo::projectSensors(const MatrixX3f& matVertices,
                                      const MatrixX3f& matSensorPositions)
{
    const std::int32_t iNumSensors = static_cast<std::int32_t>(matSensorPositions.rows());

    std::int32_t iCores = static_cast<int>(std::thread::hardware_concurrency());
    if (iCores <= 0) {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    iCores = std::min(iCores, 2);
#endif

    const std::int32_t iSubArraySize = static_cast<int>(static_cast<double>(iNumSensors) / static_cast<double>(iCores));

    if (iSubArraySize <= 1) {
        return nearestNeighbor(matVertices, matSensorPositions, 0, iNumSensors);
    }

    std::vector<VectorXi> vecPartials(static_cast<std::size_t>(iCores));
    ThreadPool::global().parallel_for(0, vecPartials.size(), [&](std::size_t i) {
        const std::int32_t iBeginOffset = static_cast<std::int32_t>(i) * iSubArraySize;
        const std::int32_t iEndOffset = (static_cast<std::int32_t>(i) == iCores - 1) ? iNumSensors : iBeginOffset + iSubArraySize;
        vecPartials[i] = nearestNeighbor(matVertices, matSensorPositions, iBeginOffset, iEndOffset);
    });

    // concatenate partial results
    VectorXi vecOutputArray(iNumSensors);
    std::int32_t iOffset = 0;
    for (const VectorXi& partial : vecPartials) {
        vecOutputArray.segment(iOffset, partial.size()) = partial;
        iOffset += static_cast<std::int32_t>(partial.size());
    }

    return vecOutputArray;
}

//=============================================================================================================

VectorXi GeometryInfo::nearestNeighbor(const MatrixX3f& matVertices,
                                       const MatrixX3f& matSensorPositions,
                                       std::int32_t iBegin,
                                       std::int32_t iEnd)
{
    VectorXi vecMappedSensors(iEnd - iBegin);

    for (std::int32_t s = iBegin; s < iEnd; ++s) {
        std::int32_t iChampionId = 0;
        double iChampDist = std::numeric_limits<double>::max();
        for (std::int32_t i = 0; i < matVertices.rows(); ++i) {
            double dDist = sqrt(squared(matVertices(i, 0) - matSensorPositions(s, 0)) + squared(matVertices(i, 1) - matSensorPositions(s, 1)) + squared(matVertices(i, 2) - matSensorPositions(s, 2)));
            if (dDist < iChampDist) {
                iChampionId = i;
                iChampDist = dDist;
            }
        }
        vecMappedSensors[s - iBegin] = iChampionId;
    }

    return vecMappedSensors;
}

//=============================================================================================================

void GeometryInfo::iterativeDijkstra(const std::shared_ptr<MatrixXd>& matOutputDistMatrix,
                                     const MatrixX3f& matVertices,
                                     const std::vector<VectorXi>& vecNeighborVertices,
                                     const VectorXi& vecVertSubset,
                                     std::int32_t iBegin,
                                     std::int32_t iEnd,
                                     double dCancelDistance)
{
    const std::vector<VectorXi>& vecAdjacency = vecNeighborVertices;
    std::int32_t n = static_cast<std::int32_t>(vecAdjacency.size());
    std::vector<double> vecMinDists(n);
    std::set<std::pair<double, std::int32_t>> vertexQ;
    const double INF = FLOAT_INFINITY;

    for (std::int32_t i = iBegin; i < iEnd; ++i) {
        if ((i - iBegin) > 0 && (i - iBegin) % 100 == 0) {
            mne::Log::debug("GeometryInfo::iterativeDijkstra progress: {}/{} (Thread range: {}-{})", i - iBegin, iEnd - iBegin, iBegin, iEnd);
        }

        std::int32_t iRoot = vecVertSubset[i];
        vertexQ.clear();
        std::ranges::fill(vecMinDists, INF);
        vecMinDists[iRoot] = 0.0;
        vertexQ.insert(std::make_pair(vecMinDists[iRoot], iRoot));

        while (vertexQ.empty() == false) {
            const double dDist = vertexQ.begin()->first;
            const std::int32_t u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());

            if (dDist <= dCancelDistance) {
                const VectorXi& vecNeighbours = vecAdjacency[u];

                for (Eigen::Index ne = 0; ne < vecNeighbours.size(); ++ne) {
                    std::int32_t v = vecNeighbours[ne];

                    const double dDistX = matVertices(u, 0) - matVertices(v, 0);
                    const double dDistY = matVertices(u, 1) - matVertices(v, 1);
                    const double dDistZ = matVertices(u, 2) - matVertices(v, 2);
                    const double dDistWithU = dDist + sqrt(dDistX * dDistX + dDistY * dDistY + dDistZ * dDistZ);

                    if (dDistWithU < vecMinDists[v]) {
                        vertexQ.erase(std::make_pair(vecMinDists[v], v));
                        vecMinDists[v] = dDistWithU;
                        vertexQ.insert(std::make_pair(vecMinDists[v], v));
                    }
                }
            }
        }

        for (std::int32_t m = 0; m < static_cast<std::int32_t>(vecMinDists.size()); ++m) {
            matOutputDistMatrix->coeffRef(m, i) = vecMinDists[m];
        }
    }
}

//=============================================================================================================

VectorXi GeometryInfo::filterBadChannels(const std::shared_ptr<Eigen::MatrixXd>& matDistanceTable,
                                         const mne::FiffInfo& fiffInfo,
                                         std::int32_t iSensorType)
{
    std::vector<int> vecBadColumns;
    std::vector<const FiffChInfo*> vecSensors;
    for (const FiffChInfo& s : fiffInfo.chs) {
        if (s.kind == iSensorType && (s.unit == FIFF_UNIT_T || s.unit == FIFF_UNIT_V)) {
            vecSensors.push_back(&s);
        }
    }

    for (const std::string& b : fiffInfo.bads) {
        for (int col = 0; col < static_cast<int>(vecSensors.size()); ++col) {
            if (vecSensors[col]->ch_name == b) {
                vecBadColumns.push_back(col);
                for (int row = 0; row < matDistanceTable->rows(); ++row) {
                    matDistanceTable->coeffRef(row, col) = FLOAT_INFINITY;
                }
                break;
            }
        }
    }

    return Eigen::Map<VectorXi>(vecBadColumns.data(), static_cast<Eigen::Index>(vecBadColumns.size()));
}
