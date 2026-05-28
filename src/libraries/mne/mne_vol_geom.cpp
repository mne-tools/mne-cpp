//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_vol_geom.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEVolGeom.
 *
 * Implements ASCII parsing of the @c valid/filename/volume/voxelsize
 * lines that FreeSurfer writes verbatim, plus the construction of the
 * voxel-to-RAS affine from those fields.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_vol_geom.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEVolGeom::MNEVolGeom()
: valid(0)
, width(0)
, height(0)
, depth(0)
, xsize(0.0f)
, ysize(0.0f)
, zsize(0.0f)
, x_ras{0.0f, 0.0f, 0.0f}
, y_ras{0.0f, 0.0f, 0.0f}
, z_ras{0.0f, 0.0f, 0.0f}
, c_ras{0.0f, 0.0f, 0.0f}
{
}

//=============================================================================================================

MNEVolGeom::~MNEVolGeom()
{
}
