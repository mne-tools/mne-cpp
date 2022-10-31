//=============================================================================================================
/**
 * @file     mne_source_space_old.cpp
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
 * @brief    Definition of the MneSourceSpaceOld Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_source_space_old.h"

#define X_51 0
#define Y_51 1
#define Z_51 2

#define ALLOC_INT_51(x) MALLOC_51(x,int)

#define MALLOC_51(x,t) (t *)malloc((x)*sizeof(t))

#define ALLOC_CMATRIX_51(x,y) mne_cmatrix_51((x),(y))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

static void matrix_error_51(int kind, int nr, int nc)

{
    if (kind == 1)
        printf("Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
    else if (kind == 2)
        printf("Failed to allocate memory for a %d x %d matrix\n",nr,nc);
    else
        printf("Allocation error for a %d x %d matrix\n",nr,nc);
    if (sizeof(void *) == 4) {
        printf("This is probably because you seem to be using a computer with 32-bit architecture.\n");
        printf("Please consider moving to a 64-bit platform.");
    }
    printf("Cannot continue. Sorry.\n");
    exit(1);
}

float **mne_cmatrix_51(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_51(nr,float *);
    if (!m) matrix_error_51(1,nr,nc);
    whole = MALLOC_51(nr*nc,float);
    if (!whole) matrix_error_51(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSourceSpaceOld::MneSourceSpaceOld(int np)
{
    this->np      = np;
    if (np > 0) {
        rr      = ALLOC_CMATRIX_51(np,3);
        nn      = ALLOC_CMATRIX_51(np,3);
        inuse   = ALLOC_INT_51(np);
        vertno  = ALLOC_INT_51(np);
    }
    else {
        rr      = Q_NULLPTR;
        nn      = Q_NULLPTR;
        inuse   = Q_NULLPTR;
        vertno  = Q_NULLPTR;
    }
    nuse     = 0;
    ntri     = 0;
    tris     = Q_NULLPTR;
    itris    = Q_NULLPTR;
    tot_area = 0.0;

    nuse_tri  = 0;
    use_tris  = Q_NULLPTR;
    use_itris = Q_NULLPTR;

    neighbor_tri = Q_NULLPTR;
    nneighbor_tri = Q_NULLPTR;
    curv = Q_NULLPTR;
    val  = Q_NULLPTR;

    neighbor_vert = Q_NULLPTR;
    nneighbor_vert = Q_NULLPTR;
    vert_dist = Q_NULLPTR;

    coord_frame = FIFFV_COORD_MRI;
    id          = FIFFV_MNE_SURF_UNKNOWN;
    subject     = "";
    type        = FIFFV_MNE_SPACE_SURFACE;

    nearest = Q_NULLPTR;
    patches = Q_NULLPTR;
    npatch  = 0;

    dist       = Q_NULLPTR;
    dist_limit = -1.0;

    voxel_surf_RAS_t     = Q_NULLPTR;
    vol_dims[0] = vol_dims[1] = vol_dims[2] = 0;

    MRI_volume           = "";
    MRI_surf_RAS_RAS_t   = Q_NULLPTR;
    MRI_voxel_surf_RAS_t = Q_NULLPTR;
    MRI_vol_dims[0] = MRI_vol_dims[1] = MRI_vol_dims[2] = 0;
    interpolator         = Q_NULLPTR;

    vol_geom         = Q_NULLPTR;
    mgh_tags         = Q_NULLPTR;
    user_data        = Q_NULLPTR;
    user_data_free   = Q_NULLPTR;

    cm[X_51] = cm[Y_51] = cm[Z_51] = 0.0;
}

//=============================================================================================================

MneSourceSpaceOld::~MneSourceSpaceOld()
{
}

