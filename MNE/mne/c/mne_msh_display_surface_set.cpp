//=============================================================================================================
/**
* @file     mne_msh_display_surface_set.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the MneMshDisplaySurfaceSet Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_msh_display_surface_set.h"

#include "mne_msh_display_surface.h"
#include "mne_surface_old.h"
#include "mne_surface_patch.h"
#include "mne_source_space_old.h"
#include "mne_msh_light.h"

#include <fiff/c/fiff_coord_trans_set.h>


#define MALLOC_47(x,t) (t *)malloc((x)*sizeof(t))

#define FREE_47(x) if ((char *)(x) != Q_NULLPTR) free((char *)(x))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMshDisplaySurfaceSet::MneMshDisplaySurfaceSet(int nsurf)
{
    int k;

    this->nsurf = nsurf;
    if (nsurf > 0) {
        MneMshDisplaySurface* temp = new MneMshDisplaySurface;
        surfs = MALLOC_47(nsurf,temp);
        patches = MALLOC_47(nsurf,mneSurfacePatch);
        patch_rot = MALLOC_47(nsurf,float);
        active = MALLOC_47(nsurf,int);
        drawable = MALLOC_47(nsurf,int);

        for (k = 0; k < nsurf; k++) {
            surfs[k]  = Q_NULLPTR;
            active[k] = FALSE;
            drawable[k] = TRUE;
            patches[k] = Q_NULLPTR;
            patch_rot[k] = 0.0;
        }
    }
    else {
        surfs = Q_NULLPTR;
        active = Q_NULLPTR;
        patches = Q_NULLPTR;
        drawable = Q_NULLPTR;
        patch_rot= Q_NULLPTR;
    }
    subj       = Q_NULLPTR;
    morph_subj = Q_NULLPTR;
    main_t     = Q_NULLPTR;
    morph_t    = Q_NULLPTR;

    use_patches = FALSE;
    lights  = Q_NULLPTR;
    user_data = Q_NULLPTR;
    user_data_free = Q_NULLPTR;

    rot[0] = 0.0;
    rot[1] = 0.0;
    rot[2] = 0.0;

    move[0] = 0.0;
    move[1] = 0.0;
    move[2] = 0.0;

    eye[0]      = 1.0;
    eye[1]      = 0.0;
    eye[2]      = 0.0;

    up[0]       = 0.0;
    up[1]       = 0.0;
    up[2]       = 1.0;

    bg_color[0] = 0.0;
    bg_color[1] = 0.0;
    bg_color[2] = 0.0;

    text_color[0] = 1.0;
    text_color[1] = 1.0;
    text_color[2] = 1.0;
}


//*************************************************************************************************************

MneMshDisplaySurfaceSet::~MneMshDisplaySurfaceSet()
{
    int k;

    for (k = 0; k < nsurf; k++)
        delete surfs[k];
    if (patches) {
        for (k = 0; k < nsurf; k++)
            delete patches[k];
        delete patches;
    }
    delete main_t;
    delete morph_t;
    FREE_47(patch_rot);
    FREE_47(surfs);
    delete lights;
    if (user_data_free)
        user_data_free(user_data);
}


//*************************************************************************************************************

MneMshDisplaySurfaceSet* MneMshDisplaySurfaceSet::load_new_surface(char *subj, char *name, char *curv)
     /*
      * Load new display surface data
      */
{
    MneSourceSpaceOld* left  = Q_NULLPTR;
    MneSourceSpaceOld* right = Q_NULLPTR;
    char *this_surf = Q_NULLPTR;
    char *this_curv = Q_NULLPTR;
    MneMshDisplaySurface* thisSurface = Q_NULLPTR;
    MneMshDisplaySurfaceSet* surfs = Q_NULLPTR;

    if (!curv)
        curv = "curv";

    this_surf = mne_compose_surf_name(subj,name,"lh");
    if (this_surf == Q_NULLPTR)
        goto bad;
    this_curv = mne_compose_surf_name(subj,curv,"lh");
    if ((left = mne_load_surface(this_surf,this_curv)) == Q_NULLPTR)
        goto bad;
    FREE_47(this_surf); FREE_47(this_curv);

    this_surf = mne_compose_surf_name(subj,name,"rh");
    this_curv = mne_compose_surf_name(subj,curv,"rh");
    if ((right = mne_load_surface(this_surf,this_curv)) == Q_NULLPTR)
        goto bad;
    FREE_47(this_surf); FREE(this_curv);

    surfs = new_display_surface_set(2);

    surfs->surfs[0] = new_display_surface();
    surfs->surfs[1] = new_display_surface();

    surfs->active[0]  = TRUE;
    surfs->active[1]  = FALSE;
    surfs->current    = 0;

    thisSurface = surfs->surfs[0];
    thisSurface->s         = left;
    thisSurface->s->id     = SURF_LEFT_HEMI;
    thisSurface->subj      = mne_strdup(subj);
    thisSurface->surf_name = mne_strdup(name);
    decide_surface_extent(thisSurface,"Left hemisphere");
    decide_curv_display(name,thisSurface);
    setup_curvature_colors (thisSurface);

    thisSurface    = surfs->surfs[1];
    thisSurface->s         = right;
    thisSurface->s->id     = SURF_RIGHT_HEMI;
    thisSurface->subj      = mne_strdup(subj);
    thisSurface->surf_name = mne_strdup(name);
    decide_surface_extent(thisSurface,"Right hemisphere");
    decide_curv_display(name,thisSurface);
    setup_curvature_colors (thisSurface);

    apply_left_right_eyes(surfs);

    setup_current_surface_lights(surfs);

    return surfs;

    bad : {
        delete left;
        delete right;
        FREE_47(this_surf);
        FREE_47(this_curv);
        return Q_NULLPTR;
    }
}
