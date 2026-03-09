//=============================================================================================================
/**
 * @file     mne_patch_info.cpp
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
 * @brief    Definition of the MNE Patch Information (MNEPatchInfo) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_patch_info.h"
#include "mne_source_space.h"
#include <mne/mne_triangle.h>

#define X_43 0
#define Y_43 1
#define Z_43 2

#define VEC_DOT_43(x,y) ((x)[X_43]*(y)[X_43] + (x)[Y_43]*(y)[Y_43] + (x)[Z_43]*(y)[Z_43])

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

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

    ave_nn[X_43] = 0.0;
    ave_nn[Y_43] = 0.0;
    ave_nn[Z_43] = 0.0;

    for (k = 0; k < memb_vert.size(); k++) {
        ave_nn[X_43] += s->nn(memb_vert[k],X_43);
        ave_nn[Y_43] += s->nn(memb_vert[k],Y_43);
        ave_nn[Z_43] += s->nn(memb_vert[k],Z_43);
    }
    size = sqrt(VEC_DOT_43(ave_nn,ave_nn));
    ave_nn[X_43] = ave_nn[X_43]/size;
    ave_nn[Y_43] = ave_nn[Y_43]/size;
    ave_nn[Z_43] = ave_nn[Z_43]/size;

    dev_nn = 0.0;
    for (k = 0; k < memb_vert.size(); k++) {
        cos_theta = VEC_DOT_43(&s->nn(memb_vert[k],0),ave_nn);
        if (cos_theta < -1.0)
            cos_theta = -1.0;
        else if (cos_theta > 1.0)
            cos_theta = 1.0;
        dev_nn += acos(cos_theta);
    }
    dev_nn = dev_nn/memb_vert.size();

    return;
}
