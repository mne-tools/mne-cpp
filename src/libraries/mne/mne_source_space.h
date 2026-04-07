//=============================================================================================================
/**
 * @file     mne_source_space.h
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
 * @brief    MNESourceSpace class declaration.
 *
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
