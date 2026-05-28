//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     mne_surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2013
 * @brief    Lightweight triangulated surface (vertices, triangles, normals) used by surface-based routines.
 *
 * @ref MNELIB::MNESurface is the minimal geometric surface object that
 * the cortex-based algorithms in MNELIB and INVERSELIB share: it carries
 * no decimation or patch information and is used wherever only the
 * topology is needed (projection to surface, distance maps, head-shape
 * fitting). Heavier flavours - @ref MNESourceSpace and
 * @ref MNEBemSurface - subclass or wrap it.
 */

#ifndef MNE_SURFACE_H
#define MNE_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_surface_or_volume.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNEProjData;

//=============================================================================================================
/**
 * Implements the MNE FsSurface (Replaces typedef mneSurfaceOrVolume mneSurface; struct of MNE-C mne_types.h).
 *
 * @brief Lightweight triangulated surface (vertices, triangles, normals).
 */
class MNESHARED_EXPORT MNESurface : public MNESurfaceOrVolume
{
public:
    typedef std::shared_ptr<MNESurface> SPtr;              /**< Shared pointer type for MNESurface. */
    typedef std::shared_ptr<const MNESurface> ConstSPtr;   /**< Const shared pointer type for MNESurface. */
    typedef std::unique_ptr<MNESurface> UPtr;              /**< Unique pointer type for MNESurface. */

    //=========================================================================================================
    /**
     * Constructs the MNE FsSurface
     */
    MNESurface();

    //=========================================================================================================
    /**
     * Destroys the MNE FsSurface
     */
    ~MNESurface();

    //=========================================================================================================
    // FsSurface geometry const methods
    //=========================================================================================================

    /**
     * Sum the solid angles of all triangles as seen from a given point.
     * ~4pi if inside, ~0 if outside.
     *
     * @param[in] from   The test point.
     * @return Total solid angle in steradians.
     */
    double sum_solids(const Eigen::Vector3f& from) const;

    /**
     * Compute barycentric-like coordinates of a point relative to a triangle.
     *
     * @param[in]  r     The point.
     * @param[in]  tri   Triangle index.
     * @param[out] x     Barycentric coordinate along first edge.
     * @param[out] y     Barycentric coordinate along second edge.
     * @param[out] z     Signed perpendicular distance from the plane.
     */
    void triangle_coords(const Eigen::Vector3f& r, int tri,
                          float &x, float &y, float &z) const;

    /**
     * Find the nearest point on a triangle to a given point.
     *
     * @param[in]  r     The point.
     * @param[in]  user  Optional MNEProjData restricting active triangles (may be NULL).
     * @param[in]  tri   Triangle index.
     * @param[out] x     Coordinate along first edge.
     * @param[out] y     Coordinate along second edge.
     * @param[out] z     Distance from the triangle.
     * @return TRUE if the triangle is active, FALSE if inactive.
     */
    int nearest_triangle_point(const Eigen::Vector3f& r,
                               const MNEProjData *user,
                               int tri,
                               float &x, float &y, float &z) const;

    /**
     * Find the nearest point on a triangle (simplified, no projection data).
     *
     * @param[in]  r     The point.
     * @param[in]  tri   Triangle index.
     * @param[out] x     Coordinate along first edge.
     * @param[out] y     Coordinate along second edge.
     * @param[out] z     Distance from the triangle.
     * @return TRUE always.
     */
    int nearest_triangle_point(const Eigen::Vector3f& r,
                               int tri,
                               float &x, float &y, float &z) const;

    /**
     * Compute 3D position on a triangle from barycentric coordinates.
     *
     * @param[in] tri   Triangle index.
     * @param[in] p     Barycentric coordinate along first edge.
     * @param[in] q     Barycentric coordinate along second edge.
     * @return The 3D point on the triangle.
     */
    Eigen::Vector3f project_to_triangle(int tri, float p, float q) const;

    /**
     * Find the nearest point on a given triangle and return the projected 3D coordinates.
     *
     * @param[in] best   Triangle index.
     * @param[in] r      The source point.
     * @return The projected 3D point.
     */
    Eigen::Vector3f project_to_triangle(int best, const Eigen::Vector3f& r) const;

    /**
     * Project a point onto the nearest triangle of the surface.
     *
     * @param[in]  proj_data  Optional MNEProjData restricting active triangles (may be NULL).
     * @param[in]  r          The 3D point to project.
     * @param[out] distp      Receives the signed distance to the surface.
     * @return Index of the closest triangle, or -1 if none found.
     */
    int project_to_surface(const MNEProjData *proj_data,
                           const Eigen::Vector3f& r,
                           float &distp) const;

    /**
     * For each point, find the closest point on the surface using
     * neighborhood-restricted search.
     *
     * @param[in]      r            Array of np point coordinates.
     * @param[in]      np           Number of points.
     * @param[in,out]  nearest_tri  Best triangle index for each point.
     * @param[out]     dist         Distance to the surface for each point.
     * @param[in]      nstep        Number of neighborhood expansion steps.
     */
    void find_closest_on_surface_approx(const PointsT& r, int np,
                                        Eigen::VectorXi& nearest_tri,
                                        Eigen::VectorXf& dist,
                                        int nstep) const;

    /**
     * Set up the triangle activation mask for a restricted surface search.
     *
     * @param[in,out]  p            The projection data whose mask is set.
     * @param[in]      approx_best  Approximate best triangle, or negative for brute-force.
     * @param[in]      nstep        Neighborhood expansion levels.
     * @param[in]      r            The query point.
     */
    void decide_search_restriction(MNEProjData& p,
                                   int approx_best,
                                   int nstep,
                                   const Eigen::Vector3f& r) const;

    /**
     * Recursively mark neighboring triangles as active.
     *
     * @param[in]     start   Starting vertex index.
     * @param[in,out] act     Triangle activation array.
     * @param[in]     nstep   Recursive expansion steps.
     */
    void activate_neighbors(int start, Eigen::VectorXi &act, int nstep) const;

    //=========================================================================================================
    // Non-const mutator
    //=========================================================================================================

    //=========================================================================================================
    // Static factory methods
    //=========================================================================================================

    /**
     * Read a BEM surface from a FIFF file (excess-neighbor checking enabled).
     *
     * @param[in]  name           Path to the BEM FIFF file.
     * @param[in]  which          FsSurface ID to load (-1 loads the first found).
     * @param[in]  add_geometry   If true, compute full geometry info.
     * @return The loaded surface, or nullptr on failure. Caller takes ownership.
     */
    static std::unique_ptr<MNESurface> read_bem_surface(const QString& name, int which,
                                        bool add_geometry);

    /**
     * Read a BEM surface from a FIFF file (excess-neighbor checking enabled).
     *
     * @param[in]  name           Path to the BEM FIFF file.
     * @param[in]  which          FsSurface ID to load (-1 loads the first found).
     * @param[in]  add_geometry   If true, compute full geometry info.
     * @param[out] sigma          Receives the surface conductivity.
     * @return The loaded surface, or nullptr on failure. Caller takes ownership.
     */
    static std::unique_ptr<MNESurface> read_bem_surface(const QString& name, int which,
                                        bool add_geometry, float& sigma);

    /**
     * Read a BEM surface from a FIFF file (excess-neighbor checking disabled).
     *
     * @param[in]  name           Path to the BEM FIFF file.
     * @param[in]  which          FsSurface ID to load (-1 loads the first found).
     * @param[in]  add_geometry   If true, compute full geometry info.
     * @return The loaded surface, or nullptr on failure. Caller takes ownership.
     */
    static std::unique_ptr<MNESurface> read_bem_surface2(const QString& name, int which,
                                         bool add_geometry);

    /**
     * Read a BEM surface from a FIFF file.
     *
     * @param[in]  name                      Path to the BEM FIFF file.
     * @param[in]  which                     FsSurface ID to load (-1 loads the first found).
     * @param[in]  add_geometry              If true, compute full geometry info.
     * @param[out] sigma                     Receives the surface conductivity.
     * @param[in]  check_too_many_neighbors  Fail on excess neighbor count.
     * @return The loaded surface, or nullptr on failure. Caller takes ownership.
     */
    static std::unique_ptr<MNESurface> read_bem_surface(const QString& name, int which,
                                        bool add_geometry, float& sigma,
                                        bool check_too_many_neighbors);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNE_SURFACE_H
