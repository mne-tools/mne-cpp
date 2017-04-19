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
#include "mne_msh_light_set.h"
#include "mne_msh_light.h"
#include "mne_msh_eyes.h"

#include <fiff/c/fiff_coord_trans_set.h>


#define MALLOC_47(x,t) (t *)malloc((x)*sizeof(t))

#define FREE_47(x) if ((char *)(x) != Q_NULLPTR) free((char *)(x))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define X_47 0
#define Y_47 1
#define Z_47 2

#define SURF_LEFT_HEMI        FIFFV_MNE_SURF_LEFT_HEMI
#define SURF_RIGHT_HEMI       FIFFV_MNE_SURF_RIGHT_HEMI

#define VEC_COPY_47(to,from) {\
    (to)[X_47] = (from)[X_47];\
    (to)[Y_47] = (from)[Y_47];\
    (to)[Z_47] = (from)[Z_47];\
}

#define SHOW_CURVATURE_NONE    0
#define SHOW_CURVATURE_OVERLAY 1
#define SHOW_OVERLAY_HEAT      1

#define SURF_LEFT_MORPH_HEMI  (1 << 16 | FIFFV_MNE_SURF_LEFT_HEMI)
#define SURF_RIGHT_MORPH_HEMI (1 << 16 | FIFFV_MNE_SURF_RIGHT_HEMI)

#define POS_CURV_COLOR  0.25
#define NEG_CURV_COLOR  0.375
#define EVEN_CURV_COLOR 0.375

static MNELIB::MneMshEyes   default_eyes;
static MNELIB::MneMshEyes*  all_eyes     = Q_NULLPTR;
static int          neyes        = 0;
static int          current_eyes = -1;
static int         ndefault         = 8;

static mshLightSet custom_lights = Q_NULLPTR;
static mshLightRec default_lights[] = { { TRUE, { 0.0,   0.0,  1.0 } , { 0.8, 0.8, 0.8 } },
                    { TRUE, { 0.0,   0.0, -1.0 } , { 0.8, 0.8, 0.8 } },
                    { TRUE, { 0.6,  -1.0, -1.0 } , { 0.6, 0.6, 0.6 } },
                    { TRUE, { -0.6, -1.0, -1.0 } , { 0.6, 0.6, 0.6 } },
                    { TRUE, { 1.0,   0.0, 0.0 }  , { 0.8, 0.8, 0.8 } },
                    { TRUE, { -1.0,  0.0, 0.0 }  , { 0.8, 0.8, 0.8 } },
                    { TRUE, { 0.0,   1.0, 0.5 }  , { 0.6, 0.6, 0.6 } },
                    { FALSE, { 0.0,   0.0, -1.0 } , { 1.0, 1.0, 1.0 } }} ;

mshLightSet new_light_set()
{
  mshLightSet s = MALLOC_47(1,mshLightSetRec);

  s->name = NULL;
  s->lights = NULL;
  s->nlight = 0;

  return s;
}

void free_light_set(mshLightSet s)
{
  if (!s)
    return;
  FREE_47(s->name);
  FREE_47(s->lights);
  FREE_47(s);
  return;
}


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
    default_eyes.name = Q_NULLPTR;

    default_eyes.left[0] = -0.2;
    default_eyes.left[0] = 0.0;
    default_eyes.left[0] = 0.0;

    default_eyes.right[0] = 0.2;
    default_eyes.right[0] = 0.0;
    default_eyes.right[0] = 0.0;

    default_eyes.left_up[0] = 0.0;
    default_eyes.left_up[0] = 0.0;
    default_eyes.left_up[0] = 1.0;

    default_eyes.right_up[0] = 0.0;
    default_eyes.right_up[0] = 0.0;
    default_eyes.right_up[0] = 1.0;

    int k;

    this->nsurf = nsurf;
    if (nsurf > 0) {
        surfs = MALLOC_47(nsurf,MneMshDisplaySurface*);
        patches = MALLOC_47(nsurf,MneSurfacePatch*);
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
    } else {
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
    char *left_file = Q_NULLPTR;
    char *right_file = Q_NULLPTR;
    char *this_surf = Q_NULLPTR;
    char *this_curv = Q_NULLPTR;
    MneMshDisplaySurface* pThis = Q_NULLPTR;
    MneMshDisplaySurfaceSet* surfs = Q_NULLPTR;

    if (!curv)
        *curv = 'curv';

    this_surf = MneSurfaceOrVolume::mne_compose_surf_name(subj,name,"lh");
    if (this_surf == Q_NULLPTR)
        goto bad;
    this_curv = MneSurfaceOrVolume::mne_compose_surf_name(subj,curv,"lh");
    fprintf(stderr,"Loading surface %s ...\n",this_surf);
    if ((left = MneSurfaceOrVolume::mne_load_surface(this_surf,this_curv)) == Q_NULLPTR) {
        if ((left = MneSurfaceOrVolume::mne_load_surface(this_surf,Q_NULLPTR)) == Q_NULLPTR)
            goto bad;
        else
            MneSurfaceOrVolume::add_uniform_curv((MneSurfaceOld*)left);
    }
    left_file = this_surf; this_surf = Q_NULLPTR;
    FREE_47(this_curv);

    char rh = 'rh';

    this_surf = MneSurfaceOrVolume::mne_compose_surf_name(subj,name,&rh);
    this_curv = MneSurfaceOrVolume::mne_compose_surf_name(subj,curv,&rh);
    fprintf(stderr,"Loading surface %s ...\n",this_surf);
    if ((right = MneSurfaceOrVolume::mne_load_surface(this_surf,this_curv)) == Q_NULLPTR) {
        if ((right = MneSurfaceOrVolume::mne_load_surface(this_surf,Q_NULLPTR)) == Q_NULLPTR)
            goto bad;
        else
            MneSurfaceOrVolume::add_uniform_curv((MneSurfaceOld*)right);
    }
    right_file = this_surf; this_surf = Q_NULLPTR;
    FREE_47(this_curv);

    surfs = new MneMshDisplaySurfaceSet(2);

    surfs->surfs[0] = new MneMshDisplaySurface();
    surfs->surfs[1] = new MneMshDisplaySurface();

    surfs->active[0]  = TRUE;
    surfs->active[1]  = FALSE;
    surfs->drawable[0]  = TRUE;
    surfs->drawable[1]  = TRUE;

    pThis              = surfs->surfs[0];
    pThis->filename    = left_file;
    //pThis->time_loaded = time(Q_NULLPTR); //Comment out due to unknown timestemp function ToDo
    pThis->s           = (MneSurfaceOld*)left;
    pThis->s->id       = SURF_LEFT_HEMI;
    pThis->subj        = MneSurfaceOrVolume::mne_strdup(subj);
    pThis->surf_name   = MneSurfaceOrVolume::mne_strdup(name);

    char lefthemi[16] = "Left hemisphere";
    decide_surface_extent(pThis,lefthemi);
    decide_curv_display(name,pThis);
    setup_curvature_colors (pThis);

    pThis              = surfs->surfs[1];
    pThis->filename    = right_file;
    //pThis->time_loaded = time(Q_NULLPTR); //Comment out due to unknown timestemp function ToDo
    pThis->s           = (MneSurfaceOld*)right;
    pThis->s->id       = SURF_RIGHT_HEMI;
    pThis->subj        = MneSurfaceOrVolume::mne_strdup(subj);
    pThis->surf_name   = MneSurfaceOrVolume::mne_strdup(name);

    char righthemi[17] = "Right hemisphere";
    decide_surface_extent(pThis,righthemi);
    decide_curv_display(name,pThis);
    setup_curvature_colors (pThis);

    apply_left_right_eyes(surfs);

    setup_current_surface_lights(surfs);

    return surfs;

bad : {
        FREE_47(left_file);
        FREE_47(right_file);
        delete left;
        delete right;
        FREE_47(this_surf);
        FREE_47(this_curv);
        return Q_NULLPTR;
    }
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::decide_surface_extent(MneMshDisplaySurface* surf,
                                                    char *tag)

{
    float minv[3],maxv[3];
    int k,c;
    float *r;
    MneSourceSpaceOld* s = (MneSourceSpaceOld*)surf->s;

    VEC_COPY_47(minv,s->rr[0]);
    VEC_COPY_47(maxv,s->rr[0]);
    for (k = 0; k < s->np; k++) {
        r = s->rr[k];
        for (c = 0; c < 3; c++) {
            if (r[c] < minv[c])
                minv[c] = r[c];
            if (r[c] > maxv[c])
                maxv[c] = r[c];
        }
    }
#ifdef DEBUG
    fprintf(stderr,"%s:\n",tag);
    fprintf(stderr,"\tx = %f ... %f mm\n",1000*minv[X],1000*maxv[X]);
    fprintf(stderr,"\ty = %f ... %f mm\n",1000*minv[Y],1000*maxv[Y]);
    fprintf(stderr,"\tz = %f ... %f mm\n",1000*minv[Z],1000*maxv[Z]);
#endif

    surf->fov = 0;
    for (c = 0; c < 3; c++) {
        if (fabs(minv[c]) > surf->fov)
            surf->fov = fabs(minv[c]);
        if (fabs(maxv[c]) > surf->fov)
            surf->fov = fabs(maxv[c]);
    }
    VEC_COPY_47(surf->minv,minv);
    VEC_COPY_47(surf->maxv,maxv);
    surf->fov_scale = 1.1f;
    return;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::decide_curv_display(char *name,
                MneMshDisplaySurface* s)

{
    if (strstr(name,"inflated") == name || strstr(name,"sphere") == name || strstr(name,"white") == name)
        s->curvature_color_mode = SHOW_CURVATURE_OVERLAY;
    else
        s->curvature_color_mode = SHOW_CURVATURE_NONE;
    s->overlay_color_mode = SHOW_OVERLAY_HEAT;
    /*
  s->overlay_color_mode = SHOW_OVERLAY_NEGPOS;
  */
    return;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::setup_curvature_colors(MneMshDisplaySurface* surf)
{
    int k,c;
    MneSourceSpaceOld* s = NULL;;
    float *col;
    float curv_sum;
    int   ncolor;

    if (surf == NULL || surf->s == NULL)
        return;

    s = (MneSourceSpaceOld*)surf->s;

    ncolor = surf->nvertex_colors;

    if (!surf->vertex_colors)
        surf->vertex_colors = MALLOC_47(ncolor*s->np,float);
    col = surf->vertex_colors;

    curv_sum = 0.0;
    if (surf->curvature_color_mode == SHOW_CURVATURE_OVERLAY) {
        for (k = 0; k < s->np; k++) {
            curv_sum += fabs(s->curv[k]);
            for (c = 0; c < 3; c++)
                col[c] = (s->curv[k] > 0) ? POS_CURV_COLOR : NEG_CURV_COLOR;
            if (ncolor == 4)
                col[3] = 1.0;
            col = col+ncolor;
        }
    }
    else {
        for (k = 0; k < s->np; k++) {
            curv_sum += fabs(s->curv[k]);
            for (c = 0; c < 3; c++)
                col[c] = EVEN_CURV_COLOR;
            if (ncolor == 4)
                col[3] = 1.0;
            col = col+ncolor;
        }
    }
#ifdef DEBUG
    fprintf(stderr,"Average curvature : %f\n",curv_sum/s->np);
#endif
    return;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::apply_left_right_eyes(MneMshDisplaySurfaceSet* surfs)
{
    MneMshEyes* eyes = Q_NULLPTR;
    MneMshDisplaySurface* surf = Q_NULLPTR;
    int k;

    if (surfs == NULL)
        return;

    if (neyes == 0 || current_eyes < 0 || current_eyes > neyes-1) {
        eyes = &default_eyes;
    } else {
        eyes = all_eyes+current_eyes;
    }

    for (k = 0; k < surfs->nsurf; k++) {
        surf = surfs->surfs[k];
        switch(surf->s->id) {
        case SURF_LEFT_HEMI :
        case SURF_LEFT_MORPH_HEMI :
            VEC_COPY_47(surf->eye,eyes->left);
            VEC_COPY_47(surf->up,eyes->left_up);
            break;
        case SURF_RIGHT_HEMI :
        case SURF_RIGHT_MORPH_HEMI :
            VEC_COPY_47(surf->eye,eyes->right);
            VEC_COPY_47(surf->up,eyes->right_up);
            break;
        default :
            VEC_COPY_47(surf->eye,eyes->left);
            VEC_COPY_47(surf->up,eyes->left_up);
            break;
        }
    }
    return;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::setup_current_surface_lights(MneMshDisplaySurfaceSet* surfs)
{
    if (!surfs)
        return;
    initialize_custom_lights();
    setup_these_surface_lights(surfs,custom_lights);
    return;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::initialize_custom_lights()
{
    if (!custom_lights) {
        mshLightSet s = new_light_set();
        s->nlight = ndefault;
        s->lights = default_lights;
        custom_lights = dup_light_set(s);
        free_light_set(s);
    }
}


//*************************************************************************************************************

mshLightSet MneMshDisplaySurfaceSet::dup_light_set(mshLightSet s)
{
    mshLightSet res = NULL;
    int k;

    if (s) {
        res = new_light_set();
        res->lights = MALLOC_47(s->nlight,mshLightRec);
        res->nlight = s->nlight;

        for (k = 0; k < s->nlight; k++)
            res->lights[k] = s->lights[k];
    }
    return res;
}


//*************************************************************************************************************

void MneMshDisplaySurfaceSet::setup_these_surface_lights(MneMshDisplaySurfaceSet* surfs, mshLightSet set)
{
    if (!surfs || !set)
        return;
    free_light_set(surfs->lights);
    surfs->lights = dup_light_set(set);
    return;
}

