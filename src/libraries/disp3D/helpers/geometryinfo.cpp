//=============================================================================================================
/**
 * @file     geometryinfo.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     May, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    GeometryInfo class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryinfo.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <fstream>
#include <set>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
    std::function<void(int, int)> progressCallback)
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

    // We'll collect per-vertex sparse distance info
    // Using a vector of vectors: for each mesh vertex, store (sourceIdx, dist) pairs
    std::vector<std::vector<VertexWeight>> perVertexWeights(nVerts);

    // Process sources sequentially (each Dijkstra is already fast with cancelDist)
    QVector<double> vecMinDists(nAdj);
    std::set<std::pair<double, qint32>> vertexQ;
    const double INF = FLOAT_INFINITY;

    for (qint32 s = 0; s < nSources; ++s) {
        if (progressCallback && s > 0 && s % 100 == 0) {
            progressCallback(s, nSources);
        }

        qint32 iRoot = vecVertSubset[s];
        vertexQ.clear();
        vecMinDists.fill(INF);
        vecMinDists[iRoot] = 0.0;
        vertexQ.insert(std::make_pair(0.0, iRoot));

        while (!vertexQ.empty()) {
            const double dDist = vertexQ.begin()->first;
            const qint32 u = vertexQ.begin()->second;
            vertexQ.erase(vertexQ.begin());

            if (dDist <= dCancelDist) {
                const VectorXi& vecNeighbours = vecNeighborVertices[u];

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

        // Collect all vertices reachable from this source within cancelDist
        for (qint32 m = 0; m < nAdj; ++m) {
            if (vecMinDists[m] < dCancelDist) {
                perVertexWeights[m].push_back({s, static_cast<float>(vecMinDists[m])});
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
