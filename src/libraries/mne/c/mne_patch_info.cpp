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
 * @brief    Definition of the MNE Patch Information (MnePatchInfo) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_patch_info.h"
#include "mne_source_space_old.h"
#include <mne/c/mne_triangle.h>

#define X_43 0
#define Y_43 1
#define Z_43 2

#define VEC_DOT_43(x,y) ((x)[X_43]*(y)[X_43] + (x)[Y_43]*(y)[Y_43] + (x)[Z_43]*(y)[Z_43])

#define FREE_43(x) if ((char *)(x) != NULL) free((char *)(x))

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnePatchInfo::MnePatchInfo()
    :vert (-1)
    ,memb_vert (NULL)
    ,nmemb (0)
    ,area (0)
    ,dev_nn (0)
{
}

//=============================================================================================================

MnePatchInfo::~MnePatchInfo()
{
    FREE_43(memb_vert);
}

//=============================================================================================================

void MnePatchInfo::calculate_patch_area(MneSourceSpaceOld* s, MnePatchInfo *p)
{
    int k,q;
    int nneigh;
    int *neigh;

    p->area = 0.0;
    for (k = 0; k < p->nmemb; k++) {
        nneigh = s->nneighbor_tri[p->memb_vert[k]];
        neigh  = s->neighbor_tri[p->memb_vert[k]];
        for (q = 0; q < nneigh; q++)
            p->area += s->tris[neigh[q]].area/3.0;
    }
}

//=============================================================================================================

void MnePatchInfo::calculate_normal_stats(MneSourceSpaceOld *s, MnePatchInfo *p)
{
    int k;
    float cos_theta,size;

    p->ave_nn[X_43] = 0.0;
    p->ave_nn[Y_43] = 0.0;
    p->ave_nn[Z_43] = 0.0;

    for (k = 0; k < p->nmemb; k++) {
        p->ave_nn[X_43] += s->nn[p->memb_vert[k]][X_43];
        p->ave_nn[Y_43] += s->nn[p->memb_vert[k]][Y_43];
        p->ave_nn[Z_43] += s->nn[p->memb_vert[k]][Z_43];
    }
    size = sqrt(VEC_DOT_43(p->ave_nn,p->ave_nn));
    p->ave_nn[X_43] = p->ave_nn[X_43]/size;
    p->ave_nn[Y_43] = p->ave_nn[Y_43]/size;
    p->ave_nn[Z_43] = p->ave_nn[Z_43]/size;

    p->dev_nn = 0.0;
    for (k = 0; k < p->nmemb; k++) {
        cos_theta = VEC_DOT_43(s->nn[p->memb_vert[k]],p->ave_nn);
        if (cos_theta < -1.0)
            cos_theta = -1.0;
        else if (cos_theta > 1.0)
            cos_theta = 1.0;
        p->dev_nn += acos(cos_theta);
    }
    p->dev_nn = p->dev_nn/p->nmemb;

    return;
}
