//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     geometryinfo.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Surface-constrained geodesic distance and sensor-to-mesh projection helpers.
 *
 * GeometryInfo provides the geometric kernels that drive every
 * smooth source / sensor overlay in disp3D. @ref scdc runs a
 * multi-source Dijkstra on the surface adjacency graph to compute
 * geodesic (surface-constrained) distances from a set of seed
 * vertices to all vertices on the mesh; the result is used by
 * @ref Interpolation to build a sparse weight matrix that propagates
 * source / sensor values across the cortex without crossing sulci.
 *
 * @ref scdcInterpolationMat fuses the Dijkstra pass and the
 * weight-matrix construction so the dense distance table is never
 * materialised &mdash; essential for high-resolution ico-5 source spaces
 * where the dense table would exceed several gigabytes.
 *
 * @ref projectSensors performs Euclidean nearest-vertex projection
 * for MEG / EEG sensors onto the head or helmet mesh; @ref
 * filterBadChannels prunes columns of the distance table for
 * channels marked bad in the @ref FIFFLIB::FiffInfo.
 */

#ifndef GEOMETRYINFO_H
#define GEOMETRYINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <fiff/fiff_evoked.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <limits>
#include <functional>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DLIB {

#define FLOAT_INFINITY std::numeric_limits<float>::infinity()

//=============================================================================================================
/**
 * This class allows sensor-to-mesh mapping and calculation of surface constrained distances.
 *
 * @brief This class holds static methods for sensor-to-mesh mapping and surface constrained distance calculation on a mesh
 */

class DISP3DSHARED_EXPORT GeometryInfo
{

public:
    typedef QSharedPointer<GeometryInfo> SPtr;
    typedef QSharedPointer<const GeometryInfo> ConstSPtr;

    GeometryInfo() = delete;

    //=========================================================================================================
    /**
     * @brief scdc   Calculates surface constrained distances on a mesh.
     */
    static QSharedPointer<Eigen::MatrixXd> scdc(const Eigen::MatrixX3f &matVertices,
                                                const std::vector<Eigen::VectorXi> &vecNeighborVertices,
                                                Eigen::VectorXi &vecVertSubset,
                                                double dCancelDist = FLOAT_INFINITY);

    //=========================================================================================================
    /**
     * @brief scdcInterpolationMat   Computes geodesic distances (SCDC) and builds the sparse interpolation
     *        matrix in a single pass, without allocating a dense nVertices x nSources distance table.
     *        Produces identical results to scdc() + Interpolation::createInterpolationMat() but uses
     *        dramatically less memory (sparse representation only stores entries within cancelDist).
     *
     * @param[in] matVertices              Vertex positions (nVertices x 3).
     * @param[in] vecNeighborVertices      Adjacency list for each vertex.
     * @param[in] vecVertSubset            Source vertex indices.
     * @param[in] interpolationFunction    Weight function (e.g. Interpolation::cubic).
     * @param[in] dCancelDist             Maximum geodesic distance for interpolation.
     * @param[in] progressCallback         Optional callback reporting Dijkstra progress (current, total).
     * @param[in] cancelledFlag            Optional pointer to an atomic bool; when set to true the computation aborts early.
     * @return Sparse interpolation matrix (nVertices x nSources), or empty matrix if cancelled.
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> scdcInterpolationMat(
        const Eigen::MatrixX3f &matVertices,
        const std::vector<Eigen::VectorXi> &vecNeighborVertices,
        const Eigen::VectorXi &vecVertSubset,
        double (*interpolationFunction)(double),
        double dCancelDist,
        std::function<void(int, int)> progressCallback = nullptr,
        const std::atomic<bool> *cancelledFlag = nullptr);

    //=========================================================================================================
    /**
     * @brief projectSensors   Calculates the nearest neighbor vertex to each sensor.
     */
    static Eigen::VectorXi projectSensors(const Eigen::MatrixX3f &matVertices,
                                          const Eigen::MatrixX3f &matSensorPositions);

    //=========================================================================================================
    /**
     * @brief filterBadChannels   Filters bad channels from distance table.
     */
    static Eigen::VectorXi filterBadChannels(QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                             const FIFFLIB::FiffInfo& fiffInfo,
                                             qint32 iSensorType);

protected:
    static inline double squared(double dBase);

    static Eigen::VectorXi nearestNeighbor(const Eigen::MatrixX3f &matVertices,
                                           const Eigen::MatrixX3f &matSensorPositions,
                                           qint32 iBegin,
                                           qint32 iEnd);

    static void iterativeDijkstra(QSharedPointer<Eigen::MatrixXd> matOutputDistMatrix,
                                  const Eigen::MatrixX3f &matVertices,
                                  const std::vector<Eigen::VectorXi> &vecNeighborVertices,
                                  const Eigen::VectorXi &vecVertSubset,
                                  qint32 iBegin,
                                  qint32 iEnd,
                                  double dCancelDistance);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

double GeometryInfo::squared(double dBase)
{
    return dBase * dBase;
}

} // namespace DISP3DLIB

#endif // GEOMETRYINFO_H
