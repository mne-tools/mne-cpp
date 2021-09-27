//=============================================================================================================
/**
 * @file     mne_msh_display_surface_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    Definition of the MneMshDisplaySurfaceSet Class.
 *
 */

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

#define REALLOC_47(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
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

static MNELIB::MneMshLightSet* custom_lights = Q_NULLPTR;

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qmath.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMshDisplaySurfaceSet::MneMshDisplaySurfaceSet(int nsurf)
{
    default_eyes.name = Q_NULLPTR;

    default_eyes.left[0] = -0.2f;
    default_eyes.left[0] = 0.0f;
    default_eyes.left[0] = 0.0f;

    default_eyes.right[0] = 0.2f;
    default_eyes.right[0] = 0.0f;
    default_eyes.right[0] = 0.0f;

    default_eyes.left_up[0] = 0.0f;
    default_eyes.left_up[0] = 0.0f;
    default_eyes.left_up[0] = 1.0f;

    default_eyes.right_up[0] = 0.0f;
    default_eyes.right_up[0] = 0.0f;
    default_eyes.right_up[0] = 1.0f;

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

//=============================================================================================================

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
    FREE_47(subj);
    FREE_47(morph_subj);
    FREE_47(active);
    FREE_47(drawable);

    delete lights;
    if (user_data_free)
        user_data_free(user_data);
}

//=============================================================================================================

MneMshDisplaySurfaceSet* MneMshDisplaySurfaceSet::load_new_surface(const QString &subject_id, const QString &surf, const QString &subjects_dir)
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
    QString pathLh, pathLhCurv, pathRh, pathRhCurv;
    QByteArray ba_surf, ba_curv;

    pathLh = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("lh").arg(surf);
    ba_surf = pathLh.toLatin1();
    this_surf = ba_surf.data();

    if (this_surf == Q_NULLPTR)
        goto bad;

    pathLhCurv = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("lh").arg("curv");
    ba_curv = pathLhCurv.toLatin1();
    this_curv = ba_curv.data();

    fprintf(stderr,"Loading surface %s ...\n",this_surf);
    if ((left = MneSurfaceOrVolume::mne_load_surface(this_surf,this_curv)) == Q_NULLPTR) {
        if ((left = MneSurfaceOrVolume::mne_load_surface(this_surf,Q_NULLPTR)) == Q_NULLPTR)
            goto bad;
        else
            MneSurfaceOrVolume::add_uniform_curv((MneSurfaceOld*)left);
    }
    left_file = this_surf; this_surf = Q_NULLPTR;
    FREE_47(this_curv);

    pathRh = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("rh").arg(surf);
    ba_surf = pathRh.toLatin1();
    this_surf = ba_surf.data();

    pathRhCurv = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("rh").arg("curv");
    ba_curv = pathRhCurv.toLatin1();
    this_curv = ba_curv.data();

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
    pThis->subj        = subject_id.toUtf8().data();
    pThis->surf_name   = surf.toUtf8().data();

    decide_surface_extent(pThis,"Left hemisphere");
    decide_curv_display(surf.toUtf8().data(),pThis);
    setup_curvature_colors (pThis);

    pThis              = surfs->surfs[1];
    pThis->filename    = right_file;
    //pThis->time_loaded = time(Q_NULLPTR); //Comment out due to unknown timestemp function ToDo
    pThis->s           = (MneSurfaceOld*)right;
    pThis->s->id       = SURF_RIGHT_HEMI;
    pThis->subj        = subject_id.toUtf8().data();
    pThis->surf_name   = surf.toUtf8().data();

    decide_surface_extent(pThis,"Right hemisphere");
    decide_curv_display(surf.toUtf8().data(),pThis);
    setup_curvature_colors (pThis);

    apply_left_right_eyes(surfs);

    setup_current_surface_lights(surfs);

    return surfs;

bad : {
        FREE_47(left_file);
        FREE_47(right_file);
        delete left;
        delete right;
        //The following deletes are obsolete since the two char* are point to data of the QStrings which are deleted automatically
//        FREE_47(this_surf);
//        FREE_47(this_curv);
        return Q_NULLPTR;
    }
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::decide_surface_extent(MneMshDisplaySurface* surf,
                                                    const char *tag)

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
        if (std::fabs(minv[c]) > surf->fov)
            surf->fov = std::fabs(minv[c]);
        if (std::fabs(maxv[c]) > surf->fov)
            surf->fov = std::fabs(maxv[c]);
    }
    VEC_COPY_47(surf->minv,minv);
    VEC_COPY_47(surf->maxv,maxv);
    surf->fov_scale = 1.1f;
    return;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::decide_curv_display(const char *name,
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

//=============================================================================================================

int MneMshDisplaySurfaceSet::add_bem_surface(MneMshDisplaySurfaceSet* surfs,
                                                QString              filepath,
                                                int                  kind,
                                                QString              bemname,
                                                int                  full_geom,
                                                int                  check)
{
    MneSurfaceOld*           surf = Q_NULLPTR;
    MneMshDisplaySurface*    newSurf = new MneMshDisplaySurface();

    //Transform from QString to char*
    QByteArray baFilepath = filepath.toLatin1();
    char* filename = baFilepath.data();

    QByteArray baBemname = bemname.toLatin1();
    char* name = baBemname.data();

    if (!surfs) {
        qWarning("Cannot add to nonexisting surface set.");
        goto bad;
    }

    fprintf(stderr,"Loading BEM surface %s (id = %d) from %s ...\n",name,kind,filename);
    if ((surf = MneSurfaceOld::mne_read_bem_surface2(filename,kind,full_geom,Q_NULLPTR)) == Q_NULLPTR)
        goto bad;
    if (check) {
        double sum;
        MneSurfaceOld::mne_compute_surface_cm(surf);
        sum = MneSurfaceOld::sum_solids(surf->cm,surf)/(4*M_PI);
        if (std::fabs(sum - 1.0) > 1e-4) {
            fprintf(stderr, "%s surface is not closed "
                                 "(sum of solid angles = %g * 4*PI).",name,sum);
            return FAIL;
        }
    }

    newSurf->filename    = MneSurfaceOld::mne_strdup(filename);
    //newSurf->time_loaded = time(Q_NULLPTR); //Comment out due to unknown timestemp function ToDo
    newSurf->s           = surf;
    newSurf->s->id       = kind;
    newSurf->subj        = Q_NULLPTR;
    newSurf->surf_name   = MneSurfaceOld::mne_strdup(name);

    newSurf->curvature_color_mode = SHOW_CURVATURE_NONE;
    newSurf->overlay_color_mode   = SHOW_OVERLAY_HEAT;

    decide_surface_extent(newSurf,name);
    add_replace_display_surface(surfs, newSurf, true, true);
    apply_left_eyes(surfs);
    setup_current_surface_lights(surfs);

    return OK;

bad : {
        delete surf;
        return FAIL;
    }
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::add_replace_display_surface(MneMshDisplaySurfaceSet* surfs,
                                                          MneMshDisplaySurface*    newSurf,
                                                          bool                  replace,
                                                          bool                  drawable)
{
    int k;
    MneMshDisplaySurface* surf = new MneMshDisplaySurface();

    if (replace) {
        for (k = 0; k < surfs->nsurf; k++) {
            surf = surfs->surfs[k];
            if (surf->s->id == newSurf->s->id) {
                newSurf->transparent   = surf->transparent;
                newSurf->show_aux_data = surf->show_aux_data;
                delete surf;
                surfs->surfs[k] = newSurf;
                if (!drawable) {
                    surfs->active[k]   = FALSE;
                    surfs->drawable[k] = FALSE;
                }
                newSurf             = Q_NULLPTR;
                break;
            }
        }
    }
    if (newSurf) {		/* New surface */
        surfs->surfs     = REALLOC_47(surfs->surfs,surfs->nsurf+1,MneMshDisplaySurface*);
        surfs->patches   = REALLOC_47(surfs->patches,surfs->nsurf+1,MneSurfacePatch*);
        surfs->patch_rot = REALLOC_47(surfs->patch_rot,surfs->nsurf+1,float);
        surfs->active    = REALLOC_47(surfs->active,surfs->nsurf+1,int);
        surfs->drawable  = REALLOC_47(surfs->drawable,surfs->nsurf+1,int);
        surfs->surfs[surfs->nsurf]     = newSurf;
        surfs->active[surfs->nsurf]    = drawable;
        surfs->drawable[surfs->nsurf]  = drawable;
        surfs->patches[surfs->nsurf]   = NULL;
        surfs->patch_rot[surfs->nsurf] = 0.0;
        surfs->nsurf++;
    }
    return;
}

//=============================================================================================================

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
            curv_sum += std::fabs(s->curv[k]);
            for (c = 0; c < 3; c++)
                col[c] = (s->curv[k] > 0) ? POS_CURV_COLOR : NEG_CURV_COLOR;
            if (ncolor == 4)
                col[3] = 1.0;
            col = col+ncolor;
        }
    }
    else {
        for (k = 0; k < s->np; k++) {
            curv_sum += std::fabs(s->curv[k]);
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

//=============================================================================================================

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

//=============================================================================================================

void MneMshDisplaySurfaceSet::apply_left_eyes(MneMshDisplaySurfaceSet* surfs)
{
    int k;

    if (surfs == Q_NULLPTR)
        return;

    for (k = 0; k < surfs->nsurf; k++) {
        if (neyes == 0 || current_eyes < 0 || current_eyes > neyes-1) {
            VEC_COPY_47(surfs->surfs[k]->eye,default_eyes.left);
            VEC_COPY_47(surfs->surfs[k]->up,default_eyes.left_up);
        }
        else {
            VEC_COPY_47(surfs->surfs[k]->eye,all_eyes[current_eyes].left);
            VEC_COPY_47(surfs->surfs[k]->up,all_eyes[current_eyes].left_up);
        }
    }
    return;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::setup_current_surface_lights(MneMshDisplaySurfaceSet* surfs)
{
    if (!surfs)
        return;
    initialize_custom_lights();
    setup_these_surface_lights(surfs,custom_lights);
    return;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::initialize_custom_lights()
{
    if (!custom_lights) {
        MneMshLightSet* s = new MneMshLightSet();
        s->nlight = ndefault;

        QList<MneMshLight*> default_lights;
        default_lights << new MneMshLight(TRUE, 0.0f, 0.0f,  1.0f, 0.8f, 0.8f, 0.8f);
        default_lights << new MneMshLight(TRUE, 0.0f, 0.0f, -1.0f, 0.8f, 0.8f, 0.8f);
        default_lights << new MneMshLight(TRUE, 0.6f, -1.0f, -1.0f, 0.6f, 0.6f, 0.6f);
        default_lights << new MneMshLight(TRUE, -0.6f, -1.0f, -1.0f, 0.6f, 0.6f, 0.6f);
        default_lights << new MneMshLight(TRUE, 1.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f);
        default_lights << new MneMshLight(TRUE, -1.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f);
        default_lights << new MneMshLight(TRUE, 0.0f, 1.0f, 0.5f, 0.6f, 0.6f, 0.6f);
        default_lights << new MneMshLight(FALSE, 0.0f, 0.0f, -1.0, 1.0f, 1.0f, 1.0f);

        s->lights = default_lights;

        custom_lights = dup_light_set(s);
        delete s;
    }
}

//=============================================================================================================

MneMshLightSet* MneMshDisplaySurfaceSet::dup_light_set(MneMshLightSet* s)
{
    MneMshLightSet* res = Q_NULLPTR;
    int k;

    if (s) {
        res = new MneMshLightSet();
        //res->lights = MALLOC_47(s->nlight,mshLightRec);
        res->nlight = s->nlight;

        for (k = 0; k < s->nlight; k++)
            res->lights.append(new MneMshLight(*s->lights[k]));
    }
    return res;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::setup_these_surface_lights(MneMshDisplaySurfaceSet* surfs, MneMshLightSet* set)
{
    if (!surfs || !set)
        return;
    delete surfs->lights;
    surfs->lights = Q_NULLPTR;
    surfs->lights = dup_light_set(set);
    return;
}

