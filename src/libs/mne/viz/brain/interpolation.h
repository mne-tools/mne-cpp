//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     interpolation.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     May 2017
 * @brief    Distance-based sparse interpolation weights and per-frame signal smoothing on triangulated meshes.
 *
 * Given the geodesic distance table produced by @ref mne::viz::GeometryInfo,
 * Interpolation builds a sparse <em>n_vertices x n_sources</em>
 * weight matrix in which each vertex's row contains only those
 * sources within @c cancelDist metres. Weights fall off through one
 * of four radial basis functions selectable at runtime:
 * @ref mne::viz::Interpolation::linear "linear", @ref mne::viz::Interpolation::gaussian "gaussian", @ref mne::viz::Interpolation::square "square" (negative parabola) and
 * @ref mne::viz::Interpolation::cubic "cubic" (cubic hyperbola).
 *
 * @ref mne::viz::Interpolation::interpolateSignal "interpolateSignal" multiplies the precomputed matrix by a
 * per-frame source / sensor vector to produce smoothed per-vertex
 * values; the matrix is built once per scene and reused on every
 * real-time frame in @ref mne::viz::qt::RtSourceDataWorker and
 * @ref mne::viz::qt::RtSensorDataWorker, keeping per-frame cost at one sparse mat-vec.
 */

#ifndef VIZ_INTERPOLATION_H
#define VIZ_INTERPOLATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <limits>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace mne::viz
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

class VIZSHARED_EXPORT Interpolation
{
public:
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
    static std::shared_ptr<Eigen::SparseMatrix<float>> createInterpolationMat(const Eigen::VectorXi& vecProjectedSensors,
                                                                              const std::shared_ptr<Eigen::MatrixXd>& matDistanceTable,
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
    static Eigen::VectorXf interpolateSignal(const std::shared_ptr<Eigen::SparseMatrix<float>>& matInterpolationMatrix,
                                             const std::shared_ptr<Eigen::VectorXf>& vecMeasurementData);

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

} // namespace mne::viz

#endif // VIZ_INTERPOLATION_H
