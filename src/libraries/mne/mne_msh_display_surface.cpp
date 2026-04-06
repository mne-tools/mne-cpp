//=============================================================================================================
/**
 * @file     mne_msh_display_surface.cpp
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
 * @brief    Definition of the MNEMshDisplaySurface Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_msh_display_surface.h"
#include "mne_surface.h"
#include "mne_morph_map.h"
#include "mne_msh_picked.h"
#include "mne_msh_color_scale_def.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_dig_point.h>

#include <math/sphere.h>

#define _USE_MATH_DEFINES
#include <math.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//============================= local macros =================================

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

constexpr int  SHOW_CURVATURE_NONE    = 0;
constexpr int  SHOW_CURVATURE_OVERLAY = 1;
constexpr int  SHOW_OVERLAY_HEAT      = 1;

constexpr float POS_CURV_COLOR  = 0.25f;
constexpr float NEG_CURV_COLOR  = 0.375f;
constexpr float EVEN_CURV_COLOR = 0.375f;

#define X_17 0
#define Y_17 1
#define Z_17 2

#define VEC_DOT_17(x,y) ((x)[X_17]*(y)[X_17] + (x)[Y_17]*(y)[Y_17] + (x)[Z_17]*(y)[Z_17])

#define VEC_LEN_17(x) sqrt(VEC_DOT_17(x,x))

#define VEC_DIFF_17(from,to,diff) {\
    (diff)[X_17] = (to)[X_17] - (from)[X_17];\
    (diff)[Y_17] = (to)[Y_17] - (from)[Y_17];\
    (diff)[Z_17] = (to)[Z_17] - (from)[Z_17];\
    }

#define VEC_COPY_17(to,from) {\
    (to)[X_17] = (from)[X_17];\
    (to)[Y_17] = (from)[Y_17];\
    (to)[Z_17] = (from)[Z_17];\
    }

#define CROSS_PRODUCT_17(x,y,xy) {\
    (xy)[X_17] =   (x)[Y_17]*(y)[Z_17]-(y)[Y_17]*(x)[Z_17];\
    (xy)[Y_17] = -((x)[X_17]*(y)[Z_17]-(y)[X_17]*(x)[Z_17]);\
    (xy)[Z_17] =   (x)[X_17]*(y)[Y_17]-(y)[X_17]*(x)[Y_17];\
    }

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMshDisplaySurface::MNEMshDisplaySurface() = default;

//=============================================================================================================

MNEMshDisplaySurface::~MNEMshDisplaySurface()
{
    if (user_data_free)
        user_data_free(user_data);
}

//=============================================================================================================
// Alignment functions (moved from MNESurfaceOrVolume)
//=============================================================================================================

int MNEMshDisplaySurface::align_fiducials(FiffDigitizerData& head_dig,
                                        const FiffDigitizerData& mri_dig,
                                        int niter,
                                        int scale_head,
                                        float omit_dist,
                                        Eigen::Vector3f& scales)

{
    using FidMatrix = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
    FidMatrix           head_fid, mri_fid;
    bool                head_fid_found[3] = {false, false, false};
    bool                mri_fid_found[3]  = {false, false, false};
    int             j,k;
    FiffDigPoint    p;
    float          nasion_weight = 5.0;

    for (j = 0; j < 2; j++) {
        const FiffDigitizerData& d = (j == 0) ? head_dig : mri_dig;
        FidMatrix&         fid  = (j == 0) ? head_fid : mri_fid;
        bool*              found = (j == 0) ? head_fid_found : mri_fid_found;

        for (k = 0; k < d.npoint; k++) {
            p = d.points[k];
            if (p.kind == FIFFV_POINT_CARDINAL) {
                if (p.ident == FIFFV_POINT_LPA) {
                    fid.row(0) = Eigen::Map<const Eigen::RowVector3f>(d.points[k].r);
                    found[0] = true;
                }
                else if (p.ident == FIFFV_POINT_NASION) {
                    fid.row(1) = Eigen::Map<const Eigen::RowVector3f>(d.points[k].r);
                    found[1] = true;
                }
                else if (p.ident == FIFFV_POINT_RPA) {
                    fid.row(2) = Eigen::Map<const Eigen::RowVector3f>(d.points[k].r);
                    found[2] = true;
                }
            }
        }
    }

    for (k = 0; k < 3; k++) {
        if (!head_fid_found[k]) {
            qCritical("Some of the MEG fiducials were missing");
            goto bad;
        }

        if (!mri_fid_found[k]) {
            qCritical("Some of the MRI fiducials were missing");
            goto bad;
        }
    }

    if (scale_head) {
        get_head_scale(head_dig,mri_fid,scales);
        printf("xscale = %.3f yscale = %.3f zscale = %.3f\n",scales[0],scales[1],scales[2]);

        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                mri_fid(j,k) = mri_fid(j,k)*scales[k];

        scale(scales);
    }

    // Initial alignment
    head_dig.head_mri_t_adj = std::make_unique<FiffCoordTrans>(FIFFLIB::FiffCoordTrans::fromCardinalPoints(FIFFV_COORD_HEAD,FIFFV_COORD_MRI,
                                                                                    mri_fid.row(0).data(),mri_fid.row(1).data(),mri_fid.row(2).data()));

    // Populate mri_fids from cardinal digitizer points transformed into MRI coords
    head_dig.pickCardinalFiducials();

    // Overwrite the fiducial locations with the ones from the MRI digitizer data
    for (k = 0; k < head_dig.nfids(); k++)
        VEC_COPY_17(head_dig.mri_fids[k].r,mri_fid.row(k).data());
    head_dig.head_mri_t_adj->print();
    printf("After simple alignment : \n");

    if (omit_dist > 0)
        discard_outlier_digitizer_points(head_dig,omit_dist);

    // Optional iterative refinement
    if (niter > 0) {
        for (k = 0; k < niter; k++) {
            if (iterate_alignment_once(head_dig,nasion_weight,Eigen::Vector3f(mri_fid.row(1).transpose()),k == niter-1 && niter > 1) == FAIL)
                goto bad;
        }

        printf("%d / %d iterations done. RMS dist = %7.1f mm\n",k,niter,
                1000.0*rms_digitizer_distance(head_dig));
        printf("After refinement :\n");
        head_dig.head_mri_t_adj->print();
    }

    return OK;

bad :
    return FAIL;
}

//=============================================================================================================

// Simple head size fit
void MNEMshDisplaySurface::get_head_scale(FIFFLIB::FiffDigitizerData& dig,
                                        const Eigen::Matrix<float, 3, 3, Eigen::RowMajor>& mri_fid,
                                        Eigen::Vector3f& scales)
{
    float **dig_rr  = NULL;
    float **head_rr = NULL;
    int   k,ndig,nhead;
    float simplex_size = 2e-2;
    float r0[3],Rdig,Rscalp;
    float LR[3],LN[3],len,norm[3],diff[3];

    scales[0] = scales[1] = scales[2] = 1.0;

    dig_rr  = new float*[dig.npoint];
    head_rr = new float*[np];

    // Pick only the points with positive z
    for (k = 0, ndig = 0; k < dig.npoint; k++) {
        if (dig.points[k].r[Z_17] > 0) {
            dig_rr[ndig++] = dig.points[k].r;
        }
    }

    if (!UTILSLIB::Sphere::fit_sphere_to_points(dig_rr,ndig,simplex_size,r0,&Rdig)){
        goto out;
    }

    printf("Polhemus : (%.1f %.1f %.1f) mm R = %.1f mm\n",1000*r0[X_17],1000*r0[Y_17],1000*r0[Z_17],1000*Rdig);

    // Pick only the points above the fiducial plane
    VEC_DIFF_17(mri_fid.row(0).data(),mri_fid.row(2).data(),LR);
    VEC_DIFF_17(mri_fid.row(0).data(),mri_fid.row(1).data(),LN);
    CROSS_PRODUCT_17(LR,LN,norm);
    len = VEC_LEN_17(norm);
    norm[0] = norm[0]/len;
    norm[1] = norm[1]/len;
    norm[2] = norm[2]/len;

    for (k = 0, nhead = 0; k < np; k++) {
        VEC_DIFF_17(mri_fid.row(0).data(),&rr(k,0),diff);
        if (VEC_DOT_17(diff,norm) > 0) {
            head_rr[nhead++] = &rr(k,0);
        }
    }

    if (!UTILSLIB::Sphere::fit_sphere_to_points(head_rr,nhead,simplex_size,r0,&Rscalp)) {
        goto out;
    }

    printf("Scalp : (%.1f %.1f %.1f) mm R = %.1f mm\n",1000*r0[X_17],1000*r0[Y_17],1000*r0[Z_17],1000*Rscalp);

    scales[0] = scales[1] = scales[2] = Rdig/Rscalp;

out : {
        delete[] dig_rr;
        delete[] head_rr;
        return;
    }
}

//=============================================================================================================

int MNEMshDisplaySurface::discard_outlier_digitizer_points(FIFFLIB::FiffDigitizerData& d,
                                                         float maxdist) const
/*
      * Discard outlier digitizer points
      */
{
    int discarded = 0;
    int k;

    d.dist_valid = false;
    calculate_digitizer_distances(d,TRUE,TRUE);
    for (k = 0; k < d.npoint; k++) {
        d.discard[k] = 0;
        /*
        * Discard unless cardinal landmark or HPI coil
        */
        if (std::fabs(d.dist(k)) > maxdist &&
                d.points[k].kind != FIFFV_POINT_CARDINAL &&
                d.points[k].kind != FIFFV_POINT_HPI) {
            discarded++;
            d.discard[k] = 1;
        }
    }
    printf("%d points discarded (maxdist = %6.1f mm).\n",discarded,1000*maxdist);

    return discarded;
}

//=============================================================================================================

void MNEMshDisplaySurface::calculate_digitizer_distances(FIFFLIB::FiffDigitizerData& dig,
                                                       int do_all, int do_approx) const
/*
 * Calculate the distances from the scalp surface
 */
{
    int                 k,nactive;
    FiffDigPoint        point;
    Q_ASSERT(dig.head_mri_t);
    const FiffCoordTrans& t = (dig.head_mri_t_adj && !dig.head_mri_t_adj->isEmpty()) ? *dig.head_mri_t_adj : *dig.head_mri_t;
    int                 nstep = 4;

    if (dig.dist_valid)
        return ;

    PointsT rr(dig.npoint, 3);

    dig.dist.conservativeResize(dig.npoint);
    if (dig.closest.size() == 0) {
        /*
        * Ensure that all closest values are initialized correctly
        */
        dig.closest = Eigen::VectorXi::Constant(dig.npoint, -1);
    }

    dig.closest_point.setZero(dig.npoint,3);
    Eigen::VectorXi closest(dig.npoint);
    Eigen::VectorXf dists(dig.npoint);

    for (k = 0, nactive = 0; k < dig.npoint; k++) {
        if ((dig.active[k] && !dig.discard[k]) || do_all) {
            point = dig.points.at(k);
            rr.row(nactive) = Eigen::Map<const Eigen::RowVector3f>(point.r);
            FiffCoordTrans::apply_trans(rr.row(nactive).data(),t,FIFFV_MOVE);
            if (do_approx) {
                closest[nactive] = dig.closest(k);
                if (closest[nactive] < 0)
                    do_approx = FALSE;
            }
            else
                closest[nactive] = -1;
            nactive++;
        }
    }

    find_closest_on_surface_approx(rr,nactive,closest,dists,nstep);
    /*
     * Project the points on the triangles
     */
    if (!do_approx)
        printf("Inside or outside for %d points...",nactive);
    for (k = 0, nactive = 0; k < dig.npoint; k++) {
        if ((dig.active[k] && !dig.discard[k]) || do_all) {
            dig.dist(k)    = dists[nactive];
            dig.closest(k) = closest[nactive];
            {
                Eigen::Vector3f pt = Eigen::Map<const Eigen::Vector3f>(rr.row(nactive).data());
                Eigen::Vector3f proj = project_to_triangle(dig.closest(k),pt);
                dig.closest_point.row(k) = proj.transpose();
            }
            /*
            * The above distance is with respect to the closest triangle only
            * We need to use the solid angle criterion to decide the sign reliably
            */
            if (!do_approx && false) {
                Eigen::Vector3f pt = Eigen::Map<const Eigen::Vector3f>(rr.row(nactive).data());
                if (sum_solids(pt)/(4*M_PI) > 0.9)
                    dig.dist(k) = - std::fabs(dig.dist(k));
                else
                    dig.dist(k) = std::fabs(dig.dist(k));
            }
            nactive++;
        }
    }

    if (!do_approx)
        printf("[done]\n");

    dig.dist_valid = true;

    return;
}

//=============================================================================================================

int MNEMshDisplaySurface::iterate_alignment_once(FIFFLIB::FiffDigitizerData& dig,	   /* The digitizer data */
                                               int nasion_weight,	   /* Weight for the nasion */
                                               const std::optional<Eigen::Vector3f>& nasion_mri,   /* Fixed correspondence point for the nasion (optional) */
                                               int last_step) const         /* Is this the last iteration step */
/*
 * Find the best alignment of the coordinate frames
 */
{
    int   res       = FAIL;
    Eigen::MatrixXf rr_head(dig.npoint, 3);
    Eigen::MatrixXf rr_mri(dig.npoint, 3);
    Eigen::VectorXf w = Eigen::VectorXf::Zero(dig.npoint);
    int             k,nactive;
    FiffDigPoint    point;
    FiffCoordTrans t;
    float           max_diff = 40e-3;

    if (!dig.head_mri_t_adj) {
        qCritical()<<"Not adjusting the transformation";
        goto out;
    }
    /*
     * Calculate initial distances
     */
    calculate_digitizer_distances(dig,FALSE,TRUE);

    for (k = 0, nactive = 0; k < dig.npoint; k++) {
        if (dig.active[k] && !dig.discard[k]) {
            point = dig.points.at(k);
            rr_head.row(nactive) = Eigen::Map<const Eigen::RowVector3f>(point.r);
            rr_mri.row(nactive) = dig.closest_point.row(k);
            /*
            * Special handling for the nasion
            */
            if (point.kind == FIFFV_POINT_CARDINAL &&
                    point.ident == FIFFV_POINT_NASION) {
                w[nactive] = nasion_weight;
                if (nasion_mri) {
                    rr_mri.row(nactive) = nasion_mri->transpose();
                    rr_head.row(nactive) = nasion_mri->transpose();
                    Q_ASSERT(dig.head_mri_t || dig.head_mri_t_adj);
                    FiffCoordTrans::apply_inverse_trans(rr_head.row(nactive).data(),
                                                            dig.head_mri_t_adj ? *dig.head_mri_t_adj : *dig.head_mri_t,
                                                            FIFFV_MOVE);
                }
            }
            else
                w[nactive] = 1.0;
            nactive++;
        }
    }
    if (nactive < 3) {
        qCritical() << "Not enough points to do the alignment";
        goto out;
    }
    if ((t = FiffCoordTrans::procrustesAlign(FIFFV_COORD_HEAD, FIFFV_COORD_MRI,
                                                 rr_head.topRows(nactive),
                                                 rr_mri.topRows(nactive),
                                                 w.head(nactive),
                                                 max_diff)).isEmpty())
        goto out;

    if (dig.head_mri_t_adj)
        dig.head_mri_t_adj = std::make_unique<FiffCoordTrans>(t);
    /*
     * Calculate final distances
     */
    dig.dist_valid = false;
    calculate_digitizer_distances(dig,FALSE,!last_step);
    res = OK;

out : {
        return res;
    }
}

//=============================================================================================================

float MNEMshDisplaySurface::rms_digitizer_distance(FIFFLIB::FiffDigitizerData& dig) const
{
    float rms;
    int   k,nactive;

    calculate_digitizer_distances(dig,FALSE,TRUE);

    for (k = 0, rms = 0.0, nactive = 0; k < dig.npoint; k++)
        if (dig.active[k] && !dig.discard[k]) {
            rms = rms + dig.dist(k)*dig.dist(k);
            nactive++;
        }
    if (nactive > 1)
        rms = rms/(nactive-1);
    return sqrt(rms);
}

//=============================================================================================================

void MNEMshDisplaySurface::scale(const Eigen::Vector3f& scales)
/*
 * Not quite complete yet
 */
{
    int j,k;

    for (k = 0; k < 3; k++) {
        minv[k] = scales[k]*minv[k];
        maxv[k] = scales[k]*maxv[k];
    }
    for (j = 0; j < np; j++)
        for (k = 0; k < 3; k++)
            rr(j,k) = rr(j,k)*scales[k];
    return;
}

//=============================================================================================================

void MNEMshDisplaySurface::decide_surface_extent(const QString& tag)
{
    Eigen::Vector3f mn = rr.row(0).transpose();
    Eigen::Vector3f mx = mn;

    for (int k = 1; k < np; k++) {
        Eigen::Vector3f r = rr.row(k).transpose();
        mn = mn.cwiseMin(r);
        mx = mx.cwiseMax(r);
    }

#ifdef DEBUG
    printf("%s:\n",tag.toUtf8().constData());
    printf("\tx = %f ... %f mm\n",1000*mn[0],1000*mx[0]);
    printf("\ty = %f ... %f mm\n",1000*mn[1],1000*mx[1]);
    printf("\tz = %f ... %f mm\n",1000*mn[2],1000*mx[2]);
#endif

    fov = std::max(mn.cwiseAbs().maxCoeff(), mx.cwiseAbs().maxCoeff());

    minv = mn;
    maxv = mx;
    fov_scale = 1.1f;
}

//=============================================================================================================

void MNEMshDisplaySurface::decide_curv_display(const QString& name)
{
    if (name.startsWith("inflated") || name.startsWith("sphere") || name.startsWith("white"))
        curvature_color_mode = SHOW_CURVATURE_OVERLAY;
    else
        curvature_color_mode = SHOW_CURVATURE_NONE;
    overlay_color_mode = SHOW_OVERLAY_HEAT;
}

//=============================================================================================================

void MNEMshDisplaySurface::setup_curvature_colors()
{
    if (np == 0)
        return;

    const int ncolor = nvertex_colors;
    const int totalSize = ncolor * np;

    if (vertex_colors.size() == 0)
        vertex_colors.resize(totalSize);

    float curv_sum = 0.0f;
    if (curvature_color_mode == SHOW_CURVATURE_OVERLAY) {
        for (int k = 0; k < np; k++) {
            const int base = k * ncolor;
            curv_sum += std::fabs(curv[k]);
            const float c = (curv[k] > 0) ? POS_CURV_COLOR : NEG_CURV_COLOR;
            for (int j = 0; j < 3; j++)
                vertex_colors[base + j] = c;
            if (ncolor == 4)
                vertex_colors[base + 3] = 1.0f;
        }
    }
    else {
        for (int k = 0; k < np; k++) {
            const int base = k * ncolor;
            curv_sum += std::fabs(curv[k]);
            for (int j = 0; j < 3; j++)
                vertex_colors[base + j] = EVEN_CURV_COLOR;
            if (ncolor == 4)
                vertex_colors[base + 3] = 1.0f;
        }
    }
#ifdef DEBUG
    printf("Average curvature : %f\n",curv_sum/np);
#endif
}

