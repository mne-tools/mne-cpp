//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     interpolation.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Distance-based sparse interpolation weights and per-frame signal smoothing on triangulated meshes.
 *
 * Given the geodesic distance table produced by @ref DISP3DLIB::GeometryInfo,
 * Interpolation builds a sparse <em>n_vertices x n_sources</em>
 * weight matrix in which each vertex's row contains only those
 * sources within @c cancelDist metres. Weights fall off through one
 * of four radial basis functions selectable at runtime:
 * @ref DISP3DLIB::Interpolation::linear "linear", @ref DISP3DLIB::Interpolation::gaussian "gaussian", @ref DISP3DLIB::Interpolation::square "square" (negative parabola) and
 * @ref DISP3DLIB::Interpolation::cubic "cubic" (cubic hyperbola).
 *
 * @ref DISP3DLIB::Interpolation::interpolateSignal "interpolateSignal" multiplies the precomputed matrix by a
 * per-frame source / sensor vector to produce smoothed per-vertex
 * values; the matrix is built once per scene and reused on every
 * real-time frame in @ref DISP3DLIB::RtSourceDataWorker and
 * @ref DISP3DLIB::RtSensorDataWorker, keeping per-frame cost at one sparse mat-vec.
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

namespace DISP3DLIB
{

#ifndef FLOAT_INFINITY
#define FLOAT_INFINITY std::numeric_limits<float>::infinity()
#endif

//=============================================================================================================
/**
 * This class holds methods for creating distance-based weight matrices and for interpolating signals.
 *
 * @brief This class holds methods for creating distance-based weight matrices and for interpolating signals
 *
 * @snippet ex_disp3d_scene/main.cpp interpolation_usage
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
     *
     * @param[in] vecProjectedSensors     Mesh vertex index of each sensor (one column per sensor).
     * @param[in] matDistanceTable        Distance table (nVertices x nSensors), e.g. from GeometryInfo::scdc.
     * @param[in] interpolationFunction   Distance function whose inverse magnitude gives the weight.
     * @param[in] dCancelDist             Distances at or above this value get zero weight.
     * @param[in] vecExcludeIndex         Sensor column indices that are not pinned to their own vertex.
     * @return Row-normalised sparse weight matrix (nVertices x nSensors), or an empty matrix if the table is empty.
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> createInterpolationMat(const Eigen::VectorXi& vecProjectedSensors,
                                                                             const QSharedPointer<Eigen::MatrixXd> matDistanceTable,
                                                                             double (*interpolationFunction)(double),
                                                                             const double dCancelDist = FLOAT_INFINITY,
                                                                             const Eigen::VectorXi& vecExcludeIndex = Eigen::VectorXi());

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (shared pointer version).
     *
     * @param[in] matInterpolationMatrix   Weight matrix (nVertices x nSensors).
     * @param[in] vecMeasurementData       Sensor values (nSensors).
     * @return Interpolated vertex values, or an empty vector on dimension mismatch.
     */
    static Eigen::VectorXf interpolateSignal(const QSharedPointer<Eigen::SparseMatrix<float>> matInterpolationMatrix,
                                             const QSharedPointer<Eigen::VectorXf>& vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief interpolateSignal   Interpolates sensor data using the weight matrix (reference version).
     *
     * @param[in] matInterpolationMatrix   Weight matrix (nVertices x nSensors).
     * @param[in] vecMeasurementData       Sensor values (nSensors).
     * @return Interpolated vertex values, or an empty vector on dimension mismatch.
     */
    static Eigen::VectorXf interpolateSignal(const Eigen::SparseMatrix<float>& matInterpolationMatrix,
                                             const Eigen::VectorXf& vecMeasurementData);

    //=========================================================================================================
    /**
     * @brief linear   Identity interpolation function.
     *
     * @param[in] dIn   Input distance.
     * @return dIn unchanged.
     */
    static double linear(const double dIn);

    //=========================================================================================================
    /**
     * @brief gaussian   Gaussian interpolation function (sigma=1).
     *
     * @param[in] dIn   Input distance.
     * @return exp(-dIn^2 / 2).
     */
    static double gaussian(const double dIn);

    //=========================================================================================================
    /**
     * @brief square   Negative parabola interpolation function with y-offset of 1.
     *
     * @param[in] dIn   Input distance.
     * @return max(1 - dIn^2 / 9, 0).
     */
    static double square(const double dIn);

    //=========================================================================================================
    /**
     * @brief cubic   Cubic hyperbola interpolation function.
     *
     * @param[in] dIn   Input distance.
     * @return dIn^3.
     */
    static double cubic(const double dIn);
};

} // namespace DISP3DLIB

#endif // INTERPOLATION_H
