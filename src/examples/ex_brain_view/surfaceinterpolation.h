//=============================================================================================================
/**
 * @file     surfaceinterpolation.h
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
 * @brief    SurfaceInterpolation class declaration - provides distance-based interpolation on mesh surfaces.
 *
 */

#ifndef SURFACEINTERPOLATION_H
#define SURFACEINTERPOLATION_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
/**
 * @brief Surface interpolation utilities for spreading sparse values across a mesh.
 *
 * This class provides static methods for:
 * - Computing surface-constrained distances (Dijkstra-based)
 * - Creating sparse interpolation weight matrices
 * - Applying interpolation to signals
 *
 * The interpolation spreads values from sparse source vertices to all mesh vertices
 * based on geodesic (surface-constrained) distances.
 */
class SurfaceInterpolation
{
public:
    //=========================================================================================================
    /**
     * @brief Interpolation function type.
     */
    typedef double (*InterpolationFunction)(double);

    //=========================================================================================================
    /**
     * @brief Deleted constructor (static class).
     */
    SurfaceInterpolation() = delete;

    //=========================================================================================================
    /**
     * @brief Compute surface-constrained distances from a subset of vertices to all vertices.
     *
     * Uses Dijkstra's algorithm to compute shortest-path distances along the mesh surface.
     * Distances greater than cancelDist are set to infinity.
     *
     * @param[in] vertices         Nx3 matrix of vertex positions.
     * @param[in] neighbors        Neighbor vertex indices for each vertex.
     * @param[in,out] vertSubset   Source vertex indices (filled with all if empty).
     * @param[in] cancelDist       Maximum distance threshold.
     * @return Distance matrix (rows=all vertices, cols=source vertices).
     */
    static QSharedPointer<Eigen::MatrixXd> computeDistanceTable(
        const Eigen::MatrixX3f &vertices,
        const QVector<QVector<int>> &neighbors,
        QVector<int> &vertSubset,
        double cancelDist = std::numeric_limits<float>::infinity());

    //=========================================================================================================
    /**
     * @brief Create sparse interpolation weight matrix.
     *
     * Creates a matrix that maps source vertex values to all vertices based on
     * distance-weighted interpolation.
     *
     * @param[in] sourceVertices   Indices of source vertices.
     * @param[in] distanceTable    Distance matrix from computeDistanceTable().
     * @param[in] interpFunc       Interpolation function (linear, cubic, gaussian, square).
     * @param[in] cancelDist       Maximum distance for interpolation.
     * @return Sparse interpolation matrix (rows=all vertices, cols=sources).
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> createInterpolationMatrix(
        const QVector<int> &sourceVertices,
        InterpolationFunction interpFunc,
        double cancelDist = std::numeric_limits<float>::infinity(),
        int maxNeighbors = -1);

    //=========================================================================================================
    /**
     * @brief Create sparse interpolation weight matrix using Euclidean distance (Grid/Brute-force).
     *
     * Faster than surface-based interpolation for initial setup, but ignores surface topology.
     * Finds k-nearest neighbors in 3D space.
     *
     * @param[in] vertices         Mesh vertices (targets).
     * @param[in] sourceVertices   Indices of source vertices.
     * @param[in] interpFunc       Interpolation function.
     * @param[in] cancelDist       Maximum distance.
     * @param[in] maxNeighbors     Number of neighbors to find (k).
     * @return Sparse interpolation matrix.
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> createInterpolationMatrixEuclidean(
        const Eigen::MatrixX3f &vertices,
        const QVector<int> &sourceVertices,
        InterpolationFunction interpFunc,
        double cancelDist,
        int maxNeighbors);

    //=========================================================================================================
    /**
     * @brief Apply interpolation matrix to a signal vector.
     *
     * @param[in] interpMatrix     Interpolation matrix.
     * @param[in] sourceData       Source vertex data vector.
     * @return Interpolated values for all vertices.
     */
    static Eigen::VectorXf interpolateSignal(
        const Eigen::SparseMatrix<float> &interpMatrix,
        const Eigen::VectorXf &sourceData);

    //=========================================================================================================
    // Interpolation Functions
    //=========================================================================================================

    /**
     * @brief Linear interpolation function (f(d) = d).
     */
    static double linear(double d);

    /**
     * @brief Cubic interpolation function (f(d) = d³).
     */
    static double cubic(double d);

    /**
     * @brief Gaussian interpolation function (f(d) = e^(-d²/2)).
     */
    static double gaussian(double d);

    /**
     * @brief Quadratic interpolation function.
     */
    static double square(double d);

private:
    //=========================================================================================================
    /**
     * @brief Dijkstra algorithm for a range of source vertices.
     *
     * @param[out] distMatrix      Output distance matrix.
     * @param[in] vertices         Vertex positions.
     * @param[in] neighbors        Neighbor information.
     * @param[in] vertSubset       Source vertices.
     * @param[in] begin            Start index in subset.
     * @param[in] end              End index in subset.
     * @param[in] cancelDist       Distance threshold.
     */
    static void dijkstraRange(
        QSharedPointer<Eigen::MatrixXd> distMatrix,
        const Eigen::MatrixX3f &vertices,
        const QVector<QVector<int>> &neighbors,
        const QVector<int> &vertSubset,
        int begin,
        int end,
        double cancelDist);
};

#endif // SURFACEINTERPOLATION_H
