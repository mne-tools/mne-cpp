//=============================================================================================================
/**
* @file     mne_surface_or_volume.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the MNE Surface or Volume (MneSurfaceOrVolume) Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_or_volume.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;


//============================= dot.h =============================

#define X_17 0
#define Y_17 1
#define Z_17 2



#define MALLOC_17(x,t) (t *)malloc((x)*sizeof(t))

#define ALLOC_INT_17(x) MALLOC_17(x,int)

#define ALLOC_CMATRIX_17(x,y) mne_cmatrix_17((x),(y))


static void matrix_error_17(int kind, int nr, int nc)

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

float **mne_cmatrix_17(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_17(nr,float *);
    if (!m) matrix_error_17(1,nr,nc);
    whole = MALLOC_17(nr*nc,float);
    if (!whole) matrix_error_17(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

#define FREE_17(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_17(m) mne_free_cmatrix_17((m))

#define FREE_ICMATRIX_17(m) mne_free_icmatrix_17((m))



void mne_free_cmatrix_17 (float **m)
{
    if (m) {
        FREE_17(*m);
        FREE_17(m);
    }
}

void mne_free_patch_17(mnePatchInfo p)

{
    if (!p)
        return;
    FREE_17(p->memb_vert);
    FREE_17(p);
    return;
}

void mne_free_icmatrix_17 (int **m)

{
    if (m) {
        FREE_17(*m);
        FREE_17(m);
    }
}


//============================= mne_mgh_mri_io.c =============================


/*
 * The tag types are private to this module
 */
typedef struct {
    int           tag;
    long long     len;
    unsigned char *data;
} *mneMGHtag,mneMGHtagRec;

typedef struct {
    int        ntags;
    mneMGHtag  *tags;
} *mneMGHtagGroup,mneMGHtagGroupRec;


void mne_free_vol_geom(mneVolGeom g)
{
    if (!g)
        return;
    FREE_17(g->filename);
    FREE_17(g);
    return;
}


static void mne_free_mgh_tag(mneMGHtag t)
{
    if (!t)
        return;
    FREE_17(t->data);
    FREE_17(t);
    return;
}

void mne_free_mgh_tag_group(void *gp)

{
    int k;
    mneMGHtagGroup g = (mneMGHtagGroup)gp;

    if (!g)
        return;
    for (k = 0; k < g->ntags; k++)
        mne_free_mgh_tag(g->tags[k]);
    FREE_17(g->tags);
    FREE_17(g);

    return;
}




//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSurfaceOrVolume::MneSurfaceOrVolume()
{

}


//*************************************************************************************************************

MneSurfaceOrVolume::~MneSurfaceOrVolume()
{
    int k;
    FREE_CMATRIX_17(this->rr);
    FREE_CMATRIX_17(this->nn);
    FREE_17(this->inuse);
    FREE_17(this->vertno);
    FREE_17(this->tris);
    FREE_ICMATRIX_17(this->itris);

    FREE_17(this->use_tris);
    FREE_ICMATRIX_17(this->use_itris);
    if (this->neighbor_tri) {
        for (k = 0; k < this->np; k++)
            FREE_17(this->neighbor_tri[k]);
        FREE_17(this->neighbor_tri);
    }
    FREE_17(this->nneighbor_tri);
    FREE_17(this->curv);

    if (this->neighbor_vert) {
        for (k = 0; k < this->np; k++)
            FREE_17(this->neighbor_vert[k]);
        FREE_17(this->neighbor_vert);
    }
    FREE_17(this->nneighbor_vert);
    if (this->vert_dist) {
        for (k = 0; k < this->np; k++)
            FREE_17(this->vert_dist[k]);
        FREE_17(this->vert_dist);
    }
    FREE_17(this->nearest);
    if (this->patches) {
        for (k = 0; k < this->npatch; k++)
            mne_free_patch_17(this->patches[k]);
        FREE_17(this->patches);
    }
    if(this->dist)
        delete this->dist;
    FREE_17(this->voxel_surf_RAS_t);
    FREE_17(this->MRI_voxel_surf_RAS_t);
    FREE_17(this->MRI_surf_RAS_RAS_t);
    if(this->interpolator)
        delete this->interpolator;
    FREE_17(this->MRI_volume);

    mne_free_vol_geom(this->vol_geom);
    mne_free_mgh_tag_group(this->mgh_tags);

    if (this->user_data && this->user_data_free)
        this->user_data_free(this->user_data);

}





//*************************************************************************************************************

MneSurfaceOrVolume::MneCSourceSpace *MneSurfaceOrVolume::mne_new_source_space(int np)
/*
          * Create a new source space and all associated data
          */
{
    MneCSourceSpace* res = new MneCSourceSpace();
    res->np      = np;
    if (np > 0) {
        res->rr      = ALLOC_CMATRIX_17(np,3);
        res->nn      = ALLOC_CMATRIX_17(np,3);
        res->inuse   = ALLOC_INT_17(np);
        res->vertno  = ALLOC_INT_17(np);
    }
    else {
        res->rr      = NULL;
        res->nn      = NULL;
        res->inuse   = NULL;
        res->vertno  = NULL;
    }
    res->nuse     = 0;
    res->ntri     = 0;
    res->tris     = NULL;
    res->itris    = NULL;
    res->tot_area = 0.0;

    res->nuse_tri  = 0;
    res->use_tris  = NULL;
    res->use_itris = NULL;

    res->neighbor_tri = NULL;
    res->nneighbor_tri = NULL;
    res->curv = NULL;
    res->val  = NULL;

    res->neighbor_vert = NULL;
    res->nneighbor_vert = NULL;
    res->vert_dist = NULL;

    res->coord_frame = FIFFV_COORD_MRI;
    res->id          = FIFFV_MNE_SURF_UNKNOWN;
    res->subject     = NULL;
    res->type        = FIFFV_MNE_SPACE_SURFACE;

    res->nearest = NULL;
    res->patches = NULL;
    res->npatch  = 0;

    res->dist       = NULL;
    res->dist_limit = -1.0;

    res->voxel_surf_RAS_t     = NULL;
    res->vol_dims[0] = res->vol_dims[1] = res->vol_dims[2] = 0;

    res->MRI_volume           = NULL;
    res->MRI_surf_RAS_RAS_t   = NULL;
    res->MRI_voxel_surf_RAS_t = NULL;
    res->MRI_vol_dims[0] = res->MRI_vol_dims[1] = res->MRI_vol_dims[2] = 0;
    res->interpolator         = NULL;

    res->vol_geom         = NULL;
    res->mgh_tags         = NULL;
    res->user_data        = NULL;
    res->user_data_free   = NULL;

    res->cm[X_17] = res->cm[Y_17] = res->cm[Z_17] = 0.0;

    return res;
}
