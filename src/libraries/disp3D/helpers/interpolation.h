//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file interpolation.h
 * @since 2026
 * @date  March 2026
 * @brief Distance-based sparse interpolation weights and per-frame signal smoothing on triangulated meshes.
 *
 * Given the geodesic distance table produced by @ref GeometryInfo,
 * Interpolation builds a sparse <em>n_vertices x n_sources</em>
 * weight matrix in which each vertex's row contains only those
 * sources within @c cancelDist metres. Weights fall off through one
 * of four radial basis functions selectable at runtime:
 * @ref linear, @ref gaussian, @ref square (negative parabola) and
 * @ref cubic (cubic hyperbola).
 *
 * @ref interpolateSignal multiplies the precomputed matrix by a
 * per-frame source / sensor vector to produce smoothed per-vertex
 * values; the matrix is built once per scene and reused on every
 * real-time frame in @ref RtSourceDataWorker and
 * @ref RtSensorDataWorker, keeping per-frame cost at one sparse mat-vec.
 */

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <limits>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DLIB {

#ifndef FLOAT_INFINITY
#define FLOAT_INFINITY std::numeric_limits<float>::infinity()
#endif

//=============================================================================================================
/**
 * This class holds methods for creating distance-based weight matrices and for interpolating signals.
 *
 * @brief This class holds methods for creating distance-based weight matrices and for interpolating signals
 */

class DISP3DSHARED_EXPORT Interpolation
{

public:
    typedef QSharedPointer<Interpolation> SPtr;
    typedef QSharedPointer<const Interpolation> ConstSPtr;

    Interpolation() = delete;

    //=========================================================================================================
    /**
     * @brief createInterpolationMat   Calculates the weight matrix for interpolation.
     */
static QSharedPointer<Eigen::SparseMatrix<float> > createInterpolationMat(const Eigen::VectorXi &vecProjectedSensors,
                                                                                const QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                                                                double (*interpolationFunction) (double),
                                                                                const double dCancelDist = FLOAT_INFINITY,
                                                                                const Eigen::VectorXi &vecExcludeIndex = Eigen::VectorXi());

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (shared pointer version).
     */
    static Eigen::VectorXf interpolateSignal(const QSharedPointer<Eigen::SparseMatrix<float> > matInterpolationMatrix,
                                             const QSharedPointer<Eigen::VectorXf> &vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (reference version).
     */
    static Eigen::VectorXf interpolateSignal(const Eigen::SparseMatrix<float> &matInterpolationMatrix,
                                             const Eigen::VectorXf &vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief linear   Identity interpolation function.
     */
    static double linear(const double dIn);

    //=========================================================================================================
    /**
     * @brief gaussian   Gaussian interpolation function (sigma=1).
     */
    static double gaussian(const double dIn);

    //=========================================================================================================
    /**
     * @brief square   Negative parabola interpolation function with y-offset of 1.
     */
    static double square(const double dIn);

    //=========================================================================================================
    /**
     * @brief cubic   Cubic hyperbola interpolation function.
     */
    static double cubic(const double dIn);
};

} // namespace DISP3DLIB

#endif // INTERPOLATION_H
