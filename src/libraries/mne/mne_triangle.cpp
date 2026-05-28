//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_triangle.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNETriangle.
 *
 * Implements the constructors and the triangle-property recomputation
 * from three vertex coordinates.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_triangle.h"

#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNETriangle::MNETriangle() = default;

//=============================================================================================================

void MNETriangle::compute_data()
{
    r12 = r2 - r1;
    r13 = r3 - r1;

    nn = r12.cross(r13);
    float size = nn.norm();
    if (size > 0)
        nn /= size;
    area = size / 2.0f;

    float sizey = r13.norm();
    if (sizey <= 0)
        sizey = 1.0f;
    ey = r13 / sizey;

    cent = (r1 + r2 + r3) / 3.0f;

    ex = ey.cross(nn);
}
