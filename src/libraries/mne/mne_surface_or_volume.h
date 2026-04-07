//=============================================================================================================
/**
 * @file     mne_surface_or_volume.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE FsSurface or Volume (MNESurfaceOrVolume) class declaration.
 *
 */

#ifndef MNESURFACEORVOLUME_H
#define MNESURFACEORVOLUME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include <mne/mne_types.h>
#include "mne_vol_geom.h"
#include "mne_mgh_tag_group.h"
#include "mne_triangle.h"
#include "mne_nearest.h"
#include "mne_patch_info.h"
#include <fiff/fiff_sparse_matrix.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <memory>
#include <optional>
#include <vector>

#include <QStringList>
#include <QDebug>


#define FIFF_MNE_SOURCE_SPACE_NNEIGHBORS    3594    /* Number of neighbors for each source space point (used for volume source spaces) */
#define FIFF_MNE_SOURCE_SPACE_NEIGHBORS     3595    /* Neighbors for each source space point (used for volume source spaces) */

#define FIFFV_MNE_COORD_SURFACE_RAS   FIFFV_COORD_MRI    /* The surface RAS coordinates */

#define TRIANGLE_FILE_MAGIC_NUMBER  (0xfffffe)
#define NEW_QUAD_FILE_MAGIC_NUMBER  (0xfffffd)
#define QUAD_FILE_MAGIC_NUMBER      (0xffffff)

#define TAG_OLD_SURF_GEOM           20

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffDigitizerData;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNESourceSpace;
class MNESurface;
class MNEMshDisplaySurface;
class MNEProjData;
class FilterThreadArg;

//=============================================================================================================
/**
 * @brief Defines a source space or a surface.
 *
 * Holds vertex positions, normals, triangulation, patch statistics,
 * neighborhood connectivity, and optional volume-grid metadata for
 * MEG/EEG source-space analysis.
 */
class MNESHARED_EXPORT MNESurfaceOrVolume
{
public:
    typedef QSharedPointer<MNESurfaceOrVolume> SPtr;              /**< Shared pointer type for MNESurfaceOrVolume. */
    typedef QSharedPointer<const MNESurfaceOrVolume> ConstSPtr;   /**< Const shared pointer type for MNESurfaceOrVolume. */

    /*
     * Eigen convenience types – row-major so that row(i).data() gives
     * a contiguous 3-element pointer, matching the old float** / int** layout.
     */
    typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> PointsT;     /**< Type abbreviation for np x 3 point data. */
    typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> NormalsT;    /**< Type abbreviation for np x 3 normal data. */
    typedef Eigen::Matrix<int,   Eigen::Dynamic, 3, Eigen::RowMajor> TrianglesT;  /**< Type abbreviation for ntri x 3 triangle indices. */

    //=========================================================================================================
    /**
     * @brief Constructs the MNE FsSurface or Volume.
     */
    MNESurfaceOrVolume();

    //=========================================================================================================
    /**
     * @brief Destroys the MNE FsSurface or Volume description.
     */
    virtual ~MNESurfaceOrVolume();

    MNESurfaceOrVolume(const MNESurfaceOrVolume&) = default;
    MNESurfaceOrVolume& operator=(const MNESurfaceOrVolume&) = default;
    MNESurfaceOrVolume(MNESurfaceOrVolume&&) = default;
    MNESurfaceOrVolume& operator=(MNESurfaceOrVolume&&) = default;

    /**
     * Compute the solid angle subtended by a triangle as seen from a given point
     * using van Oosterom's formula (scalar triple product of the edge vectors).
     *
     * @param[in] from      The observation point (3-element float array).
     * @param[in] tri       The triangle whose solid angle is computed.
     *
     * @return The solid angle in steradians.
     */
    static double solid_angle (const Eigen::Vector3f& from,	/* From this point... */
                               const MNELIB::MNETriangle& tri);

    /**
     * Set all vertex curvature values to 1.0 (uniform curvature),
     * if not already present.
     */
    void add_uniform_curv();

    /**
     * Populate the MNETriangle structures for both the full and in-use
     * triangulations by computing edge vectors, normals, and areas.
     * Accumulates the total surface area into tot_area.
     *
     * @param[in, out] s   The source space whose triangle data is filled in.
     */
    void add_triangle_data();


    /**
     * Compute the center of mass (mean position) of a set of points.
     *
     * @param[in]  rr   Array of 3D point coordinates.
     * @param[in]  np   Number of points.
     * @param[out] cm   Receives the center of mass (3-element float array).
     */
    static void compute_cm(const PointsT& rr, int np, float (&cm)[3]);

    /**
     * Compute and store the center of mass of a surface's vertices.
     *
     * @param[in, out] s   The surface whose cm field is set.
     */
    void compute_surface_cm();

    /**
     * Compute the Euclidean distances from each vertex to its topological
     * neighbors and store them in the vert_dist array.
     *
     * @param[in, out] s   The source space with neighbor_vert already set.
     */
    void calculate_vertex_distances();

    /**
     * Compute vertex normals by area-weighted accumulation of triangle
     * normals, then normalize to unit length. Also calls add_triangle_data()
     * and compute_surface_cm().
     *
     * @param[in, out] s   The source space to update.
     *
     * @return OK on success, FAIL on error.
     */
    int add_vertex_normals();

    /**
     * Build complete geometry information for a source space: triangle data,
     * vertex normals, neighbor-triangle lists, neighbor-vertex lists, vertex
     * distances, and center of mass. Optionally checks for topological
     * defects (excessive number of neighbors).
     *
     * @param[in, out] s                        The source space to augment.
     * @param[in]      do_normals               If non-zero, compute vertex normals.
     * @param[in]      check_too_many_neighbors If non-zero, fail on excessive neighbor count.
     *
     * @return OK on success, FAIL on error.
     */
    int add_geometry_info(bool do_normals, bool check_too_many_neighbors);

    /**
     * Convenience overload that adds geometry information with border detection
     * disabled and excess-neighbor checking enabled.
     *
     * @param[in, out] s            The source space to augment.
     * @param[in]      do_normals   If non-zero, compute vertex normals.
     *
     * @return OK on success, FAIL on error.
     */
    int add_geometry_info(bool do_normals);

    /**
     * Add geometry information with excess-neighbor checking disabled
     * (warns instead of failing when a vertex has too many neighbors).
     *
     * @param[in, out] s            The source space to augment.
     * @param[in]      do_normals   If non-zero, compute vertex normals.
     *
     * @return OK on success, FAIL on error.
     */
    int add_geometry_info2(bool do_normals);


    /**
     * Extract the nearest in-use vertex indices from the nearest vector.
     * Equivalent to the old MNEHemisphere::nearest (VectorXi).
     *
     * @return VectorXi where element i = nearest[i].nearest.
     */
    Eigen::VectorXi nearestVertIdx() const;

    /**
     * Extract the nearest distances from the nearest vector as doubles.
     * Equivalent to the old MNEHemisphere::nearest_dist (VectorXd).
     *
     * @return VectorXd where element i = (double)nearest[i].dist.
     */
    Eigen::VectorXd nearestDistVec() const;

    /**
     * Populate the nearest vector from separate index and distance arrays.
     * Sets vert = i, patch = nullptr for each entry.
     *
     * @param[in] nearestIdx   Nearest in-use vertex index for each vertex.
     * @param[in] nearestDist  Distance to nearest in-use vertex for each vertex.
     */
    void setNearestData(const Eigen::VectorXi& nearestIdx, const Eigen::VectorXd& nearestDist);


public:
    int             type;          /**< Is this a volume or a surface. */
    QString         subject;       /**< Name (id) of the subject. */
    int             id;            /**< FsSurface id. */
    int             coord_frame;   /**< Which coordinate system the data are in now. */
    /*
     * These relate to the FreeSurfer way
     */
    std::optional<MNEVolGeom> vol_geom;         /**< MRI volume geometry information as FreeSurfer likes it. */
    std::optional<MNEMghTagGroup> mgh_tags;     /**< Tags listed in the file. */
    /*
     * These are meaningful for both surfaces and volumes
     */
    int              np;        /**< Number of vertices. */
    PointsT          rr;        /**< The vertex locations (np x 3, row-major). */
    NormalsT         nn;        /**< FsSurface normals at these points (np x 3, row-major). */
    float            cm[3];     /**< Center of mass of the vertex cloud. */

    /** Return a read-only map to the k-th vertex position (3 contiguous floats). */
    Eigen::Map<const Eigen::Vector3f> point(int k) const { return Eigen::Map<const Eigen::Vector3f>(rr.row(k).data()); }
    /** Return a mutable map to the k-th vertex position. */
    Eigen::Map<Eigen::Vector3f> point(int k) { return Eigen::Map<Eigen::Vector3f>(rr.row(k).data()); }
    /** Return a read-only map to the k-th vertex normal (3 contiguous floats). */
    Eigen::Map<const Eigen::Vector3f> normal(int k) const { return Eigen::Map<const Eigen::Vector3f>(nn.row(k).data()); }

    Eigen::VectorXi   inuse;    /**< Boolean array indicating whether each vertex is in use in the source space (np elements). */
    Eigen::VectorXi   vertno;   /**< Vertex numbers of the used vertices in the full source space (nuse elements). */
    int              nuse;      /**< Number of vertices in use. */

    std::vector<Eigen::VectorXi> neighbor_vert; /**< Vertices neighboring each vertex (np entries, variable length). */
    Eigen::VectorXi   nneighbor_vert; /**< Number of vertices neighboring each vertex (np elements). */
    std::vector<Eigen::VectorXf> vert_dist;     /**< Euclidean distances between neighboring vertices (np entries, variable length). */
    /*
     * These are for surfaces only
     */
    float            sigma;     /**< Conductivity of a BEM compartment (-1 if not set). */

    int              ntri;      /**< Number of triangles in the surface. */
    std::vector<MNETriangle> tris; /**< The full triangulation data (ntri elements). */
    TrianglesT       itris;     /**< Triangle vertex indices (ntri x 3, row-major). */
    float            tot_area;  /**< Total area of the surface, computed from the triangles (m^2). */

    int              nuse_tri;      /**< Number of triangles in the in-use triangulation. */
    std::vector<MNETriangle> use_tris; /**< Triangulation data for the in-use vertices. */
    TrianglesT       use_itris;     /**< Vertex indices for the in-use triangulation (row-major). */

    std::vector<Eigen::VectorXi> neighbor_tri;    /**< Neighboring triangles for each vertex (np entries, variable length). */
    Eigen::VectorXi   nneighbor_tri;    /**< Number of neighboring triangles for each vertex (np elements). */

    std::vector<MNENearest> nearest; /**< Nearest in-use vertex info for each vertex (np elements). */
    std::vector<std::optional<MNEPatchInfo>> patches; /**< Patch information for each in-use vertex (nuse elements). */

    FIFFLIB::FiffSparseMatrix dist;         /**< Distances between (used) vertices along the surface. */
    float            dist_limit;    /**< Distance limit: values above this were not computed. Negative means only used vertices were considered. */

    Eigen::VectorXf   curv; /**< The FreeSurfer curvature values (np elements). */
    Eigen::VectorXf   val;  /**< Auxiliary values associated with the vertices (np elements). */
    /*
     * These are for volumes only
     */
    std::optional<FIFFLIB::FiffCoordTrans>  voxel_surf_RAS_t;   /**< Transform from voxel coordinates to surface RAS (MRI) coordinates. */
    int             vol_dims[3];        /**< Dimensions of the volume grid (width x height x depth). Present only for complete rectangular grids including unused vertices. */
    float           voxel_size[3];      /**< Voxel size in meters, derived from the voxel transform. */
    std::optional<FIFFLIB::FiffSparseMatrix> interpolator;       /**< Sparse matrix to interpolate from source space into an MRI volume. */
    QString         MRI_volume;         /**< Path to the MRI volume file the interpolator is based on. */
    std::optional<FIFFLIB::FiffCoordTrans>  MRI_voxel_surf_RAS_t; /**< Voxel-to-surface-RAS transform for the associated MRI volume. */
    std::optional<FIFFLIB::FiffCoordTrans>  MRI_surf_RAS_RAS_t; /**< Transform from surface RAS to scanner RAS in the associated MRI volume. */
    int             MRI_vol_dims[3];       /**< Dimensions of the associated MRI volume (width x height x depth). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNESURFACEORVOLUME_H
