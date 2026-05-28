//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file warp.h
 * @since 2026
 * @date  March 2026
 * @brief Thin-plate-spline 3-D warp from landmark correspondences.
 *
 * @ref UTILSLIB::Warp implements Bookstein's thin-plate spline (TPS), the
 * minimum-bending-energy interpolant that maps an @c n×3 set of source
 * landmarks exactly onto an @c n×3 set of destination landmarks and
 * extends smoothly to the rest of @c R^3. It is mne-cpp's reference
 * tool for non-rigid registration of EEG electrodes and MEG digitiser
 * points onto a subject MRI scalp, and for warping a template mesh
 * (cortex, head surface) into the geometry of an individual subject.
 *
 * The warp evaluates as
 * @f$f(\mathbf{x}) = A\mathbf{x} + \mathbf{t} + \sum_i w_i\,U(\|\mathbf{x}-\mathbf{p}_i\|)@f$
 * with @f$U(r) = r@f$ for 3-D (the conditionally positive definite TPS
 * kernel). Fitting requires one @c (n+4)×(n+4) linear solve
 * (@c Eigen::FullPivLU); evaluation at @c m points is then
 * @c O(m*n + m). The class is also a wrapper around a tiny landmark
 * file reader (@ref readsLm) so calibration data shipped as plain text
 * can be ingested without extra glue.
 *
 * Reference: Bookstein (1989) "Principal Warps: Thin-Plate Splines and
 * the Decomposition of Deformations".
 */

#ifndef WARP_H
#define WARP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Thin-plate-spline (Bookstein) 3-D warp that maps source landmarks
 * exactly onto destination landmarks and extends smoothly to the rest
 * of @c R^3. Used for non-rigid registration of EEG/MEG sensors and
 * template-to-subject mesh warping.
 *
 * @brief Thin-plate-spline 3-D warp fitted from landmark correspondences.
 */
class MATHSHARED_EXPORT Warp
{
public:

    typedef QSharedPointer<Warp> SPtr;            /**< Shared pointer type for Warp. */
    typedef QSharedPointer<const Warp> ConstSPtr; /**< Const shared pointer type for Warp. */

    //=========================================================================================================
    /**
     * Calculates the TPS Warp of given setup
     *
     * @param[in] sLm      3D Landmarks of the source geometry.
     * @param[in] dLm      3D Landmarks of the destination geometry.
     * @param[in] sVert    Vertices of the source geometry.
     *
     * @return wVert   Vertices of the warped destination geometry.
     */
    Eigen::MatrixXf calculate(const Eigen::MatrixXf & sLm,
                              const Eigen::MatrixXf &dLm,
                              const Eigen::MatrixXf & sVert);

    //=========================================================================================================
    /**
     * Calculates the TPS Warp of a given setup for a List of vertices
     *
     * @param[in] sLm       3D Landmarks of the source geometry.
     * @param[in] dLm       3D Landmarks of the destination geometry.
     * @param[in/out] vertList  List of Vertices of the source geometry that are warped to the destination.
     */
    void calculate(const Eigen::MatrixXf & sLm,
                   const Eigen::MatrixXf &dLm,
                   QList<Eigen::MatrixXf> & vertList);

    //=========================================================================================================
    /**
     * Read electrode positions from MRI Database
     *
     * @param[in] electrodeFileName    .txt file of electrodes.
     *
     * @return electrodes   Matrix with electrode positions.
     */
    Eigen::MatrixXf readsLm(const QString &electrodeFileName);

    //=========================================================================================================
    /**
     * Read electrode positions from MRI Database
     *
     * @param[in] electrodeFileName    .txt file of electrodes.
     *
     * @return electrodes   Matrix with electrode positions.
     */
    Eigen::MatrixXf readsLm(const std::string &electrodeFileName);

private:

    //=========================================================================================================
    /**
     * Calculate the weighting parameters.
     *
     * @param[in] sLm      3D Landmarks of the source geometry.
     * @param[in] dLm      3D Landmarks of the destination geometry.
     * @param[out] warpWeight Weighting parameters of the tps warp.
     * @param[out] polWeight  Weighting papameters of the polynomial warp.
     */
    bool calcWeighting(const Eigen::MatrixXf& sLm,
                       const Eigen::MatrixXf &dLm,
                       Eigen::MatrixXf& warpWeight,
                       Eigen::MatrixXf& polWeight);

    //=========================================================================================================
    /**
     * Warp the Vertices of the source geometry
     *
     * @param[in] sVert    Vertices of the source geometry.
     * @param[in] sLm      3D Landmarks of the source geometry.
     * @param[in] warpWeight Weighting parameters of the tps warp.
     * @param[in] polWeight  Weighting papameters of the polynomial warp.
     *
     * @return Warped Vertices.
     */
    Eigen::MatrixXf warpVertices(const Eigen::MatrixXf & sVert,
                                 const Eigen::MatrixXf & sLm,
                                 const Eigen::MatrixXf& warpWeight,
                                 const Eigen::MatrixXf& polWeight);
};
} // NAMESPACE

#endif // WARP_H
