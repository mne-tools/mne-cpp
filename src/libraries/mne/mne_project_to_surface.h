//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file mne_project_to_surface.h
 * @since 2022
 * @date  April 2026
 * @brief Geometric projection of a 3D point onto the closest cortex triangle.
 *
 * @ref MNELIB::MNEProjectToSurface provides the point-to-triangle
 * projection used when associating MRI fiducials with the cortical
 * surface, when mapping discrete dipole sources to the nearest cortical
 * vertex and during head-shape registration. The implementation mirrors
 * @c mne_project_to_surface in the MNE C tools.
 */

#ifndef MNELIB_MNEPROJECTTOSURFACE_H
#define MNELIB_MNEPROJECTTOSURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB {

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEBemSurface;

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Projects 3-D points onto a triangulated surface mesh and returns nearest vertices and distances
 */

class MNESHARED_EXPORT MNEProjectToSurface
{

public:
    typedef QSharedPointer<MNEProjectToSurface> SPtr;            /**< Shared pointer type for MNEProjectToSurface. */
    typedef QSharedPointer<const MNEProjectToSurface> ConstSPtr; /**< Const shared pointer type for MNEProjectToSurface. */

    //=========================================================================================================
    /**
     * Constructs a MNEProjectToSurface object.
     */
    MNEProjectToSurface();

    //=========================================================================================================
    /**
     * Constructs a MNEProjectToSurface with the data of a MNEBemSurface object.
     * @brief Build a projector that snaps points onto the triangulation of @p p_MNEBemSurf.
     * @param[in] p_MNEBemSurf   The MNEBemSurface to which is to be projected.
     */
    MNEProjectToSurface(const MNELIB::MNEBemSurface &p_MNEBemSurf);

    //=========================================================================================================
    /**
     * Projects a set of points r on the FsSurface
     *
     * @brief Project a set of points onto the surface and return the nearest triangle and signed distance per point.
     *
     * @param[in] r         Set of pionts, which are to be projectied.
     * @param[in] np        number of points.
     * @param[out] rTri     set of points on the surface.
     * @param[out] nearest  Triangle of the new point.
     * @param[out] dist     Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool find_closest_on_surface(const Eigen::MatrixXf &r, const int np, Eigen::MatrixXf &rTri,
                                     Eigen::VectorXi &nearest, Eigen::VectorXf &dist);

protected:

private:
    //=========================================================================================================
    /**
     * Projects a point r on the FsSurface
     *
     * @brief Project a single point onto the surface and return its triangle, projected position and distance.
     *
     * @param[in] r         Piont, which is to be projectied.
     * @param[out] rTri     Point on the surface.
     * @param[out] bestTri  Triangle of the new point.
     * @param[out] bestDist Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool project_to_surface(const Eigen::Vector3f &r, Eigen::Vector3f &rTri, int &bestTri, float &bestDist);

    //=========================================================================================================
    /**
     * Finds the nearest point to a point r on a given triangle.
     *
     * @brief Closest point on a single triangle, returned in the (p,q) barycentric-style local coordinates.
     *
     * @param[in] r     Point in space.
     * @param[in] tri   The Triangle of the surface.
     * @param[out] p    Coordiante in Triangel System (rTri = r1 + p*r12 +q*r13).
     * @param[out] q    Coordiante in Triangel System (rTri = r1 + p*r12 +q*r13).
     * @param[out] dist Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool nearest_triangle_point(const Eigen::Vector3f &r, const int tri, float &p, float &q, float &dist);

    //=========================================================================================================
    /**
     * Converts a point given in triangel coordinates (p,q) to the kartesian system.
     *
     * @brief Convert local triangle (p,q) coordinates back to a 3D Cartesian point on the surface.
     *
     * @param[out] rTri Coordiante in kartesian System.
     * @param[in] p     Coordiante in Triangel System (r = r1 + p*r12 +q*r13).
     * @param[in] q     Coordiante in Triangel System (r = r1 + p*r12 +q*r13).
     * @param[in] tri   The Triangle of the surface.
     *
     * @return true if succeeded, false otherwise.
     */
    bool project_to_triangle(Eigen::Vector3f &rTri, const float p, const float q, const int tri);

    Eigen::MatrixX3f r1;         /**< Cartesian Vector to the first triangel corner. */
    Eigen::MatrixX3f r12;        /**< Cartesian Vector from the first to the second triangel corner. */
    Eigen::MatrixX3f r13;        /**< Cartesian Vector from the first to the third triangel corner. */
    Eigen::MatrixX3f nn;         /**< Cartesian Vector of the triangle plane normal. */
    Eigen::VectorXf a;           /**< r12*r12. */
    Eigen::VectorXf b;           /**< r13*r13. */
    Eigen::VectorXf c;           /**< r12*r13. */
    Eigen::VectorXf det;         /**< Determinant of the Matrix [a c, c b]. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace MNELIB

#endif // MNELIB_MNEPROJECTTOSURFACE_H
