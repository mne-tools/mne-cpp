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
 * @brief    MNE Surface or Volume (MneSurfaceOrVolume) class declaration.
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QFile>
#include <QTextStream>

#include <memory>
#include <vector>

#include <QStringList>
#include <QDebug>

namespace FIFFLIB { class FiffCoordTrans; }

//============================= mne_fiff.h =============================

#define FIFF_MNE_SOURCE_SPACE_NNEIGHBORS    3594    /* Number of neighbors for each source space point (used for volume source spaces) */
#define FIFF_MNE_SOURCE_SPACE_NEIGHBORS     3595    /* Neighbors for each source space point (used for volume source spaces) */

#define FIFFV_MNE_COORD_SURFACE_RAS   FIFFV_COORD_MRI    /* The surface RAS coordinates */

//============================= mne_surface_io.c =============================

#define TRIANGLE_FILE_MAGIC_NUMBER  (0xfffffe)
#define NEW_QUAD_FILE_MAGIC_NUMBER  (0xfffffd)
#define QUAD_FILE_MAGIC_NUMBER      (0xffffff)

//============================= mne_mgh_mri_io.c =============================

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

class MneTriangle;
class MneNearest;
class MnePatchInfo;
class MneSourceSpaceOld;
class MneSurfaceOld;
class MneMshDisplaySurface;
class MneProjData;

//=============================================================================================================
/**
 * Implements MNE Surface or Volume (Replaces *mneSurfaceOrVolume,mneSurfaceOrVolumeRec; struct of MNE-C mne_types.h).
 *
 * @brief This defines a source space or a surface
 */
class MNESHARED_EXPORT MneSurfaceOrVolume
{
public:
    typedef QSharedPointer<MneSurfaceOrVolume> SPtr;              /**< Shared pointer type for MneSurfaceOrVolume. */
    typedef QSharedPointer<const MneSurfaceOrVolume> ConstSPtr;   /**< Const shared pointer type for MneSurfaceOrVolume. */

    /*
     * Eigen convenience types – row-major so that row(i).data() gives
     * a contiguous 3-element pointer, matching the old float** / int** layout.
     */
    typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> PointsT;     /**< Type abbreviation for np x 3 point data. */
    typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> NormalsT;    /**< Type abbreviation for np x 3 normal data. */
    typedef Eigen::Matrix<int,   Eigen::Dynamic, 3, Eigen::RowMajor> TrianglesT;  /**< Type abbreviation for ntri x 3 triangle indices. */

    /*
     * These are the aliases
     */
//    typedef MneSurfaceOrVolume MneCSourceSpace; //TODO create a derived class
    //typedef mneSurfaceOrVolume mneSourceVolume;
//    typedef MneSurfaceOrVolume MneCSurface; //TODO create a derived class
    //typedef mneSurfaceOrVolume mneVolume;

    //=========================================================================================================
    /**
     * Constructs the MNE Surface or Volume
     */
    MneSurfaceOrVolume();

    //=========================================================================================================
    /**
     * Destroys the MNE Surface or Volume description
     * Refactored: mne_free_source_space (mne_source_space.c)
     */
    virtual ~MneSurfaceOrVolume();

    //============================= make_filter_source_sapces.c =============================

    /**
     * Compute the solid angle subtended by a triangle as seen from a given point
     * using van Oosterom's formula (scalar triple product of the edge vectors).
     *
     * @param[in] from      The observation point (3-element float array).
     * @param[in] tri       The triangle whose solid angle is computed.
     *
     * @return The solid angle in steradians.
     */
    static double solid_angle (float       *from,	/* From this point... */
                               MNELIB::MneTriangle* tri);

    /**
     * Sum the solid angles of all triangles in a closed surface as seen from
     * a given point. A total of approximately \f$4\pi\f$ indicates that the
     * point is inside the surface; approximately zero means outside.
     *
     * @param[in] from      The test point (3-element float array).
     * @param[in] surf      The closed surface to test against.
     *
     * @return The total solid angle in steradians.
     */
    static double sum_solids(float *from, MneSurfaceOld* surf);

    /**
     * Filter source space points by removing vertices that lie outside the
     * bounding surface (solid-angle test) or are closer than @p limit to
     * the nearest surface vertex.
     *
     * This overload performs filtering sequentially on all source spaces.
     *
     * @param[in]      surf         The bounding (inner skull) surface.
     * @param[in]      limit        Minimum allowed distance from the surface (m).
     * @param[in]      mri_head_t   MRI-to-head coordinate transformation.
     * @param[in, out] spaces       Array of source spaces to filter.
     * @param[in]      nspace       Number of source spaces in the array.
     * @param[in, out] filtered     If non-null, the coordinates of removed
     *                              vertices are written to this stream.
     *
     * @return OK on success, FAIL on error.
     */
    static int filter_source_spaces(MneSurfaceOld* surf,  /* The bounding surface must be provided */
                                        float limit,                                   /* Minimum allowed distance from the surface */
                                        const FIFFLIB::FiffCoordTrans& mri_head_t,     /* Coordinate transformation (may not be needed) */
                                        MneSourceSpaceOld* *spaces,  /* The source spaces  */
                                        int nspace,
                                        QTextStream *filtered);

    //============================= mne_patches.c =============================

    /**
     * Compute patch statistics for a source space: group vertices by their
     * nearest active source, create MnePatchInfo objects storing member
     * vertices, area, average normal, and normal deviation for each patch.
     *
     * @param[in, out] s   The source space to add patch statistics to.
     *
     * @return OK on success, FAIL on error.
     */
    static int add_patch_stats(MneSourceSpaceOld* s);

    //============================= filter_source_sapces.c =============================

    /**
     * Recount in-use vertices, rebuild the vertex-number array, and
     * optionally recompute patch statistics after source space filtering.
     *
     * @param[in, out] s   The source space to rearrange.
     */
    static void rearrange_source_space(MneSourceSpaceOld* s);

    /**
     * Thread entry point for source space filtering. Performs the same
     * inside/outside solid-angle check and minimum-distance test as
     * filter_source_spaces(), designed to be called via QtConcurrent.
     *
     * @param[in, out] arg   A FilterThreadArg pointer containing the source
     *                       space, bounding surface, distance limit, and
     *                       coordinate transform.
     *
     * @return Always returns NULL.
     */
    static void *filter_source_space(void *arg);

    /**
     * Filter source space points by loading the inner skull BEM surface
     * from a file, then filtering each source space (optionally in parallel
     * via threads) against it.
     *
     * @param[in]      limit        Minimum allowed distance from the inner skull (m).
     * @param[in]      bemfile      Path to the BEM file containing the inner skull surface.
     * @param[in]      mri_head_t   MRI-to-head coordinate transformation.
     * @param[in, out] spaces       Array of source spaces to filter.
     * @param[in]      nspace       Number of source spaces in the array.
     * @param[in, out] filtered     If non-null, the coordinates of removed
     *                              vertices are written to this stream.
     * @param[in]      use_threads  Whether to use multi-threaded filtering.
     *
     * @return OK on success, FAIL on error.
     */
    static int filter_source_spaces(float          limit,              /* Omit vertices which are closer than this to the inner skull */
                             char           *bemfile,                       /* Take the inner skull surface from here */
                             const FIFFLIB::FiffCoordTrans& mri_head_t,                 /* Coordinate transformation is needed */
                             MneSourceSpaceOld* *spaces,  /* The source spaces */
                             int            nspace,                         /* How many? */
                             QTextStream    *filtered,                      /* Output the coordinates of the filtered points here */
                             bool           use_threads);

    //============================= make_volume_source_space.c =============================

    /**
     * Create a volumetric source space by laying out a 3D grid of points
     * within the bounding box of a surface, filtering out points that lie
     * outside the surface or closer than @p mindist to the surface, and
     * establishing 26-neighbor connectivity.
     *
     * @param[in] surf      The bounding surface (e.g. inner skull).
     * @param[in] grid      Grid spacing in meters.
     * @param[in] exclude   Exclusion radius from the center of mass (m).
     * @param[in] mindist   Minimum distance from the surface (m).
     *
     * @return A new volumetric source space, or NULL on failure. Caller takes ownership.
     */
    static MneSourceSpaceOld* make_volume_source_space(MneSurfaceOld* surf,
                                                                         float grid,
                                                                         float exclude,
                                                                         float mindist);

    //============================= mne_source_space.c =============================

    /**
     * Allocate and zero-initialize a new source space with @p np vertices.
     * Sets the default coordinate frame to MRI and type to surface.
     *
     * @param[in] np   Number of vertices in the source space.
     *
     * @return A newly allocated source space. Caller takes ownership.
     */
    static MneSourceSpaceOld* create_source_space(int np);

    //============================= mne_bem_surface_io.c =============================

    /**
     * Read a BEM surface from a FIFF file with excess-neighbor checking enabled.
     *
     * Convenience overload that delegates to the five-argument version with
     * @c check_too_many_neighbors set to @c true.
     *
     * @param[in]  name           Path to the BEM FIFF file.
     * @param[in]  which          Surface ID to load (-1 loads the first found).
     * @param[in]  add_geometry   If non-zero, compute full geometry information.
     * @param[out] sigmap         If non-null, receives the surface conductivity.
     *
     * @return The loaded surface, or NULL on failure. Caller takes ownership.
     */
    //Refactored: mne_read_bem_surface (mne_bem_surface_io.c)
    static MneSurfaceOld* read_bem_surface(const QString&  name,   /* Filename */
                                         int  which,             /* Which surface are we looking for (-1 loads the first one)*/
                                         int  add_geometry,      /* Add the geometry information */
                                         float *sigmap);

    /**
     * Read a BEM surface from a FIFF file with excess-neighbor checking disabled.
     *
     * @param[in]  name           Path to the BEM FIFF file (C string).
     * @param[in]  which          Surface ID to load (-1 loads the first found).
     * @param[in]  add_geometry   If non-zero, compute full geometry information.
     * @param[out] sigmap         If non-null, receives the surface conductivity.
     *
     * @return The loaded surface, or NULL on failure. Caller takes ownership.
     */
    static MneSurfaceOld* read_bem_surface2(char *name,	        /* Filename */
                                     int  which,		/* Which surface are we looking for (-1 loads the first one)*/
                                     int  add_geometry,	/* Add the geometry information */
                                     float *sigmap);		/* Conductivity? */

    /**
     * Read a BEM surface from a FIFF file.
     *
     * Opens the file, locates the FIFFB_BEM_SURF block matching @p which,
     * reads vertices, normals, triangles, coordinate frame, and conductivity,
     * and optionally adds full geometry information or vertex normals.
     *
     * @param[in]  name                      Path to the BEM FIFF file.
     * @param[in]  which                     Surface ID to load (-1 loads the first found).
     * @param[in]  add_geometry              If non-zero, compute full geometry information.
     * @param[out] sigmap                    If non-null, receives the surface conductivity.
     * @param[in]  check_too_many_neighbors  Fail if a vertex has an excessive number
     *                                       of neighbors (topological defect check).
     *
     * @return The loaded surface, or NULL on failure. Caller takes ownership.
     */
    //Refactored: read_bem_surface (mne_bem_surface_io.c)
    static MneSurfaceOld* read_bem_surface( const QString& name,    /* Filename */
                                          int  which,             /* Which surface are we looking for (-1 loads the first one)*/
                                          int  add_geometry,      /* Add the geometry information */
                                          float *sigmap,          /* Conductivity? */
                                          bool   check_too_many_neighbors);

    //============================= project_to_surface.c =============================

    /**
     * Compute the barycentric-like coordinates (x, y) and perpendicular
     * distance z of a point relative to a triangle on a surface.
     *
     * @param[in]  r     The point coordinates (3-element float array).
     * @param[in]  s     The surface containing the triangle.
     * @param[in]  tri   Index of the triangle.
     * @param[out] x     Barycentric coordinate along the first edge.
     * @param[out] y     Barycentric coordinate along the second edge.
     * @param[out] z     Signed perpendicular distance from the triangle plane.
     */
    static void triangle_coords(float       *r,       /* Location of a point */
                                    MneSurfaceOld*  s,	       /* The surface */
                                    int         tri,      /* Which triangle */
                                    float       *x,       /* Coordinates of the point on the triangle */
                                    float       *y,
                                    float       *z);

    /**
     * Find the nearest point on a specific triangle to a given point.
     *
     * Projects the point onto the triangle plane; if the projection lies
     * inside the triangle, returns those coordinates. Otherwise, checks all
     * three edges and selects the closest clamped point.
     *
     * @param[in]  r     The point coordinates (3-element float array).
     * @param[in]  s     The surface containing the triangle.
     * @param[in]  user  Optional MneProjData restricting which triangles are active (may be NULL).
     * @param[in]  tri   Index of the triangle.
     * @param[out] x     Barycentric coordinate along the first edge.
     * @param[out] y     Barycentric coordinate along the second edge.
     * @param[out] z     Signed distance from the triangle plane (absolute value = Euclidean distance).
     *
     * @return TRUE if the triangle is active (or no projection data supplied), FALSE if inactive.
     */
    static int nearest_triangle_point(float       *r,    /* Location of a point */
                                      MneSurfaceOld*  s,     /* The surface */
                                      void        *user, /* Something precomputed */
                                      int         tri,   /* Which triangle */
                                      float       *x,    /* Coordinates of the point on the triangle */
                                      float       *y,
                                      float       *z);

    /**
     * Compute the 3D position on a triangle from barycentric coordinates:
     * \f$ r = r_1 + p \cdot r_{12} + q \cdot r_{13} \f$.
     *
     * @param[in]  s     The surface containing the triangle.
     * @param[in]  tri   Index of the triangle.
     * @param[in]  p     Barycentric coordinate along the first edge.
     * @param[in]  q     Barycentric coordinate along the second edge.
     * @param[out] r     The resulting 3D point (3-element float array).
     */
    static void project_to_triangle(MneSurfaceOld* s,
                                    int        tri,
                                    float      p,
                                    float      q,
                                    float      *r);

    /**
     * Find the nearest point on a triangle to a given point (simplified
     * wrapper without projection data).
     *
     * @param[in]  r     The point coordinates (3-element float array).
     * @param[in]  s     The surface containing the triangle.
     * @param[in]  tri   Index of the triangle.
     * @param[out] x     Barycentric coordinate along the first edge.
     * @param[out] y     Barycentric coordinate along the second edge.
     * @param[out] z     Signed distance from the triangle plane.
     *
     * @return TRUE (always, since no activation data is used).
     */
    static int nearest_triangle_point(float       *r,    /* Location of a point */
                                          MneSurfaceOld*  s,     /* The surface */
                                          int         tri,   /* Which triangle */
                                          float       *x,    /* Coordinates of the point on the triangle */
                                          float       *y,
                                          float       *z);

    /**
     * Project a point onto the nearest triangle of a surface.
     *
     * Iterates over all (active) triangles to find the one whose nearest
     * point has the smallest absolute distance, and optionally projects
     * the point onto that triangle.
     *
     * @param[in]      s           The target surface.
     * @param[in]      proj_data   Optional MneProjData restricting active triangles (may be NULL).
     * @param[in, out] r           The point to project (modified in-place if @p project_it is set).
     * @param[in]      project_it  If non-zero, move @p r to the closest surface point.
     * @param[out]     distp       If non-null, receives the signed distance to the surface.
     *
     * @return Index of the best (closest) triangle, or -1 if none found.
     */
    static int project_to_surface(MneSurfaceOld* s, void *proj_data, float *r, int project_it, float *distp);

    /**
     * Find the nearest point on a specific triangle to a given point and
     * write the projected coordinates into @p proj.
     *
     * @param[in]  s     The surface containing the triangle.
     * @param[in]  best  Index of the triangle.
     * @param[in]  r     The source point (3-element float array).
     * @param[out] proj  The projected 3D point (3-element float array).
     */
    static void project_to_triangle(MneSurfaceOld* s,
                                            int        best,
                                            float      *r,
                                            float      *proj);

    /**
     * For each point in @p r, find the closest point on the surface using a
     * neighborhood-restricted search (expanding @p nstep levels from a prior
     * approximate best triangle). Falls back to an unrestricted search when
     * the restricted one fails.
     *
     * @param[in]      s        The target surface.
     * @param[in, out] r        Array of np point coordinates.
     * @param[in]      np       Number of points.
     * @param[out]     nearest  Best triangle index for each point.
     * @param[out]     dist     Distance to the surface for each point.
     * @param[in]      nstep    Number of neighborhood expansion steps.
     */
    static void find_closest_on_surface_approx(MneSurfaceOld* s, float **r, int np, int *nearest, float *dist, int nstep);

    /**
     * Set up the triangle activation mask for a restricted surface search.
     *
     * If @p approx_best is negative, finds the closest vertex by brute force;
     * otherwise, uses the vertices of the approximate best triangle. Then
     * activates all triangles within @p nstep hops.
     *
     * @param[in]      s            The target surface.
     * @param[in, out] p            The projection data whose activation mask is set.
     * @param[in]      approx_best  Approximate best triangle index, or negative for brute-force.
     * @param[in]      nstep        Number of neighborhood expansion levels.
     * @param[in]      r            The query point (3-element float array).
     */
    static void decide_search_restriction(MneSurfaceOld* s,
                          MneProjData*   p,
                          int        approx_best, /* We know the best triangle approximately
                                       * already */
                          int        nstep,
                          float      *r);

    /**
     * Recursively mark all triangles neighboring the starting vertex as
     * active in the activation mask, expanding @p nstep levels through
     * the vertex adjacency graph.
     *
     * @param[in]      s       The surface whose adjacency data is used.
     * @param[in]      start   Starting vertex index.
     * @param[in, out] act     Triangle activation array (ntri elements).
     * @param[in]      nstep   Number of recursive expansion steps.
     */
    static void activate_neighbors(MneSurfaceOld* s, int start, Eigen::VectorXi &act, int nstep);

    //============================= mne_source_space.c =============================

    /**
     * Read source spaces from a FIFF file.
     *
     * Locates all FIFFB_MNE_SOURCE_SPACE blocks and reads vertices, normals,
     * coordinate frame, triangulations, in-use selection, patch info, distance
     * matrices, volume neighborhoods, voxel transforms, and MRI metadata.
     *
     * @param[in]  name     Path to the FIFF file.
     * @param[out] spacesp  Receives the array of loaded source spaces. Caller takes ownership.
     * @param[out] nspacep  Receives the number of source spaces loaded.
     *
     * @return FIFF_OK on success, FIFF_FAIL on error.
     */
    static int read_source_spaces(const QString& name,               /* Read from here */
                                      MneSourceSpaceOld* **spacesp, /* These are the results */
                                      int            *nspacep);

    /**
     * Replace the in-use array of a source space, recount the number of
     * active vertices, and rebuild the vertex-number index array.
     *
     * @param[in, out] s          The source space to update.
     * @param[in]      new_inuse  New boolean array of length s->np indicating which vertices are in use.
     */
    static void update_inuse(MneSourceSpaceOld* s,
                                              int *new_inuse);

    /**
     * Determine whether a source space belongs to the left hemisphere by
     * computing the mean x-coordinate of all vertices. A negative mean
     * indicates the left hemisphere.
     *
     * @param[in] s   The source space to test.
     *
     * @return TRUE if left hemisphere, FALSE otherwise.
     */
    static int is_left_hemi(MneSourceSpaceOld* s);

    /**
     * Apply a coordinate transformation to all vertex positions, vertex
     * normals, and triangle normals of a source space, and update the
     * stored coordinate frame.
     *
     * @param[in, out] ss   The source space to transform.
     * @param[in]      t    The coordinate transformation to apply.
     *
     * @return OK on success, FAIL on error.
     */
    static int transform_source_space(MneSourceSpaceOld* ss, const FIFFLIB::FiffCoordTrans& t);

    /**
     * Transform an array of source spaces to the specified coordinate frame,
     * automatically selecting the forward or inverse transformation direction
     * based on the current frame of each space.
     *
     * @param[in]      coord_frame  Target coordinate frame.
     * @param[in]      t            The coordinate transformation.
     * @param[in, out] spaces       Array of source spaces to transform.
     * @param[in]      nspace       Number of source spaces.
     *
     * @return OK on success, FAIL on error.
     */
    static int transform_source_spaces_to(int            coord_frame,   /* Which coord frame do we want? */
                                              const FIFFLIB::FiffCoordTrans& t,             /* The coordinate transformation */
                                              MneSourceSpaceOld* *spaces,       /* A list of source spaces */
                                              int            nspace);

    //============================= mne_forward_solution.c =============================

    /**
     * Mark all vertices in a source space as in-use, setting nuse equal to np.
     *
     * @param[in, out] s   The source space to modify.
     */
    static void enable_all_sources(MneSourceSpaceOld* s);

    //============================= resrict_sources.c =============================

    /**
     * Restrict source spaces to only those vertices contained in the given
     * FreeSurfer label files. Hemisphere assignment is inferred from the
     * label filename ("-lh.label" vs "-rh.label").
     *
     * @param[in, out] spaces   Array of source spaces (typically left and right hemisphere).
     * @param[in]      nspace   Number of source spaces.
     * @param[in]      labels   List of label file paths.
     * @param[in]      nlabel   Number of label files.
     *
     * @return OK on success, FAIL on error.
     */
    static int restrict_sources_to_labels(MneSourceSpaceOld* *spaces,
                                          int            nspace,
                                          const QStringList& labels,
                                          int            nlabel);

    //============================= mne_labels.c =============================
    //TODO Move to separate label class

    /**
     * Find source space vertices that fall within a FreeSurfer label.
     *
     * For each labeled vertex that is in-use in the source space, computes
     * its sequential index among in-use vertices (plus @p off) and collects
     * these into the output selection array.
     *
     * @param[in]  label   Path to the FreeSurfer label file.
     * @param[in]  s       The source space hemisphere containing the label's vertices.
     * @param[in]  off     Offset added to each sequential index (for combining hemispheres).
     * @param[out] selp    Receives the array of selected source indices. Caller takes ownership.
     * @param[out] nselp   Receives the number of selected sources.
     *
     * @return OK on success, FAIL on error.
     */
    static int find_sources_in_label(char *label,          /* The label file */
                                         MneSourceSpaceOld* s,	    /* The corresponding source space hemisphere */
                                         int  off,		    /* Offset to the complete source space */
                                         int  **selp,	    /* Sources selected */
                                         int  *nselp);

    /**
     * Read a FreeSurfer label file.
     *
     * Parses the standard format: an optional comment line starting with '#',
     * a vertex count, then lines of "vertex x y z value".
     *
     * @param[in]  label     Path to the label file.
     * @param[out] commentp  If non-null, receives the comment string from the file.
     * @param[out] selp      Receives the array of vertex indices. Caller takes ownership.
     * @param[out] nselp     Receives the number of vertices in the label.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_label(const QString& label,	    /* The label file */
                              char **commentp, /* The comment in the file */
                              int  **selp,	    /* Points in label */
                              int  *nselp);

    /**
     * Write a FreeSurfer label file.
     *
     * Writes the standard format: a comment header line, vertex count,
     * then "vertex x y z 0.0" for each selected vertex (coordinates in mm).
     *
     * @param[in] label    Path to the output label file.
     * @param[in] comment  Comment string to include in the header.
     * @param[in] sel      Array of vertex indices to write.
     * @param[in] nsel     Number of vertices.
     * @param[in] rr       Vertex coordinates (may be NULL; if so, zeros are written).
     *
     * @return OK on success, FAIL on error.
     */
    static int write_label(char *label,    /* The label file */
                               char *comment,
                               int  *sel,	    /* Points in label */
                               int  nsel,	    /* How many? */
                               float **rr);

    /**
     * Compute the cortical surface area covered by a FreeSurfer label.
     *
     * For each labeled vertex, one-third of the area of every neighboring
     * triangle is accumulated (each triangle is shared by three vertices).
     *
     * @param[in]  label   Path to the FreeSurfer label file.
     * @param[in]  s       The source space with triangle data.
     * @param[out] areap   Receives the label area in square meters.
     *
     * @return OK on success, FAIL on error.
     */
    static int label_area(char *label,      /* The label file */
                              MneSourceSpaceOld* s, /* The associated source space */
                              float *areap);

    //============================= mne_add_geometry_info.c =============================

    /**
     * Populate the MneTriangle structures for both the full and in-use
     * triangulations by computing edge vectors, normals, and areas.
     * Accumulates the total surface area into tot_area.
     *
     * @param[in, out] s   The source space whose triangle data is filled in.
     */
    static void add_triangle_data(MneSourceSpaceOld* s);

    //============================= mne_add_geometry_info.c =============================

    /**
     * Compute the center of mass (mean position) of a set of points.
     *
     * @param[in]  rr   Array of 3D point coordinates.
     * @param[in]  np   Number of points.
     * @param[out] cm   Receives the center of mass (3-element float array).
     */
    static void compute_cm(const PointsT& rr, int np, float *cm);

    /**
     * Compute and store the center of mass of a surface's vertices.
     *
     * @param[in, out] s   The surface whose cm field is set.
     */
    static void compute_surface_cm(MneSurfaceOld* s);

    /**
     * Compute the Euclidean distances from each vertex to its topological
     * neighbors and store them in the vert_dist array.
     *
     * @param[in, out] s   The source space with neighbor_vert already set.
     */
    static void calculate_vertex_distances(MneSourceSpaceOld* s);

    /**
     * Compute vertex normals by area-weighted accumulation of triangle
     * normals, then normalize to unit length. Also calls add_triangle_data()
     * and compute_surface_cm().
     *
     * @param[in, out] s   The source space to update.
     *
     * @return OK on success, FAIL on error.
     */
    static int add_vertex_normals(MneSourceSpaceOld* s);

    /**
     * Build complete geometry information for a source space: triangle data,
     * vertex normals, neighbor-triangle lists, neighbor-vertex lists, vertex
     * distances, and center of mass. Optionally checks for topological
     * defects (excessive number of neighbors).
     *
     * @param[in, out] s                        The source space to augment.
     * @param[in]      do_normals               If non-zero, compute vertex normals.
     * @param[out]     border                   If non-null, receives the array of border
     *                                          vertex flags (vertices touching a topological boundary).
     * @param[in]      check_too_many_neighbors If non-zero, fail on excessive neighbor count.
     *
     * @return OK on success, FAIL on error.
     */
    static int add_geometry_info(MneSourceSpaceOld* s, int do_normals, int *border, int check_too_many_neighbors);

    /**
     * Convenience overload that adds geometry information with border detection
     * disabled and excess-neighbor checking enabled.
     *
     * @param[in, out] s            The source space to augment.
     * @param[in]      do_normals   If non-zero, compute vertex normals.
     *
     * @return OK on success, FAIL on error.
     */
    static int add_geometry_info(MneSourceSpaceOld* s, int do_normals);

    /**
     * Add geometry information with excess-neighbor checking disabled
     * (warns instead of failing when a vertex has too many neighbors).
     *
     * @param[in, out] s            The source space to augment.
     * @param[in]      do_normals   If non-zero, compute vertex normals.
     *
     * @return OK on success, FAIL on error.
     */
    static int add_geometry_info2(MneSourceSpaceOld* s, int do_normals);

    //============================= digitizer.c =============================

    /**
     * Align MEG and MRI coordinate systems using fiducial points and ICP.
     *
     * Extracts LPA/Nasion/RPA fiducials from both digitizer datasets,
     * optionally scales the head surface, computes an initial rigid-body
     * alignment from the cardinal points, discards outlier digitizer points,
     * and runs @p niter ICP iterations to refine the alignment.
     *
     * @param[in, out] head_dig    MEG digitizer data (transformed in-place).
     * @param[in]      mri_dig     MRI digitizer data with fiducial locations.
     * @param[in]      head_surf   The scalp surface used for ICP.
     * @param[in]      niter       Number of ICP iterations.
     * @param[in]      scale_head  If non-zero, scale the head surface to match digitizer.
     * @param[in]      omit_dist   Discard digitizer points farther than this from the surface (m).
     * @param[out]     scales      If non-null, receives the per-axis scale factors applied.
     *
     * @return OK on success, FAIL on error.
     */
    static int align_fiducials(FIFFLIB::FiffDigitizerData* head_dig,
                               FIFFLIB::FiffDigitizerData* mri_dig,
                               MneMshDisplaySurface* head_surf,
                               int niter,
                               int scale_head,
                               float omit_dist,
                               float *scales);

    /**
     * Compute a uniform head scale factor by fitting spheres to the
     * digitizer points and to the scalp surface, then taking the ratio
     * of the two radii.
     *
     * @param[in]  dig        The digitizer data (points above the fiducial plane are used).
     * @param[in]  mri_fid    MRI fiducial coordinates (LPA, Nasion, RPA).
     * @param[in]  head_surf  The scalp surface.
     * @param[out] scales     Receives three identical scale factors (dig_radius / surface_radius).
     */
    static void get_head_scale(FIFFLIB::FiffDigitizerData* dig,
                                   float **mri_fid,
                                   MneMshDisplaySurface* head_surf,
                                   float *scales);

    /**
     * Mark digitizer points whose distance to the head surface exceeds
     * @p maxdist as discarded. Cardinal landmarks and HPI coils are
     * always kept.
     *
     * @param[in, out] d        The digitizer data.
     * @param[in]      head     The scalp surface.
     * @param[in]      maxdist  Maximum allowed distance from the surface (m).
     *
     * @return The number of discarded points.
     */
    static int discard_outlier_digitizer_points(FIFFLIB::FiffDigitizerData* d,
                                                 MneMshDisplaySurface* head,
                                                 float maxdist);

    /**
     * Compute the distance from each active digitizer point to the head
     * surface by projecting onto the closest triangle (using approximate
     * neighborhood search).
     *
     * @param[in, out] dig        The digitizer data whose distances are computed.
     * @param[in]      head       The scalp surface.
     * @param[in]      do_all     If non-zero, process all point types including HPI.
     * @param[in]      do_approx  If non-zero, use approximate (neighborhood-restricted) search.
     */
    static void calculate_digitizer_distances(FIFFLIB::FiffDigitizerData* dig, MneMshDisplaySurface* head,
                                                                  int do_all, int do_approx);

    /**
     * Perform one iteration of ICP-like alignment between digitizer points
     * and the head surface: compute current distances, build corresponding
     * point pairs, run Procrustes alignment, and update the head-to-MRI
     * transform.
     *
     * @param[in, out] dig            The digitizer data (transform is updated in-place).
     * @param[in]      head           The scalp surface.
     * @param[in]      nasion_weight  Extra weight factor for the nasion point.
     * @param[in]      nasion_mri     Fixed MRI correspondence point for the nasion (may be NULL).
     * @param[in]      last_step      If non-zero, this is the final iteration (skip re-transform).
     *
     * @return OK on success, FAIL on error.
     */
    static int iterate_alignment_once(FIFFLIB::FiffDigitizerData* dig,	   /* The digitizer data */
                                                   MneMshDisplaySurface* head, /* The head surface */
                                                   int nasion_weight,	   /* Weight for the nasion */
                                                   float *nasion_mri,	   /* Fixed correspondence point for the nasion (optional) */
                                                   int last_step);          /* Is this the last iteration step */

    /**
     * Compute the root-mean-square distance from all active, non-discarded
     * digitizer points to the head surface.
     *
     * @param[in] dig    The digitizer data with pre-computed distances.
     * @param[in] head   The scalp surface.
     *
     * @return The RMS distance in meters.
     */
    static float rms_digitizer_distance(FIFFLIB::FiffDigitizerData* dig, MneMshDisplaySurface* head);

    //============================= display_surfaces.c =============================

    /**
     * Scale a display surface's bounding box and all vertex positions by
     * per-axis scale factors.
     *
     * @param[in, out] surf    The display surface to scale.
     * @param[in]      scales  Per-axis scale factors (3-element float array: x, y, z).
     */
    static void scale_display_surface(MneMshDisplaySurface* surf,
                                        float *scales);

    /**
     * Allocate a curvature array and set all vertex curvature values to 1.0
     * (uniform curvature), if not already present.
     *
     * @param[in, out] s   The surface to modify.
     */
    static void add_uniform_curv(MneSurfaceOld* s);

    //============================= mne_filename_util.c =============================

    /**
     * Construct a FreeSurfer surface file path in the format
     * "$SUBJECTS_DIR/\<subj\>/surf/[\<prefix\>.]\<name\>".
     *
     * @param[in] subj    Subject name (or NULL to use $SUBJECT).
     * @param[in] name    Surface name (e.g. "lh.white").
     * @param[in] prefix  Optional prefix prepended with a dot separator (may be NULL).
     *
     * @return Heap-allocated path string, or NULL if environment variables are unset.
     *         Caller must free the returned string.
     */
    static char * compose_surf_name(const char *subj,
                                        const char *name,
                                        const char *prefix);

    //============================= mne_surface_io.c =============================

    /**
     * Load a FreeSurfer surface and optional curvature file, adding full
     * geometry information (normals, neighbors, distances). Convenience
     * wrapper around load_surface_geom() with geometry and excess-neighbor
     * checking enabled.
     *
     * @param[in] surf_file  Path to the FreeSurfer surface file.
     * @param[in] curv_file  Path to the curvature file (may be NULL).
     *
     * @return A new source space, or NULL on failure. Caller takes ownership.
     */
    static MneSourceSpaceOld* load_surface(char *surf_file,
                    char *curv_file);

    /**
     * Load a FreeSurfer surface and optional curvature file with configurable
     * geometry and neighbor-checking options. Extracts volume geometry from
     * MGH tags when available.
     *
     * @param[in] surf_file                Path to the FreeSurfer surface file.
     * @param[in] curv_file                Path to the curvature file (may be NULL).
     * @param[in] add_geometry             If non-zero, add full geometry information.
     * @param[in] check_too_many_neighbors If non-zero, fail on topological defects.
     *
     * @return A new source space, or NULL on failure. Caller takes ownership.
     */
    static MneSourceSpaceOld* load_surface_geom(char *surf_file,
                         char *curv_file,
                         int  add_geometry,
                         int  check_too_many_neighbors);

    /**
     * Read a FreeSurfer surface file (triangle or quad format).
     *
     * Supports the standard triangle format (3-byte magic TRIANGLE_FILE_MAGIC_NUMBER),
     * old quad format, and new quad format. Quad faces are split into two
     * triangles each. Vertex coordinates are converted from millimeters to meters.
     * Optionally reads trailing MGH tags.
     *
     * @param[in]  fname   Path to the surface file.
     * @param[out] nvertp  Receives the number of vertices.
     * @param[out] ntrip   Receives the number of triangles.
     * @param[out] vertp   Receives the vertex coordinate array. Caller takes ownership.
     * @param[out] trip    Receives the triangle index array. Caller takes ownership.
     * @param[out] tagsp   If non-null, receives a pointer to the MGH tag group.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_triangle_file(char  *fname,
                   int   *nvertp,
                   int   *ntrip,
                   float ***vertp,
                   int   ***trip,
                   void  **tagsp);

    /**
     * Read a FreeSurfer curvature file (new or old binary format).
     *
     * New format: 3-byte magic, integer count, then float values.
     * Old format: 3-byte vertex count, then 2-byte integer values divided by 100.
     *
     * @param[in]  fname   Path to the curvature file.
     * @param[out] curvsp  Receives the curvature values array. Caller takes ownership.
     * @param[out] ncurvp  Receives the number of curvature values.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_curvature_file(char  *fname,
                    float **curvsp,
                    int   *ncurvp);

    /**
     * Validate a quad's edge lengths to detect degenerate quads
     * (edges shorter than 0.1 mm).
     *
     * @param[in] rr   The four vertex coordinates of the quad.
     *
     * @return OK if the quad is valid, FAIL if degenerate.
     */
    static int check_quad(float **rr);

    /**
     * Validate that a vertex index is within the valid range [0, maxno-1].
     *
     * @param[in] no     The vertex index to check.
     * @param[in] maxno  The total number of vertices.
     *
     * @return OK if in range, FAIL otherwise.
     */
    static int check_vertex(int no, int maxno);

    //============================= mne_mgh_mri_io.c =============================

    /**
     * Search an MGH tag group for TAG_OLD_SURF_GEOM and extract a copy of
     * the associated volume geometry.
     *
     * @param[in] tagsp   Pointer to the MGH tag group (MneMghTagGroup*).
     *
     * @return A new copy of the volume geometry, or NULL if not found. Caller takes ownership.
     */
    static MneVolGeom* get_volume_geom_from_tag(void *tagsp);

    /**
     * Deep-copy an MneVolGeom structure, including the filename string.
     *
     * @param[in] g   The volume geometry to duplicate (may be NULL).
     *
     * @return A new copy, or NULL if @p g is NULL. Caller takes ownership.
     */
    static MneVolGeom* dup_vol_geom(MneVolGeom* g);

    /**
     * Read all MGH tags from the current file position until a tag with
     * value 0 (EOF marker) is encountered.
     *
     * @param[in]  fp     The open file positioned after the surface data.
     * @param[out] tagsp  Receives a pointer to the tag group. Caller takes ownership.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_mgh_tags(QFile &fp, void **tagsp);

    /**
     * Read a single MGH tag (ID and data) from the file. Handles both
     * old-style tags (no length field) and new-style tags (with int length).
     *
     * @param[in]  fp     The open file.
     * @param[out] tagp   Receives the tag ID.
     * @param[out] lenp   Receives the data length in bytes.
     * @param[out] datap  Receives a pointer to the tag data (heap-allocated). Caller takes ownership.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_next_tag(QFile &fp, int *tagp, long long *lenp, unsigned char **datap);

    /**
     * Read the raw data payload for a single MGH tag. Handles special cases
     * for zero-length tags (TAG_OLD_SURF_GEOM reads volume geometry,
     * TAG_OLD_USEREALRAS / TAG_USEREALRAS reads a single integer).
     *
     * @param[in]  fp       The open file.
     * @param[in]  tag      The tag ID.
     * @param[in]  nbytes   Number of bytes to read.
     * @param[out] val      Receives a pointer to the data. Caller takes ownership.
     * @param[out] nbytesp  Receives the actual number of bytes read.
     *
     * @return OK on success, FAIL on error.
     */
    static int read_tag_data(QFile &fp, int tag, long long nbytes, unsigned char **val, long long *nbytesp);

    /**
     * Append a new tag entry to an MGH tag group, creating the group if needed.
     *
     * @param[in] g     Existing tag group (may be NULL to create a new one).
     * @param[in] tag   The tag ID.
     * @param[in] len   Length of the tag data in bytes.
     * @param[in] data  Pointer to the tag data (ownership transferred to the group).
     *
     * @return The (possibly newly created) tag group.
     */
    static MneMghTagGroup* add_mgh_tag_to_group(MneMghTagGroup* g, int tag, long long len, unsigned char *data);

    /**
     * Read FreeSurfer volume geometry from text key-value lines in the file.
     *
     * Parses: valid, filename, volume (width/height/depth), voxelsize,
     * xras/yras/zras direction cosines, and cras center. Converts spatial
     * values from millimeters to meters.
     *
     * @param[in] fp   The open file positioned at the volume geometry text.
     *
     * @return A new MneVolGeom structure, or NULL on error. Caller takes ownership.
     */
    static MneVolGeom* read_vol_geom(QFile &fp);

    //============================= mne_binio.c =============================

    /**
     * Read a 3-byte big-endian integer (FreeSurfer custom format) and
     * convert to native byte order.
     *
     * @param[in]  in    The open file.
     * @param[out] ival  Receives the decoded integer value.
     *
     * @return OK on success, FAIL on read error.
     */
    static int read_int3(QFile &in, int *ival);

    /**
     * Read a 4-byte big-endian 32-bit integer and convert to native byte order.
     *
     * @param[in]  in    The open file.
     * @param[out] ival  Receives the decoded integer value.
     *
     * @return OK on success, FAIL on read error.
     */
    static int read_int(QFile &in, qint32 *ival);

    /**
     * Read a 2-byte big-endian short integer and convert to native byte order.
     *
     * @param[in]  in    The open file.
     * @param[out] ival  Receives the decoded integer value (widened to int).
     *
     * @return OK on success, FAIL on read error.
     */
    static int read_int2(QFile &in, int *ival);

    /**
     * Read a 4-byte big-endian float and convert to native byte order.
     *
     * @param[in]  in    The open file.
     * @param[out] fval  Receives the decoded float value.
     *
     * @return OK on success, FAIL on read error.
     */
    static int read_float(QFile &in, float *fval);

    /**
     * Read an 8-byte big-endian long long and convert to native byte order.
     *
     * @param[in]  in    The open file.
     * @param[out] lval  Receives the decoded 64-bit integer value.
     *
     * @return OK on success, FAIL on read error.
     */
    static int read_long(QFile &in, long long *lval);

    //============================= mne_misc.c =============================

    /**
     * Allocate a heap copy of a C string using malloc (platform-independent
     * implementation of POSIX strdup).
     *
     * @param[in] s   The string to duplicate (may be NULL).
     *
     * @return A new heap-allocated copy, or NULL if @p s is NULL.
     *         Caller must free the returned string.
     */
    static char *strdup(const char *s);

public:
    int             type;          /**< Is this a volume or a surface. */
    QString         subject;       /**< Name (id) of the subject. */
    int             id;            /**< Surface id. */
    int             coord_frame;   /**< Which coordinate system the data are in now. */
    /*
     * These relate to the FreeSurfer way
     */
    std::unique_ptr<MneVolGeom> vol_geom;         /**< MRI volume geometry information as FreeSurfer likes it. */
    std::unique_ptr<MneMghTagGroup> mgh_tags;     /**< Tags listed in the file. */
    /*
     * These are meaningful for both surfaces and volumes
     */
    int              np;        /**< Number of vertices. */
    PointsT          rr;        /**< The vertex locations (np x 3, row-major). */
    NormalsT         nn;        /**< Surface normals at these points (np x 3, row-major). */
    float            cm[3];     /**< Center of mass of the vertex cloud. */

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
    std::vector<MneTriangle> tris; /**< The full triangulation data (ntri elements). */
    TrianglesT       itris;     /**< Triangle vertex indices (ntri x 3, row-major). */
    float            tot_area;  /**< Total area of the surface, computed from the triangles (m^2). */

    int              nuse_tri;      /**< Number of triangles in the in-use triangulation. */
    std::vector<MneTriangle> use_tris; /**< Triangulation data for the in-use vertices. */
    TrianglesT       use_itris;     /**< Vertex indices for the in-use triangulation (row-major). */

    std::vector<Eigen::VectorXi> neighbor_tri;    /**< Neighboring triangles for each vertex (np entries, variable length). */
    Eigen::VectorXi   nneighbor_tri;    /**< Number of neighboring triangles for each vertex (np elements). */

    std::vector<MneNearest> nearest; /**< Nearest in-use vertex info for each vertex (np elements). */
    std::vector<std::unique_ptr<MnePatchInfo>> patches; /**< Patch information for each in-use vertex (nuse elements). */

    std::unique_ptr<FIFFLIB::FiffSparseMatrix> dist;         /**< Distances between (used) vertices along the surface. */
    float            dist_limit;    /**< Distance limit: values above this were not computed. Negative means only used vertices were considered. */

    Eigen::VectorXf   curv; /**< The FreeSurfer curvature values (np elements). */
    Eigen::VectorXf   val;  /**< Auxiliary values associated with the vertices (np elements). */
    /*
     * These are for volumes only
     */
    std::unique_ptr<FIFFLIB::FiffCoordTrans>  voxel_surf_RAS_t;   /**< Transform from voxel coordinates to surface RAS (MRI) coordinates. */
    int             vol_dims[3];        /**< Dimensions of the volume grid (width x height x depth). Present only for complete rectangular grids including unused vertices. */
    float           voxel_size[3];      /**< Voxel size in meters, derived from the voxel transform. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> interpolator;       /**< Sparse matrix to interpolate from source space into an MRI volume. */
    QString         MRI_volume;         /**< Path to the MRI volume file the interpolator is based on. */
    std::unique_ptr<FIFFLIB::FiffCoordTrans>  MRI_voxel_surf_RAS_t; /**< Voxel-to-surface-RAS transform for the associated MRI volume. */
    std::unique_ptr<FIFFLIB::FiffCoordTrans>  MRI_surf_RAS_RAS_t; /**< Transform from surface RAS to scanner RAS in the associated MRI volume. */
    int             MRI_vol_dims[3];       /**< Dimensions of the associated MRI volume (width x height x depth). */

// ### OLD STRUCT ###
//typedef struct {                /* This defines a source space or a surface */
//    int              type;          /* Is this a volume or a surface */
//    char             *subject;      /* Name (id) of the subject */
//    int              id;            /* Surface id */
//    int              coord_frame;   /* Which coordinate system are the data in now */
//    /*
//    * These relate to the FreeSurfer way
//    */
//    mneVolGeom       vol_geom;      /* MRI volume geometry information as FreeSurfer likes it */
//    void             *mgh_tags;     /* Tags listed in the file */
//    /*
//    * These are meaningful for both surfaces and volumes
//    */
//    int              np;        /* Number of vertices */
//    float            **rr;      /* The vertex locations */
//    float            **nn;      /* Surface normals at these points */
//    float            cm[3];     /* Center of mass */

//    int              *inuse;    /* Is this point in use in the source space */
//    int              *vertno;   /* Vertex numbers of the used vertices in the full source space */
//    int              nuse;      /* Number of points in use */

//    int              **neighbor_vert; /* Vertices neighboring each vertex */
//    int              *nneighbor_vert; /* Number of vertices neighboring each vertex */
//    float            **vert_dist;     /* Distances between neigboring vertices */
//    /*
//    * These are for surfaces only
//    */
//    int              ntri;      /* Number of triangles */
//    MneTriangle*      tris;      /* The triangulation information */
//    int              **itris;   /* The vertex numbers */
//    float            tot_area;  /* Total area of the surface, computed from the triangles */

//    int              nuse_tri;      /* The triangulation corresponding to the vertices in use */
//    MneTriangle*      use_tris;      /* The triangulation information for the vertices in use */
//    int              **use_itris;   /* The vertex numbers for the 'use' triangulation */

//    int              **neighbor_tri;    /* Neighboring triangles for each vertex Note: number of entries varies for vertex to vertex */
//    int              *nneighbor_tri;    /* Number of neighboring triangles for each vertex */

//    mneNearest       nearest;   /* Nearest inuse vertex info (number of these is the same as the number vertices) */
//    mnePatchInfo     *patches;  /* Patch information (number of these is the same as the number of points in use) */
//    int              npatch;    /* How many (should be same as nuse) */

//    mneSparseMatrix  dist;          /* Distances between the (used) vertices (along the surface). */
//    float            dist_limit;    /* Distances above this (in volume) have not been calculated. If negative, only used vertices have been considered */

//    float            *curv; /* The FreeSurfer curvature values */
//    float            *val;  /* Some other values associated with the vertices */
//    /*
//    * These are for volumes only
//    */
//    MNELIB::FiffCoordTransOld*   voxel_surf_RAS_t; /* Transform from voxel coordinate to the surface RAS (MRI) coordinates */
//    int              vol_dims[3];   /* Dimensions of the volume grid (width x height x depth) NOTE: This will be present only if the source space is a complete rectangular grid with unused vertices included */
//    float            voxel_size[3]; /* Derived from the above */
//    mneSparseMatrix  interpolator;  /* Matrix to interpolate into an MRI volume */
//    char             *MRI_volume;   /* The name of the file the above interpolator is based on */
//    MNELIB::FiffCoordTransOld*   MRI_voxel_surf_RAS_t;
//    MNELIB::FiffCoordTransOld*   MRI_surf_RAS_RAS_t;   /* Transform from surface RAS to RAS coordinates in the associated MRI volume */
//    int              MRI_vol_dims[3];               /* Dimensions of the MRI volume (width x height x depth) */
//    /*
//    * Possibility to add user-defined data
//    */
//    void             *user_data;        /* Anything else we want */
//    mneUserFreeFunc  user_data_free;    /* Function to set the above free */
//} *mneSurfaceOrVolume,mneSurfaceOrVolumeRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNESURFACEORVOLUME_H
