//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_patch_info.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref MNELIB::MNEPatchInfo.
 *
 * Implements patch construction from the source-space @ref MNENearest
 * mapping and the FIFF read/write of @c FIFF_MNE_SOURCE_SPACE_PATCH_INFO
 * tags.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_patch_info.h"
#include "mne_source_space.h"
#include <mne/mne_triangle.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEPatchInfo::MNEPatchInfo()
    :vert (-1)
    ,area (0)
    ,dev_nn (0)
{
    ave_nn[0] = 0;
    ave_nn[1] = 0;
    ave_nn[2] = 0;
}

//=============================================================================================================

MNEPatchInfo::~MNEPatchInfo() = default;

//=============================================================================================================

void MNEPatchInfo::calculate_area(MNESourceSpace* s)
{
    int k,q;
    int nneigh;

    area = 0.0;
    for (k = 0; k < memb_vert.size(); k++) {
        nneigh = s->nneighbor_tri[memb_vert[k]];
        const Eigen::VectorXi& neigh = s->neighbor_tri[memb_vert[k]];
        for (q = 0; q < nneigh; q++)
            area += s->tris[neigh[q]].area/3.0;
    }
}

//=============================================================================================================

void MNEPatchInfo::calculate_normal_stats(MNESourceSpace *s)
{
    int k;
    float cos_theta,size;

    Eigen::Map<Eigen::Vector3f> ave(ave_nn);
    ave.setZero();

    for (k = 0; k < memb_vert.size(); k++) {
        ave += s->nn.row(memb_vert[k]).transpose();
    }
    size = ave.norm();
    ave /= size;

    dev_nn = 0.0;
    for (k = 0; k < memb_vert.size(); k++) {
        cos_theta = s->nn.row(memb_vert[k]).dot(ave);
        if (cos_theta < -1.0)
            cos_theta = -1.0;
        else if (cos_theta > 1.0)
            cos_theta = 1.0;
        dev_nn += acos(cos_theta);
    }
    dev_nn = dev_nn/memb_vert.size();

    return;
}
