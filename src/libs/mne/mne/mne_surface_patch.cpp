//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_surface_patch.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     April 2017
 * @brief    Implementation of @ref mne::MNESurfacePatch.
 *
 * Implements patch growth from a seed using triangle adjacency, geodesic
 * distance accumulation and the export back to a plain
 * @ref mne::MNESurface for rendering.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_patch.h"

#include "mne_source_space.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurfacePatch::MNESurfacePatch(int np)
{
    if (np > 0) {
        vert = Eigen::VectorXi::Zero(np);
        border = Eigen::VectorXi::Zero(np);
    }
    s = std::make_unique<MNESourceSpace>(np);
}

//=============================================================================================================

MNESurfacePatch::~MNESurfacePatch() = default;
