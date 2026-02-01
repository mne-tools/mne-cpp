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
    double cancelDist)
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
            // Interpolate from nearby sources
            QVector<QPair<int, float>> nearbyWeights;
            float weightSum = 0.0f;

            const RowVectorXd &rowDists = distanceTable->row(row);

            for (int col = 0; col < numCols; ++col) {
                float dist = rowDists[col];
                if (dist < cancelDist) {
                    float weight = std::fabs(1.0f / interpFunc(dist));
                    weightSum += weight;
                    nearbyWeights.append(qMakePair(col, weight));
                }
            }

            // Normalize weights and add to triplets
            for (const auto &nw : nearbyWeights) {
                triplets.append(Triplet<float>(row, nw.first, nw.second / weightSum));
            }
        }
    }

    interpMatrix->setFromTriplets(triplets.begin(), triplets.end());
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
