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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qmath.h>

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace {

constexpr int  SURF_LEFT_HEMI        = FIFFV_MNE_SURF_LEFT_HEMI;
constexpr int  SURF_RIGHT_HEMI       = FIFFV_MNE_SURF_RIGHT_HEMI;
constexpr int  SURF_LEFT_MORPH_HEMI  = (1 << 16 | FIFFV_MNE_SURF_LEFT_HEMI);
constexpr int  SURF_RIGHT_MORPH_HEMI = (1 << 16 | FIFFV_MNE_SURF_RIGHT_HEMI);

constexpr int  SHOW_CURVATURE_NONE    = 0;
constexpr int  SHOW_CURVATURE_OVERLAY = 1;
constexpr int  SHOW_OVERLAY_HEAT      = 1;

constexpr float POS_CURV_COLOR  = 0.25f;
constexpr float NEG_CURV_COLOR  = 0.375f;
constexpr float EVEN_CURV_COLOR = 0.375f;

} // anonymous namespace

//=============================================================================================================
// STATIC DATA
//=============================================================================================================

static MNELIB::MneMshEyes   default_eyes;
static MNELIB::MneMshEyes*  all_eyes     = nullptr;
static int          neyes        = 0;
static int          current_eyes = -1;

static std::unique_ptr<MNELIB::MneMshLightSet> custom_lights;

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMshDisplaySurfaceSet::MneMshDisplaySurfaceSet(int nsurf)
{
    Eigen::Map<Eigen::Vector3f>(default_eyes.left)    = Eigen::Vector3f(-0.2f, 0.0f, 0.0f);
    Eigen::Map<Eigen::Vector3f>(default_eyes.right)   = Eigen::Vector3f( 0.2f, 0.0f, 0.0f);
    Eigen::Map<Eigen::Vector3f>(default_eyes.left_up)  = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
    Eigen::Map<Eigen::Vector3f>(default_eyes.right_up) = Eigen::Vector3f(0.0f, 0.0f, 1.0f);

    this->nsurf = nsurf;
    if (nsurf > 0) {
        surfs.resize(nsurf);
        patches.resize(nsurf);
        patch_rot.resize(nsurf, 0.0f);
        active = Eigen::VectorXi::Zero(nsurf);
        drawable = Eigen::VectorXi::Ones(nsurf);
    }

    use_patches = false;

    Eigen::Vector3f::Map(rot)  = Eigen::Vector3f::Zero();
    Eigen::Vector3f::Map(move) = Eigen::Vector3f::Zero();
    Eigen::Vector3f::Map(eye)  = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
    Eigen::Vector3f::Map(up)   = Eigen::Vector3f(0.0f, 0.0f, 1.0f);

    Eigen::Vector3f::Map(bg_color)   = Eigen::Vector3f::Zero();
    Eigen::Vector3f::Map(text_color) = Eigen::Vector3f::Ones();
}

//=============================================================================================================

MneMshDisplaySurfaceSet::~MneMshDisplaySurfaceSet() = default;

//=============================================================================================================

std::unique_ptr<MneMshDisplaySurfaceSet> MneMshDisplaySurfaceSet::load(const QString &subject_id, const QString &surf, const QString &subjects_dir)
     /*
      * Load new display surface data
      */
{
    QString pathLh     = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("lh").arg(surf);
    QString pathLhCurv = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("lh").arg("curv");
    QString pathRh     = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("rh").arg(surf);
    QString pathRhCurv = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg("rh").arg("curv");

    printf("Loading surface %s ...\n", pathLh.toUtf8().constData());
    std::unique_ptr<MneSourceSpaceOld> left(MneSurfaceOrVolume::load_surface(pathLh, pathLhCurv));
    if (!left) {
        left.reset(MneSurfaceOrVolume::load_surface(pathLh, QString()));
        if (!left)
            return nullptr;
        MneSurfaceOrVolume::add_uniform_curv(reinterpret_cast<MneSurfaceOld&>(*left));
    }

    printf("Loading surface %s ...\n", pathRh.toUtf8().constData());
    std::unique_ptr<MneSourceSpaceOld> right(MneSurfaceOrVolume::load_surface(pathRh, pathRhCurv));
    if (!right) {
        right.reset(MneSurfaceOrVolume::load_surface(pathRh, QString()));
        if (!right)
            return nullptr;
        MneSurfaceOrVolume::add_uniform_curv(reinterpret_cast<MneSurfaceOld&>(*right));
    }

    auto result = std::make_unique<MneMshDisplaySurfaceSet>(2);

    result->surfs[0] = std::make_unique<MneMshDisplaySurface>();
    result->surfs[1] = std::make_unique<MneMshDisplaySurface>();

    result->active[0]  = true;
    result->active[1]  = false;
    result->drawable[0]  = true;
    result->drawable[1]  = true;

    auto* pThis        = result->surfs[0].get();
    pThis->filename    = pathLh;
    pThis->s.reset(reinterpret_cast<MneSurfaceOld*>(left.release()));
    pThis->s->id       = SURF_LEFT_HEMI;
    pThis->subj        = subject_id;
    pThis->surf_name   = surf;

    decide_surface_extent(*pThis,"Left hemisphere");
    decide_curv_display(surf, *pThis);
    setup_curvature_colors(*pThis);

    pThis              = result->surfs[1].get();
    pThis->filename    = pathRh;
    pThis->s.reset(reinterpret_cast<MneSurfaceOld*>(right.release()));
    pThis->s->id       = SURF_RIGHT_HEMI;
    pThis->subj        = subject_id;
    pThis->surf_name   = surf;

    decide_surface_extent(*pThis,"Right hemisphere");
    decide_curv_display(surf, *pThis);
    setup_curvature_colors(*pThis);

    result->apply_left_right_eyes();
    result->setup_current_lights();

    return result;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::decide_surface_extent(MneMshDisplaySurface& surf,
                                                    const QString& tag)

{
    auto* s = reinterpret_cast<MneSourceSpaceOld*>(surf.s.get());

    Eigen::Vector3f minv = s->rr.row(0).transpose();
    Eigen::Vector3f maxv = minv;

    for (int k = 1; k < s->np; k++) {
        Eigen::Vector3f r = s->rr.row(k).transpose();
        minv = minv.cwiseMin(r);
        maxv = maxv.cwiseMax(r);
    }

#ifdef DEBUG
    printf("%s:\n",tag.toUtf8().constData());
    printf("\tx = %f ... %f mm\n",1000*minv[0],1000*maxv[0]);
    printf("\ty = %f ... %f mm\n",1000*minv[1],1000*maxv[1]);
    printf("\tz = %f ... %f mm\n",1000*minv[2],1000*maxv[2]);
#endif

    surf.fov = std::max(minv.cwiseAbs().maxCoeff(), maxv.cwiseAbs().maxCoeff());

    surf.minv = minv;
    surf.maxv = maxv;
    surf.fov_scale = 1.1f;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::decide_curv_display(const QString& name,
                MneMshDisplaySurface& s)

{
    if (name.startsWith("inflated") || name.startsWith("sphere") || name.startsWith("white"))
        s.curvature_color_mode = SHOW_CURVATURE_OVERLAY;
    else
        s.curvature_color_mode = SHOW_CURVATURE_NONE;
    s.overlay_color_mode = SHOW_OVERLAY_HEAT;
}

//=============================================================================================================

int MneMshDisplaySurfaceSet::add_bem_surface(const QString&       filepath,
                                                int                  kind,
                                                const QString&       bemname,
                                                int                  full_geom,
                                                int                  check)
{
    printf("Loading BEM surface %s (id = %d) from %s ...\n",
           bemname.toUtf8().constData(), kind, filepath.toUtf8().constData());

    std::unique_ptr<MneSurfaceOld> surf(MneSurfaceOld::read_bem_surface2(filepath,kind,full_geom,nullptr));
    if (!surf)
        return -1;

    if (check) {
        MneSurfaceOld::compute_surface_cm(*surf);
        double sum = MneSurfaceOld::sum_solids(Eigen::Map<const Eigen::Vector3f>(surf->cm), *surf) / (4*M_PI);
        if (std::fabs(sum - 1.0) > 1e-4) {
            printf( "%s surface is not closed "
                                 "(sum of solid angles = %g * 4*PI).",
                   bemname.toUtf8().constData(), sum);
            return -1;
        }
    }

    auto newSurf = std::make_unique<MneMshDisplaySurface>();
    newSurf->filename    = filepath;
    //newSurf->time_loaded = time(nullptr); //Comment out due to unknown timestamp function ToDo
    newSurf->s.reset(surf.release());
    newSurf->s->id       = kind;
    newSurf->surf_name   = bemname;

    newSurf->curvature_color_mode = SHOW_CURVATURE_NONE;
    newSurf->overlay_color_mode   = SHOW_OVERLAY_HEAT;

    decide_surface_extent(*newSurf, bemname);
    add_replace_surface(std::move(newSurf), true, true);
    apply_left_eyes();
    setup_current_lights();

    return 0;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::add_replace_surface(std::unique_ptr<MneMshDisplaySurface> newSurf,
                                                   bool                  replace,
                                                   bool                  drawable)
{
    if (replace) {
        for (int k = 0; k < nsurf; k++) {
            auto& surf = surfs[k];
            if (surf->s->id == newSurf->s->id) {
                newSurf->transparent   = surf->transparent;
                newSurf->show_aux_data = surf->show_aux_data;
                surfs[k] = std::move(newSurf);
                if (!drawable) {
                    active[k]   = false;
                    this->drawable[k] = false;
                }
                return;
            }
        }
    }
    if (newSurf) {		/* New surface */
        surfs.push_back(std::move(newSurf));
        patches.push_back(nullptr);
        patch_rot.push_back(0.0f);
        active.conservativeResize(nsurf+1);
        this->drawable.conservativeResize(nsurf+1);
        active[nsurf]    = drawable;
        this->drawable[nsurf]  = drawable;
        nsurf++;
    }
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::setup_curvature_colors(MneMshDisplaySurface& surf)
{
    if (!surf.s)
        return;

    auto* s = reinterpret_cast<MneSourceSpaceOld*>(surf.s.get());

    const int ncolor = surf.nvertex_colors;
    const int totalSize = ncolor * s->np;

    if (surf.vertex_colors.size() == 0)
        surf.vertex_colors.resize(totalSize);

    float curv_sum = 0.0f;
    if (surf.curvature_color_mode == SHOW_CURVATURE_OVERLAY) {
        for (int k = 0; k < s->np; k++) {
            const int base = k * ncolor;
            curv_sum += std::fabs(s->curv[k]);
            const float c = (s->curv[k] > 0) ? POS_CURV_COLOR : NEG_CURV_COLOR;
            for (int j = 0; j < 3; j++)
                surf.vertex_colors[base + j] = c;
            if (ncolor == 4)
                surf.vertex_colors[base + 3] = 1.0f;
        }
    }
    else {
        for (int k = 0; k < s->np; k++) {
            const int base = k * ncolor;
            curv_sum += std::fabs(s->curv[k]);
            for (int j = 0; j < 3; j++)
                surf.vertex_colors[base + j] = EVEN_CURV_COLOR;
            if (ncolor == 4)
                surf.vertex_colors[base + 3] = 1.0f;
        }
    }
#ifdef DEBUG
    printf("Average curvature : %f\n",curv_sum/s->np);
#endif
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::apply_left_right_eyes()
{
    MneMshEyes* eyes = nullptr;

    if (neyes == 0 || current_eyes < 0 || current_eyes > neyes-1) {
        eyes = &default_eyes;
    } else {
        eyes = all_eyes+current_eyes;
    }

    for (int k = 0; k < nsurf; k++) {
        auto* surf = surfs[k].get();
        switch(surf->s->id) {
        case SURF_LEFT_HEMI :
        case SURF_LEFT_MORPH_HEMI :
            surf->eye = Eigen::Vector3f::Map(eyes->left);
            surf->up  = Eigen::Vector3f::Map(eyes->left_up);
            break;
        case SURF_RIGHT_HEMI :
        case SURF_RIGHT_MORPH_HEMI :
            surf->eye = Eigen::Vector3f::Map(eyes->right);
            surf->up  = Eigen::Vector3f::Map(eyes->right_up);
            break;
        default :
            surf->eye = Eigen::Vector3f::Map(eyes->left);
            surf->up  = Eigen::Vector3f::Map(eyes->left_up);
            break;
        }
    }
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::apply_left_eyes()
{
    for (int k = 0; k < nsurf; k++) {
        if (neyes == 0 || current_eyes < 0 || current_eyes > neyes-1) {
            surfs[k]->eye = Eigen::Vector3f::Map(default_eyes.left);
            surfs[k]->up  = Eigen::Vector3f::Map(default_eyes.left_up);
        }
        else {
            surfs[k]->eye = Eigen::Vector3f::Map(all_eyes[current_eyes].left);
            surfs[k]->up  = Eigen::Vector3f::Map(all_eyes[current_eyes].left_up);
        }
    }
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::setup_current_lights()
{
    initialize_custom_lights();
    setup_lights(*custom_lights);
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::initialize_custom_lights()
{
    if (!custom_lights) {
        custom_lights = std::make_unique<MneMshLightSet>();

        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, 0.0f, 0.0f,  1.0f, 0.8f, 0.8f, 0.8f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, 0.0f, 0.0f, -1.0f, 0.8f, 0.8f, 0.8f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, 0.6f, -1.0f, -1.0f, 0.6f, 0.6f, 0.6f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, -0.6f, -1.0f, -1.0f, 0.6f, 0.6f, 0.6f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, 1.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, -1.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(true, 0.0f, 1.0f, 0.5f, 0.6f, 0.6f, 0.6f));
        custom_lights->lights.push_back(std::make_unique<MneMshLight>(false, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f));
    }
}

//=============================================================================================================

std::unique_ptr<MneMshLightSet> MneMshDisplaySurfaceSet::dup_light_set(const MneMshLightSet& s)
{
    auto res = std::make_unique<MneMshLightSet>();

    for (const auto &light : s.lights)
        res->lights.push_back(std::make_unique<MneMshLight>(*light));

    return res;
}

//=============================================================================================================

void MneMshDisplaySurfaceSet::setup_lights(const MneMshLightSet& set)
{
    lights = dup_light_set(set);
}

