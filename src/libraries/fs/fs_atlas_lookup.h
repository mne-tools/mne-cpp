//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_atlas_lookup.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    RAS-coordinate lookup against a FreeSurfer volumetric parcellation (e.g. aparc+aseg.mgz / wmparc.mgz).
 *
 * FreeSurfer’s @c recon-all writes the joint cortical+subcortical
 * parcellation @c mri/aparc+aseg.mgz and the white-matter counterpart
 * @c mri/wmparc.mgz: each voxel carries a single integer label whose
 * meaning is fixed by @c $FREESURFER_HOME/FreeSurferColorLUT.txt (e.g.
 * @c 17 = Left-Hippocampus, @c 1023 = ctx-lh-posteriorcingulate). The
 * embedded MGH/MGZ @c vox2ras affine ties those voxel indices to scanner
 * RAS millimetres.
 *
 * This class loads such a volume, inverts the @c vox2ras matrix to obtain
 * @c ras2vox, and exposes @c label_at_ras() / @c labels_for_positions() so any
 * point in scanner RAS (typically dipole positions, electrode locations or
 * sampled fibre tracks) can be tagged with the FreeSurfer region name it
 * falls into. Out-of-bounds or unlabeled voxels resolve to @c "Unknown",
 * matching @c mri_label2vol convention.
 */

#ifndef FS_ATLAS_LOOKUP_H
#define FS_ATLAS_LOOKUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief RAS-point → anatomical-region resolver backed by a FreeSurfer volumetric parcellation.
 *
 * Owns the parcellation voxel grid, its inverse RAS-to-voxel affine, and a
 * label-id → region-name map seeded from FreeSurferColorLUT. Designed for
 * cheap per-point queries: a @c label_at_ras() call is an affine
 * transform, three bound checks and an @c int → name hash lookup.
 *
 * @snippet ex_fs/main.cpp fs_atlas_lookup_usage
 */
class FSSHARED_EXPORT FsAtlasLookup
{
public:
    //=========================================================================================================
    /**
     * @brief Load a FreeSurfer MGH or gzip-compressed MGZ volume parcellation.
     * @param[in] parcellation  The volume file (e.g. aparc+aseg.mgz).
     * @return true if loaded successfully.
     */
    bool load(const std::filesystem::path& parcellation);

    //=========================================================================================================
    /**
     * @brief Look up the anatomical label at a RAS coordinate.
     * @param[in] ras  3D position in RAS space (in mm).
     * @return Region name, or "Unknown" if out of bounds or unlabeled.
     */
    const std::string& label_at_ras(const Eigen::Vector3f& ras) const;

    //=========================================================================================================
    /**
     * @brief Look up labels for multiple RAS positions.
     * @param[in] positions  3D RAS positions.
     * @return One region name per position.
     */
    std::vector<std::string> labels_for_positions(std::span<const Eigen::Vector3f> positions) const;

    //=========================================================================================================
    /**
     * @brief Check whether a parcellation volume has been loaded.
     * @return true if loaded.
     */
    bool is_loaded() const;

private:
    //=========================================================================================================
    /**
     * @brief Convert a RAS coordinate to voxel indices.
     * @param[in] ras  3D RAS position.
     * @return Rounded voxel indices.
     */
    Eigen::Vector3i ras_to_voxel(const Eigen::Vector3f& ras) const;

    std::vector<int> m_voxelData;                            /**< Labels of the first frame, x fastest. */
    int m_dimX = 0;                                          /**< Volume dimension X. */
    int m_dimY = 0;                                          /**< Volume dimension Y. */
    int m_dimZ = 0;                                          /**< Volume dimension Z. */
    Eigen::Matrix4f m_ras2vox = Eigen::Matrix4f::Identity(); /**< RAS-to-voxel affine (inverse of vox2ras). */
    bool m_loaded = false;                                   /**< Whether a volume was loaded. */
};

} // namespace mne

#endif // FS_ATLAS_LOOKUP_H
