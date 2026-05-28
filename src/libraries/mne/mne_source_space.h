//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_source_space.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Single-hemisphere source space (cortical surface or volume grid) loaded from FIFF.
 *
 * @ref MNELIB::MNESourceSpace replaces the @c mneSourceSpace typedef of
 * @c mne_types.h and is the C++ representation of one decimated source
 * space stored under the @c FIFFB_MNE_SOURCE_SPACE block in a forward /
 * inverse FIFF file. It holds the dense surface (vertices, triangles,
 * normals, neighbouring relations) plus the subset selected by the MNE
 * decimation (@c inuse / @c nuse / @c vertno) used to index the leadfield
 * columns of @ref MNEForwardSolution. The structure is consumed by
 * @ref MNESourceSpaces (left+right pair), @ref MNEHemisphere, the BEM
 * tools and every clustering or morphing step in the inverse pipeline.
 */

#ifndef MNESOURCESPACE_H
#define MNESOURCESPACE_H

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
// STL INCLUDES
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

class MNEHemisphere;

//=============================================================================================================
/**
 * Implements the MNE Source Space (Replaces typedef mneSurfaceOrVolume mneSourceSpace; struct of MNE-C mne_types.h).
 *
 * @brief This defines a source space
 */
class MNESHARED_EXPORT MNESourceSpace : public MNESurfaceOrVolume
{
public:
    using SPtr = std::shared_ptr<MNESourceSpace>;              /**< Shared pointer type for MNESourceSpace. */
    using ConstSPtr = std::shared_ptr<const MNESourceSpace>;   /**< Const shared pointer type for MNESourceSpace. */
    using UPtr = std::unique_ptr<MNESourceSpace>;              /**< Unique pointer type for MNESourceSpace. */

    //=========================================================================================================
    /**
     * Constructs the MNE Source Space
     */
    MNESourceSpace(int np = 0);

    //=========================================================================================================
    /**
     * Destroys the MNE Source Space
     */
    ~MNESourceSpace() override;

    //=========================================================================================================
    /**
     * Creates a deep copy of this source space, preserving the actual derived type.
     *
     * @return a shared_ptr to the cloned source space.
     */
    virtual MNESourceSpace::SPtr clone() const;

    //=========================================================================================================
    /**
     * Enable all source points (set inuse to TRUE, nuse = np).
     */
    void enable_all_sources();

    //=========================================================================================================
    /**
     * Determine whether this source space is on the left hemisphere
     * based on the average x-coordinate of all vertices.
     *
     * @return true if left hemisphere, false otherwise.
     */
    bool is_left_hemi() const;

    //=========================================================================================================
    /**
     * Returns the hemisphere id (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI)
     * for this source space, based on the average x-coordinate of all vertices.
     *
     * @return the deduced hemisphere id.
     */
    qint32 find_source_space_hemi() const;

    //=========================================================================================================
    /**
     * Update the active vertex (inuse) vector and recompute vertno.
     *
     * @param[in] new_inuse   New in-use vector (size np).
     */
    void update_inuse(Eigen::VectorXi new_inuse);

    //=========================================================================================================
    /**
     * Transform this source space into another coordinate frame.
     *
     * @param[in] t   The coordinate transformation to apply.
     * @return OK on success, FAIL on error.
     */
    int transform_source_space(const FIFFLIB::FiffCoordTrans& t);

    //=========================================================================================================
    /**
     * Compute patch statistics (areas, normals, deviations).
     *
     * @return OK on success, FAIL on error.
     */
    int add_patch_stats();

    //=========================================================================================================
    /**
     * Rearrange source space after filtering: recount nuse, rebuild vertno,
     * and recompute patch statistics if nearest info is available.
     */
    void rearrange_source_space();

    //=========================================================================================================
    /**
     * Create a new source space with np vertices and all associated data
     * initialized to defaults.
     *
     * @param[in] np   Number of vertices.
     * @return Unique pointer to the newly allocated source space.
     */
    static std::unique_ptr<MNESourceSpace> create_source_space(int np);

    //=========================================================================================================
    /**
     * Load a FreeSurfer surface from disk, adding full geometry information.
     *
     * @param[in] surf_file   Path to the FreeSurfer surface file.
     * @param[in] curv_file   Path to the curvature file (may be empty).
     * @return Unique pointer to the loaded source space, or nullptr on error.
     */
    static std::unique_ptr<MNESourceSpace> load_surface(const QString& surf_file,
                                                        const QString& curv_file);

    //=========================================================================================================
    /**
     * Load a FreeSurfer surface from disk, optionally adding geometry information.
     *
     * @param[in] surf_file                 Path to the FreeSurfer surface file.
     * @param[in] curv_file                 Path to the curvature file (may be empty).
     * @param[in] add_geometry              Whether to compute full geometry info.
     * @param[in] check_too_many_neighbors  Whether to error on excess neighbors.
     * @return Unique pointer to the loaded source space, or nullptr on error.
     */
    static std::unique_ptr<MNESourceSpace> load_surface_geom(const QString& surf_file,
                                                             const QString& curv_file,
                                                             bool add_geometry,
                                                             bool check_too_many_neighbors);

    //=========================================================================================================
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
    static MNESourceSpace* make_volume_source_space(const MNESurface& surf,
                                                    float grid,
                                                    float exclude,
                                                    float mindist);

    //=========================================================================================================
    // Source-space filtering, I/O, and label helpers (moved from MNESurfaceOrVolume)
    //=========================================================================================================

    static int filter_source_spaces(const MNESurface& surf,
                                    float limit,
                                    const FIFFLIB::FiffCoordTrans& mri_head_t,
                                    std::vector<std::unique_ptr<MNESourceSpace>>& spaces,
                                    QTextStream *filtered);

    static void filter_source_space(FilterThreadArg *arg);

    static int filter_source_spaces(float limit,
                                    const QString& bemfile,
                                    const FIFFLIB::FiffCoordTrans& mri_head_t,
                                    std::vector<std::unique_ptr<MNESourceSpace>>& spaces,
                                    QTextStream *filtered,
                                    bool use_threads);

    static int read_source_spaces(const QString& name,
                                  std::vector<std::unique_ptr<MNESourceSpace>>& spaces);

    static int transform_source_spaces_to(int coord_frame,
                                          const FIFFLIB::FiffCoordTrans& t,
                                          std::vector<std::unique_ptr<MNESourceSpace>>& spaces);

    static int restrict_sources_to_labels(std::vector<std::unique_ptr<MNESourceSpace>>& spaces,
                                          const QStringList& labels,
                                          int nlabel);

    static int read_label(const QString& label,
                          Eigen::VectorXi& sel);

    //=========================================================================================================
    /**
     * Write this source space to a FIFF stream.
     *
     * @param[in] stream         The FIFF output stream.
     * @param[in] selected_only  If true, write only the vertices that are in use.
     *
     * @return FIFF_OK on success, FIFF_FAIL on error.
     */
    int writeToStream(FIFFLIB::FiffStream::SPtr& stream, bool selected_only) const;

    //=========================================================================================================
    /**
     * Downsample a hemisphere source space to an icosahedral subdivision
     * of the given grade.
     *
     * For each vertex of the icosahedron at the requested grade, the
     * nearest vertex in the hemisphere is found and marked as in-use.
     *
     * @param[in] hemi      The hemisphere to downsample.
     * @param[in] icoGrade  Icosahedral subdivision grade (0-7).
     *
     * @return A new MNEHemisphere with the downsampled vertex selection.
     *
     * @since 2.2.0
     */
    static MNEHemisphere icoDownsample(const MNEHemisphere& hemi, int icoGrade);

private:
    /**
     * Write volume source space information (neighbors, voxel transforms, interpolator) to a FIFF stream.
     *
     * @param[in] stream         The FIFF output stream.
     * @param[in] selected_only  If true, write only selected (in-use) vertices.
     *
     * @return OK on success, FAIL on error.
     */
    int writeVolumeInfo(FIFFLIB::FiffStream::SPtr& stream, bool selected_only) const;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNESOURCESPACE_H
