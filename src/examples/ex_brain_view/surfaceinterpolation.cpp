//=============================================================================================================
/**
 * @file     surfaceinterpolation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2026
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
 * @brief    SurfaceInterpolation class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfaceinterpolation.h"

#include <QtConcurrent/QtConcurrent>
#include <QSet>
#include <QDebug>

#include <cmath>
#include <set>
#include <limits>
#include <algorithm>
#include <QFuture>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QSharedPointer<MatrixXd> SurfaceInterpolation::computeDistanceTable(
    const MatrixX3f &vertices,
    const QVector<QVector<int>> &neighbors,
    QVector<int> &vertSubset,
    double cancelDist)
{
    // Handle empty subset - compute for all vertices (warning: expensive!)
    int numCols = vertSubset.size();
    if (vertSubset.empty()) {
        qDebug() << "[SurfaceInterpolation] Empty subset, computing full distance table";
        vertSubset.reserve(vertices.rows());
        for (int i = 0; i < vertices.rows(); ++i) {
            vertSubset.append(i);
        }
        numCols = vertices.rows();
    }

    // Create output matrix (rows=all vertices, cols=source subset)
    auto distMatrix = QSharedPointer<MatrixXd>::create(vertices.rows(), numCols);

    // Distribute work across available cores
    int numCores = qMax(1, QThread::idealThreadCount());
    int chunkSize = qMax(1, vertSubset.size() / numCores);

    QVector<QFuture<void>> futures(numCores);
    int begin = 0;

    for (int i = 0; i < numCores; ++i) {
        int end = (i == numCores - 1) ? vertSubset.size() : begin + chunkSize;

        futures[i] = QtConcurrent::run([=]() {
            dijkstraRange(distMatrix, vertices, neighbors, vertSubset, begin, end, cancelDist);
        });

        begin = end;
    }

    // Wait for all threads to complete
    for (auto &f : futures) {
        f.waitForFinished();
    }

    return distMatrix;
}

//=============================================================================================================

QSharedPointer<SparseMatrix<float>> SurfaceInterpolation::createInterpolationMatrix(
    const QVector<int> &sourceVertices,
    const QSharedPointer<MatrixXd> &distanceTable,
    InterpolationFunction interpFunc,
    double cancelDist,
    int maxNeighbors)
{
    if (!distanceTable || distanceTable->rows() == 0 || distanceTable->cols() == 0) {
        qDebug() << "[SurfaceInterpolation] Empty distance table";
        return QSharedPointer<SparseMatrix<float>>::create();
    }

    const int numRows = distanceTable->rows();
    const int numCols = sourceVertices.size();

    auto interpMatrix = QSharedPointer<SparseMatrix<float>>::create(numRows, numCols);

    // Build lookup set for source vertices
    QSet<int> sourceSet;
    for (int s : sourceVertices) {
        sourceSet.insert(s);
    }

    // Build triplet list for sparse matrix construction
    QVector<Triplet<float>> triplets;
    triplets.reserve(numRows * 10); // Estimate ~10 non-zeros per row

    for (int row = 0; row < numRows; ++row) {
        if (sourceSet.contains(row)) {
            // This vertex is a source - weight is 1.0
            int colIdx = sourceVertices.indexOf(row);
            triplets.append(Triplet<float>(row, colIdx, 1.0f));
        } else {
            // Interpolation from nearby sources
            QVector<QPair<int, float>> nearbyWeights;
            float weightSum = 0.0f;

            const RowVectorXd &rowDists = distanceTable->row(row);

            // Collect all valid neighbors within cancelDist
            for (int col = 0; col < numCols; ++col) {
                float dist = rowDists[col];
                if (dist < cancelDist) {
                    nearbyWeights.append(qMakePair(col, dist));
                }
            }

            // Optimization: Keep only k-nearest neighbors if maxNeighbors is set
            if (maxNeighbors > 0 && nearbyWeights.size() > maxNeighbors) {
                // Sort by distance (ascending)
                std::sort(nearbyWeights.begin(), nearbyWeights.end(),
                    [](const QPair<int, float> &a, const QPair<int, float> &b) {
                        return a.second < b.second;
                    });
                
                // Keep only top k
                nearbyWeights.resize(maxNeighbors);
            }

            // Calculate weights for the selected neighbors
            for (auto &nw : nearbyWeights) {
                float dist = nw.second;
                // Avoid division by zero for very small distances (should be handled by sourceSet check, but safety first)
                float weight = (dist < 1e-6) ? 1.0f : std::fabs(1.0f / interpFunc(dist));
                
                // Update the pair's second value to be the weight instead of distance
                nw.second = weight;
                weightSum += weight;
            }

            // Normalize weights and add to triplets
            if (weightSum > 0.0f) {
                for (const auto &nw : nearbyWeights) {
                    triplets.append(Triplet<float>(row, nw.first, nw.second / weightSum));
                }
            }
        }
    }

    interpMatrix->setFromTriplets(triplets.begin(), triplets.end());
    return interpMatrix;
}

//=============================================================================================================

QSharedPointer<Eigen::SparseMatrix<float>> SurfaceInterpolation::createInterpolationMatrixEuclidean(
    const Eigen::MatrixX3f &vertices,
    const QVector<int> &sourceVertices,
    InterpolationFunction interpFunc,
    double cancelDist,
    int maxNeighbors)
{
    if (vertices.rows() == 0 || sourceVertices.isEmpty()) {
        qDebug() << "[SurfaceInterpolation] Empty vertices or source vertices";
        return QSharedPointer<Eigen::SparseMatrix<float>>::create();
    }

    const int numRows = vertices.rows();
    const int numCols = sourceVertices.size();
    const float cancelDistSq = cancelDist * cancelDist;

    // 1. Compute Bounding Box of Source Vertices
    Eigen::Vector3f minBounds(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Eigen::Vector3f maxBounds(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    // Pre-calculate source positions and update bounds
    Eigen::MatrixX3f sourcePositions(numCols, 3);
    for(int i = 0; i < numCols; ++i) {
        int srcIdx = sourceVertices[i];
        if (srcIdx < 0 || srcIdx >= numRows) {
            qWarning() << "[SurfaceInterpolation] Source vertex index out of bounds:" << srcIdx;
            continue;
        }
        
        Eigen::Vector3f pos = vertices.row(srcIdx); // Implicit transpose (1x3 -> 3x1) handled by assignment
        sourcePositions.row(i) = pos;
        
        minBounds = minBounds.cwiseMin(pos);
        maxBounds = maxBounds.cwiseMax(pos);
    }

    // 2. Setup Spatial Grid
    // Cell size should be at least cancelDist to ensure we check enough area
    float cellSize = std::max(static_cast<float>(cancelDist), 1e-4f);
    
    int dimX = std::ceil((maxBounds.x() - minBounds.x()) / cellSize) + 1;
    int dimY = std::ceil((maxBounds.y() - minBounds.y()) / cellSize) + 1;
    int dimZ = std::ceil((maxBounds.z() - minBounds.z()) / cellSize) + 1;
    
    // Flattened grid: index = x + y*dimX + z*dimX*dimY
    QVector<QVector<int>> grid(dimX * dimY * dimZ);

    // 3. Populate Grid
    for(int i = 0; i < numCols; ++i) {
        Eigen::Vector3f pos = sourcePositions.row(i);
        int gx = static_cast<int>((pos.x() - minBounds.x()) / cellSize);
        int gy = static_cast<int>((pos.y() - minBounds.y()) / cellSize);
        int gz = static_cast<int>((pos.z() - minBounds.z()) / cellSize);
        
        // Clamp to be safe (though logic above should prevent overflow)
        gx = std::clamp(gx, 0, dimX - 1);
        gy = std::clamp(gy, 0, dimY - 1);
        gz = std::clamp(gz, 0, dimZ - 1);
        
        grid[gx + gy * dimX + gz * dimX * dimY].append(i);
    }

    qDebug() << "[SurfaceInterpolation] Spatial Grid created:" << dimX << "x" << dimY << "x" << dimZ 
             << "CellSize:" << cellSize << "Sources:" << numCols;

    // 4. Parallel Neighbor Search
    auto computeRange = [vertices, sourcePositions, grid, minBounds, cellSize, dimX, dimY, dimZ, numCols, interpFunc, cancelDistSq, maxNeighbors](int begin, int end) -> QVector<Eigen::Triplet<float>> {
        QVector<Eigen::Triplet<float>> localTriplets;
        localTriplets.reserve((end - begin) * (maxNeighbors > 0 ? maxNeighbors : 10));

        // Offsets for 3x3x3 block search
        int offsets[27][3];
        int idx = 0;
        for(int z=-1; z<=1; ++z)
            for(int y=-1; y<=1; ++y)
                for(int x=-1; x<=1; ++x) {
                    offsets[idx][0] = x; offsets[idx][1] = y; offsets[idx][2] = z;
                    idx++;
                }

        for (int row = begin; row < end; ++row) {
            Eigen::Vector3f vPos = vertices.row(row);
            
            // Find grid cell for this vertex
            int vx = static_cast<int>((vPos.x() - minBounds.x()) / cellSize);
            int vy = static_cast<int>((vPos.y() - minBounds.y()) / cellSize);
            int vz = static_cast<int>((vPos.z() - minBounds.z()) / cellSize);
            
            QVector<QPair<int, float>> neighbors;
            
            // Search 3x3x3 block
            for (int i = 0; i < 27; ++i) {
                int nx = vx + offsets[i][0];
                int ny = vy + offsets[i][1];
                int nz = vz + offsets[i][2];
                
                if (nx >= 0 && nx < dimX && ny >= 0 && ny < dimY && nz >= 0 && nz < dimZ) {
                    const QVector<int>& cellSources = grid[nx + ny * dimX + nz * dimX * dimY];
                    
                    for (int srcIdx : cellSources) {
                        float distSq = (vPos - sourcePositions.row(srcIdx).transpose()).squaredNorm();
                        if (distSq < cancelDistSq) {
                            neighbors.append(qMakePair(srcIdx, distSq));
                        }
                    }
                }
            }

            // Optimization: Keep only k-nearest neighbors
            if (maxNeighbors > 0 && neighbors.size() > maxNeighbors) {
                std::partial_sort(neighbors.begin(), neighbors.begin() + maxNeighbors, neighbors.end(),
                    [](const QPair<int, float> &a, const QPair<int, float> &b) {
                        return a.second < b.second;
                    });
                neighbors.resize(maxNeighbors);
            }

            // Calculate weights
            float weightSum = 0.0f;
            for (auto &nw : neighbors) {
                float dist = std::sqrt(nw.second);
                float weight = (dist < 1e-6f) ? 1.0f : std::fabs(1.0f / interpFunc(dist));
                nw.second = weight;
                weightSum += weight;
            }

            if (weightSum > 0.0f) {
                for (const auto &nw : neighbors) {
                    localTriplets.append(Eigen::Triplet<float>(row, nw.first, nw.second / weightSum));
                }
            }
        }
        return localTriplets;
    };

    // Run in parallel
    int coreCount = QThread::idealThreadCount();
    if (coreCount <= 0) coreCount = 2;
    
    int chunkSize = numRows / coreCount;
    if (chunkSize < 100) {
        chunkSize = numRows;
        coreCount = 1;
    }

    QVector<QFuture<QVector<Eigen::Triplet<float>>>> futures;
    int start = 0;

    for (int i = 0; i < coreCount; ++i) {
        int end = (i == coreCount - 1) ? numRows : start + chunkSize;
        futures.append(QtConcurrent::run(computeRange, start, end));
        start = end;
    }

    QVector<Eigen::Triplet<float>> allTriplets;
    for (auto &f : futures) {
        f.waitForFinished();
        allTriplets.append(f.result());
    }

    auto interpMatrix = QSharedPointer<Eigen::SparseMatrix<float>>::create(numRows, numCols);
    interpMatrix->setFromTriplets(allTriplets.begin(), allTriplets.end());
    
    return interpMatrix;
}

//=============================================================================================================

VectorXf SurfaceInterpolation::interpolateSignal(
    const SparseMatrix<float> &interpMatrix,
    const VectorXf &sourceData)
{
    if (interpMatrix.cols() != sourceData.rows()) {
        qDebug() << "[SurfaceInterpolation] Dimension mismatch:" << interpMatrix.cols() << "vs" << sourceData.rows();
        return VectorXf();
    }

    return interpMatrix * sourceData;
}

//=============================================================================================================

double SurfaceInterpolation::linear(double d)
{
    return d;
}

//=============================================================================================================

double SurfaceInterpolation::cubic(double d)
{
    return d * d * d;
}

//=============================================================================================================

double SurfaceInterpolation::gaussian(double d)
{
    return std::exp(-(d * d) / 2.0);
}

//=============================================================================================================

double SurfaceInterpolation::square(double d)
{
    return std::max(-(1.0 / 9.0) * (d * d) + 1.0, 0.0);
}

//=============================================================================================================

void SurfaceInterpolation::dijkstraRange(
    QSharedPointer<MatrixXd> distMatrix,
    const MatrixX3f &vertices,
    const QVector<QVector<int>> &neighbors,
    const QVector<int> &vertSubset,
    int begin,
    int end,
    double cancelDist)
{
    const int numVerts = neighbors.size();
    const double INF = std::numeric_limits<float>::infinity();

    QVector<double> minDists(numVerts);
    std::set<std::pair<double, int>> queue;

    // Process each source vertex in our range
    for (int i = begin; i < end; ++i) {
        int root = vertSubset.at(i);

        // Initialize distances
        minDists.fill(INF);
        minDists[root] = 0.0;
        queue.clear();
        queue.insert(std::make_pair(0.0, root));

        // Dijkstra main loop
        while (!queue.empty()) {
            auto it = queue.begin();
            double dist = it->first;
            int u = it->second;
            queue.erase(it);

            // Stop if beyond cancel distance
            if (dist > cancelDist) continue;

            // Visit neighbors
            for (int v : neighbors[u]) {
                // Compute edge length
                float dx = vertices(u, 0) - vertices(v, 0);
                float dy = vertices(u, 1) - vertices(v, 1);
                float dz = vertices(u, 2) - vertices(v, 2);
                double edgeLen = std::sqrt(dx*dx + dy*dy + dz*dz);

                double newDist = dist + edgeLen;
                if (newDist < minDists[v]) {
                    // Update distance (remove old entry if exists)
                    queue.erase(std::make_pair(minDists[v], v));
                    minDists[v] = newDist;
                    queue.insert(std::make_pair(newDist, v));
                }
            }
        }

        // Store results in output matrix
        for (int m = 0; m < numVerts; ++m) {
            distMatrix->coeffRef(m, i) = minDists[m];
        }
    }
}
