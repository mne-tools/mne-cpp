//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_atlas_lookup.h
 * @since April 2026
 * @brief RAS-coordinate lookup against a FreeSurfer volumetric parcellation (e.g. aparc+aseg.mgz / wmparc.mgz).
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
 * @c ras2vox, and exposes @c labelAtRas() / @c labelsForPositions() so any
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
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
/**
 * @brief RAS-point → anatomical-region resolver backed by a FreeSurfer volumetric parcellation.
 *
 * Owns the parcellation voxel grid, its inverse RAS-to-voxel affine, and a
 * label-id → region-name map seeded from FreeSurferColorLUT. Designed for
 * cheap per-point queries: a @c labelAtRas() call is an affine
 * transform, three bound checks and an @c int → @c QString hash lookup.
 */
class FSSHARED_EXPORT FsAtlasLookup
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty atlas lookup.
     */
    FsAtlasLookup();

    //=========================================================================================================
    /**
     * @brief Load a FreeSurfer MGH/MGZ volume parcellation.
     * @param[in] sParcellationPath  Path to the volume file (e.g. aparc+aseg.mgz).
     * @return true if loaded successfully.
     */
    bool load(const QString& sParcellationPath);

    //=========================================================================================================
    /**
     * @brief Look up the anatomical label at a RAS coordinate.
     * @param[in] ras  3D position in RAS space (in mm).
     * @return Label string, or "Unknown" if out of bounds or unlabeled.
     */
    QString labelAtRas(const Eigen::Vector3f& ras) const;

    //=========================================================================================================
    /**
     * @brief Look up labels for multiple RAS positions.
     * @param[in] positions  List of 3D RAS positions.
     * @return List of label strings (one per position).
     */
    QStringList labelsForPositions(const QVector<Eigen::Vector3f>& positions) const;

    //=========================================================================================================
    /**
     * @brief Check whether a parcellation volume has been loaded.
     * @return true if loaded.
     */
    bool isLoaded() const;

private:
    QVector<int> m_voxelData;           /**< Flat voxel data (label indices). */
    int m_dimX = 0;                     /**< Volume dimension X. */
    int m_dimY = 0;                     /**< Volume dimension Y. */
    int m_dimZ = 0;                     /**< Volume dimension Z. */

    Eigen::Matrix4f m_ras2vox;          /**< RAS-to-voxel affine (inverse of vox2ras). */

    QMap<int, QString> m_lookupTable;   /**< Label index → region name. */

    bool m_loaded = false;

    //=========================================================================================================
    /**
     * @brief Initialize the FreeSurfer color lookup table.
     */
    void initLookupTable();

    //=========================================================================================================
    /**
     * @brief Convert a RAS coordinate to voxel indices.
     * @param[in] ras  3D RAS position.
     * @return Rounded voxel indices.
     */
    Eigen::Vector3i rasToVoxel(const Eigen::Vector3f& ras) const;
};

} // namespace FSLIB

#endif // FS_ATLAS_LOOKUP_H
