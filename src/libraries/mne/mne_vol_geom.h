//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_vol_geom.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    FreeSurfer volume geometry header carried by surface and tag streams.
 *
 * @ref mne::MNEVolGeom records the original volume's dimensions,
 * voxel size, direction cosines and centre RAS - the information needed
 * to convert a surface saved in @c surf/lh.white back into the RAS
 * coordinates of the originating @c mri/orig.mgz. Embedded in MGH tag
 * streams and in the headers of FreeSurfer surface files.
 */

#ifndef MNEVOLGEOM_H
#define MNEVOLGEOM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <string>
#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements the MNE Volume Geometry description (Replaces *mneVolGeom,mneVolGeomRec; struct of MNE-C mne_types.h).
 *
 * @brief MRI data volume geometry information like FreeSurfer keeps it
 */
class MNESHARED_EXPORT MNEVolGeom
{
public:
    //=========================================================================================================
    /**
     * Constructs the MNE Volume Geometry
     */
    MNEVolGeom();

    //=========================================================================================================
    /**
     * Destroys the MNE Volume Geometry
     * Refactored: mne_free_vol_geom (mne_mgh_mri_io.c)
     */
    ~MNEVolGeom();

public:
    int valid;                          /**< Non-zero if the geometry information below is valid. */
    int width, height, depth;           /**< Dimensions of the volume stack (in voxels). */
    float xsize, ysize, zsize;          /**< Voxel size in each direction (m; the file stores mm). */
    float x_ras[3], y_ras[3], z_ras[3]; /**< Direction cosines of the three voxel axes in RAS coordinates. */
    float c_ras[3];                     /**< Center of the volume in RAS coordinates (m; the file stores mm). */
    std::string filename;               /**< Path to the MRI data file this geometry was read from. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEVOLGEOM_H
