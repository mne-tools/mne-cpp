//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     geometryinfo.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Multi-source Dijkstra (SCDC), sensor-to-mesh projection and sparse-matrix fusion implementations.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryinfo.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <cmath>
#include <fstream>
#include <queue>
#include <set>
#include <utility>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<MatrixXd> GeometryInfo::scdc(const MatrixX3f &matVertices,
                                            const std::vector<VectorXi> &vecNeighborVertices,
                                            VectorXi &vecVertSubset,
                                            double dCancelDist)
{
    // create matrix and check for empty subset:
    qint32 iCols = static_cast<qint32>(vecVertSubset.size());
    if(vecVertSubset.size() == 0) {
        qDebug() << "[WARNING] SCDC received empty subset, calculating full distance table, make sure you have enough memory !";
        vecVertSubset = VectorXi::LinSpaced(matVertices.rows(), 0, static_cast<int>(matVertices.rows()) - 1);
        iCols = static_cast<qint32>(matVertices.rows());
    }

    QSharedPointer<MatrixXd> returnMat = QSharedPointer<MatrixXd>::create(matVertices.rows(), iCols);

    // distribute calculation on cores
    int iCores = QThread::idealThreadCount();
    if (iCores <= 0) {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    // Cap parallel threads to avoid exhausting the Emscripten pthread pool.
    iCores = qMin(iCores, 2);
#endif

    qint32 iSubArraySize = int(double(vecVertSubset.size()) / double(iCores));
    QVector<QFuture<void> > vecThreads(iCores);
    qint32 iBegin = 0;
    qint32 iEnd = iSubArraySize;

    for (int i = 0; i < vecThreads.size(); ++i) {
        if(i == vecThreads.size()-1)
        {
            vecThreads[i] = QtConcurrent::run(std::bind(iterativeDijkstra,
                                                        returnMat,
                                                        std::cref(matVertices),
                                                        std::cref(vecNeighborVertices),
                                                        std::cref(vecVertSubset),
                                                        iBegin,
                                                        static_cast<qint32>(vecVertSubset.size()),
                                                        dCancelDist));
            break;
        }
        else
        {
            vecThreads[i] = QtConcurrent::run(std::bind(iterativeDijkstra,
                                                        returnMat,
                                                        std::cref(matVertices),
                                                        std::cref(vecNeighborVertices),
                                                        std::cref(vecVertSubset),
                                                        iBegin,
                                                        iEnd,
                                                        dCancelDist));
            iBegin += iSubArraySize;
            iEnd += iSubArraySize;
        }
    }

    for (QFuture<void>& f : vecThreads) {
        f.waitForFinished();
    }

    return returnMat;
}

//=============================================================================================================

QSharedPointer<SparseMatrix<float>> GeometryInfo::scdcInterpolationMat(
    const MatrixX3f &matVertices,
    const std::vector<VectorXi> &vecNeighborVertices,
    const VectorXi &vecVertSubset,
    double (*interpolationFunction)(double),
    double dCancelDist,
    std::function<void(int, int)> progressCallback,
    const std::atomic<bool> *cancelledFlag)
{
    const qint32 nVerts = static_cast<qint32>(matVertices.rows());
    const qint32 nSources = static_cast<qint32>(vecVertSubset.size());
    const qint32 nAdj = static_cast<qint32>(vecNeighborVertices.size());

    // Build a set of source vertex indices for O(1) lookup
    // (vertices that ARE source vertices get weight=1 on themselves)
    QSet<qint32> sourceVertexSet;
    QHash<qint32, qint32> vertexToSourceIdx;
    for (qint32 s = 0; s < nSources; ++s) {
        sourceVertexSet.insert(vecVertSubset[s]);
        vertexToSourceIdx.insert(vecVertSubset[s], s);
    }

    // For each source vertex, run Dijkstra from that source.
    // Collect distances to all reachable mesh vertices within cancelDist.
    // Store as: perVertex[meshVertexIdx] -> list of (sourceIdx, geodesicDist)
    // We process source-by-source since each Dijkstra only touches a small neighborhood.

    // Thread-local storage: each chunk produces triplets
    struct VertexWeight {
        qint32 sourceIdx;
        float dist;
    };
    struct WeightTriple {
        qint32 vertex;
        qint32 sourceIdx;
        float  dist;
    };

    // Parallelize the per-source Dijkstra loop. Sources are independent;
    // each worker keeps its own thread-local Dijkstra scratch buffers and
    // emits a flat list of (vertex, source, dist) triples. A final
    // single-threaded merge pass groups them per mesh vertex. This recovers
    // the multi-core scaling that the legacy scdc() path used to provide.
    int iCores = QThread::idealThreadCount();
    if (iCores <= 0) {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    iCores = qMin(iCores, 2);
#endif
    iCores = qMin(iCores, qMax(1, static_cast<int>(nSources)));

    std::atomic<bool> internalCancel(false);
    const std::atomic<bool> *cancelView = cancelledFlag ? cancelledFlag : &internalCancel;

    std::atomic<qint32> progressCounter(0);

    std::vector<std::vector<WeightTriple>> perThreadTriples(iCores);

    auto worker = [&](int threadIdx, qint32 sBegin, qint32 sEnd) {
        std::vector<WeightTriple> &out = perThreadTriples[threadIdx];
        // Heuristic reserve: average ~32 reachable vertices per source.
        out.reserve(static_cast<size_t>(sEnd - sBegin) * 32);

        // Thread-local Dijkstra scratch.
        std::vector<float> vecMinDists(nAdj, FLOAT_INFINITY);
        // Track which entries we touched so we only have to reset those
        // between sources (avoids O(nVerts) fill() per source).
        std::vector<qint32> touched;
        touched.reserve(1024);

        // Lazy-deletion priority queue: push (dist, vertex); skip stale entries
        // when popping. ~3-5x faster than std::set for Dijkstra.
        using QueueEntry = std::pair<float, qint32>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> vertexQ;

        const float fCancelDist = static_cast<float>(dCancelDist);

        for (qint32 s = sBegin; s < sEnd; ++s) {
            // Cooperative cancellation; only check every 16 sources to keep
            // the atomic load out of the hot inner loop.
            if (((s - sBegin) & 0xF) == 0
                && cancelView->load(std::memory_order_relaxed)) {
                return;
            }

            const qint32 iRoot = vecVertSubset[s];

            // Reset only previously-touched slots.
            for (qint32 idx : touched) vecMinDists[idx] = FLOAT_INFINITY;
            touched.clear();
            while (!vertexQ.empty()) vertexQ.pop();

            vecMinDists[iRoot] = 0.0f;
            touched.push_back(iRoot);
            vertexQ.emplace(0.0f, iRoot);

            while (!vertexQ.empty()) {
                const float fDist = vertexQ.top().first;
                const qint32 u   = vertexQ.top().second;
                vertexQ.pop();

                // Stale entry from lazy deletion?
                if (fDist > vecMinDists[u]) continue;
                if (fDist > fCancelDist)    continue;

                const VectorXi &vecNeighbours = vecNeighborVertices[u];
                const float ux = matVertices(u, 0);
                const float uy = matVertices(u, 1);
                const float uz = matVertices(u, 2);

                for (Eigen::Index ne = 0; ne < vecNeighbours.size(); ++ne) {
                    const qint32 v = vecNeighbours[ne];
                    const float dx = ux - matVertices(v, 0);
                    const float dy = uy - matVertices(v, 1);
                    const float dz = uz - matVertices(v, 2);
                    const float fDistWithU = fDist + std::sqrt(dx*dx + dy*dy + dz*dz);

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
            for (qint32 idx : touched) {
                const float d = vecMinDists[idx];
                if (d < fCancelDist) {
                    out.push_back({idx, s, d});
                }
            }

            if (progressCallback) {
                const qint32 done = progressCounter.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((done % 100) == 0) {
                    progressCallback(done, nSources);
                }
            }
        }
    };

    if (iCores <= 1) {
        worker(0, 0, nSources);
    } else {
        QVector<QFuture<void>> vecThreads;
        vecThreads.reserve(iCores);
        const qint32 chunk = nSources / iCores;
        qint32 sBegin = 0;
        for (int t = 0; t < iCores; ++t) {
            const qint32 sEnd = (t == iCores - 1) ? nSources : (sBegin + chunk);
            vecThreads.push_back(QtConcurrent::run(worker, t, sBegin, sEnd));
            sBegin = sEnd;
        }
        for (QFuture<void> &f : vecThreads) f.waitForFinished();
    }

    if (cancelView->load(std::memory_order_relaxed))
        return QSharedPointer<SparseMatrix<float>>();

    // Merge thread-local triples into the per-vertex list.
    std::vector<std::vector<VertexWeight>> perVertexWeights(nVerts);
    {
        // Pre-size each per-vertex bucket to avoid repeated reallocations.
        std::vector<size_t> counts(nVerts, 0);
        for (const auto &chunk : perThreadTriples) {
            for (const auto &t : chunk) ++counts[t.vertex];
        }
        for (qint32 v = 0; v < nVerts; ++v) {
            if (counts[v]) perVertexWeights[v].reserve(counts[v]);
        }
        for (const auto &chunk : perThreadTriples) {
            for (const auto &t : chunk) {
                perVertexWeights[t.vertex].push_back({t.sourceIdx, t.dist});
            }
        }
    }

    // Build the sparse interpolation matrix from the collected distances

    QVector<Eigen::Triplet<float>> vecTriplets;
    // Estimate ~10-20 sources per vertex on average
    vecTriplets.reserve(nVerts * 10);

    for (qint32 r = 0; r < nVerts; ++r) {
        if (sourceVertexSet.contains(r)) {
            // Source vertex: identity mapping (weight = 1)
            vecTriplets.push_back(Eigen::Triplet<float>(r, vertexToSourceIdx[r], 1.0f));
        } else {
            const auto& weights = perVertexWeights[r];
            if (weights.empty()) continue;

            // Apply interpolation function and normalize weights
            float dWeightsSum = 0.0f;
            QVector<QPair<qint32, float>> vecBelowThresh;
            vecBelowThresh.reserve(static_cast<int>(weights.size()));

            for (const auto& w : weights) {
                const float dValueWeight = std::fabs(1.0f / static_cast<float>(interpolationFunction(w.dist)));
                dWeightsSum += dValueWeight;
                vecBelowThresh.push_back(qMakePair(w.sourceIdx, dValueWeight));
            }

            for (const auto& qp : vecBelowThresh) {
                vecTriplets.push_back(Eigen::Triplet<float>(r, qp.first, qp.second / dWeightsSum));
            }
        }
    }

    auto interpMat = QSharedPointer<SparseMatrix<float>>::create(nVerts, nSources);
    interpMat->setFromTriplets(vecTriplets.begin(), vecTriplets.end());

    return interpMat;
}

//=============================================================================================================

VectorXi GeometryInfo::projectSensors(const MatrixX3f &matVertices,
                                      const MatrixX3f &matSensorPositions)
{
    const qint32 iNumSensors = static_cast<qint32>(matSensorPositions.rows());

    qint32 iCores = QThread::idealThreadCount();
    if (iCores <= 0)
    {
        iCores = 2;
    }
#ifdef __EMSCRIPTEN__
    iCores = qMin(iCores, (qint32)2);
#endif

    const qint32 iSubArraySize = int(double(iNumSensors) / double(iCores));

    if(iSubArraySize <= 1)
    {
        return nearestNeighbor(matVertices, matSensorPositions, 0, iNumSensors);
    }

    QVector<QFuture<VectorXi> > vecThreads(iCores);
    qint32 iBeginOffset = 0;
    qint32 iEndOffset = iBeginOffset + iSubArraySize;
    for(qint32 i = 0; i < vecThreads.size(); ++i)
    {
        if(i == vecThreads.size()-1)
        {
            vecThreads[i] = QtConcurrent::run(nearestNeighbor,
                                              matVertices,
                                              matSensorPositions,
                                              iBeginOffset,
                                              iNumSensors);
            break;
        }
        else
        {
            vecThreads[i] = QtConcurrent::run(nearestNeighbor,
                                              matVertices,
                                              matSensorPositions,
                                              iBeginOffset,
                                              iEndOffset);
            iBeginOffset = iEndOffset;
            iEndOffset += iSubArraySize;
        }
    }

    for (QFuture<VectorXi>& f : vecThreads) {
        f.waitForFinished();
    }

    // concatenate partial results
    VectorXi vecOutputArray(iNumSensors);
    qint32 iOffset = 0;
    for(qint32 i = 0; i < vecThreads.size(); ++i)
    {
        const VectorXi& partial = vecThreads[i].result();
        vecOutputArray.segment(iOffset, partial.size()) = partial;
        iOffset += static_cast<qint32>(partial.size());
    }

    return vecOutputArray;
}

//=============================================================================================================

VectorXi GeometryInfo::nearestNeighbor(const MatrixX3f &matVertices,
                                       const MatrixX3f &matSensorPositions,
                                       qint32 iBegin,
                                       qint32 iEnd)
{
    VectorXi vecMappedSensors(iEnd - iBegin);

    for(qint32 s = iBegin; s < iEnd; ++s)
    {
        qint32 iChampionId = 0;
        double iChampDist = std::numeric_limits<double>::max();
        for(qint32 i = 0; i < matVertices.rows(); ++i)
        {
            double dDist = sqrt(squared(matVertices(i, 0) - matSensorPositions(s, 0))
                                + squared(matVertices(i, 1) - matSensorPositions(s, 1))
                                + squared(matVertices(i, 2) - matSensorPositions(s, 2)));
            if(dDist < iChampDist)
            {
                iChampionId = i;
                iChampDist = dDist;
            }
        }
        vecMappedSensors[s - iBegin] = iChampionId;
    }

    return vecMappedSensors;
}

//=============================================================================================================

void GeometryInfo::iterativeDijkstra(QSharedPointer<MatrixXd> matOutputDistMatrix,
                                     const MatrixX3f &matVertices,
                                     const std::vector<VectorXi> &vecNeighborVertices,
                                     const VectorXi &vecVertSubset,
                                     qint32 iBegin,
                                     qint32 iEnd,
                                     double dCancelDistance) {
    const std::vector<VectorXi> &vecAdjacency = vecNeighborVertices;
    qint32 n = static_cast<qint32>(vecAdjacency.size());
    QVector<double> vecMinDists(n);
    std::set< std::pair< double, qint32> > vertexQ;
    const double INF = FLOAT_INFINITY;

    for (qint32 i = iBegin; i < iEnd; ++i) {
        if ((i - iBegin) > 0 && (i - iBegin) % 100 == 0) {
            qDebug() << "GeometryInfo::iterativeDijkstra progress:" << (i - iBegin) << "/" << (iEnd - iBegin) << " (Thread range:" << iBegin << "-" << iEnd << ")";
        }

        qint32 iRoot = vecVertSubset[i];
        vertexQ.clear();
        vecMinDists.fill(INF);
        vecMinDists[iRoot] = 0.0;
        vertexQ.insert(std::make_pair(vecMinDists[iRoot], iRoot));

        while (vertexQ.empty() == false) {
            const double dDist = vertexQ.begin()->first;
            const qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());

            if (dDist <= dCancelDistance) {
                const VectorXi& vecNeighbours = vecAdjacency[u];

                for (Eigen::Index ne = 0; ne < vecNeighbours.size(); ++ne) {
                    qint32 v = vecNeighbours[ne];

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

        for (qint32 m = 0; m < vecMinDists.size(); ++m) {
            matOutputDistMatrix->coeffRef(m , i) = vecMinDists[m];
        }
    }
}

//=============================================================================================================

VectorXi GeometryInfo::filterBadChannels(QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                         const FIFFLIB::FiffInfo& fiffInfo,
                                         qint32 iSensorType) {
    std::vector<int> vecBadColumns;
    QVector<const FiffChInfo*> vecSensors;
    for(const FiffChInfo& s : fiffInfo.chs){
        if(s.kind == iSensorType && (s.unit == FIFF_UNIT_T || s.unit == FIFF_UNIT_V)){
           vecSensors.push_back(&s);
        }
    }

    for(const QString& b : fiffInfo.bads){
        for(int col = 0; col < vecSensors.size(); ++col){
            if(vecSensors[col]->ch_name == b){
                vecBadColumns.push_back(col);
                for(int row = 0; row < matDistanceTable->rows(); ++row){
                    matDistanceTable->coeffRef(row, col) = FLOAT_INFINITY;
                }
                break;
            }
        }
    }

    return Eigen::Map<VectorXi>(vecBadColumns.data(), static_cast<Eigen::Index>(vecBadColumns.size()));
}
