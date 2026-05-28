//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_surface_patch.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNESurfacePatch.
 *
 * Implements patch growth from a seed using triangle adjacency, geodesic
 * distance accumulation and the export back to a plain
 * @ref MNELIB::MNESurface for rendering.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_patch.h"

#include "mne_source_space.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurfacePatch::MNESurfacePatch(int np)
{
     if (np > 0) {
       vert   = Eigen::VectorXi::Zero(np);
       border = Eigen::VectorXi::Zero(np);
     }
     s = std::make_unique<MNESourceSpace>(np);
}

//=============================================================================================================

MNESurfacePatch::~MNESurfacePatch() = default;
