//=============================================================================================================
/**
 * @file     mne_surface_or_volume.cpp
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
 * @brief    Definition of the MNE Surface or Volume (MneSurfaceOrVolume) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_or_volume.h"
#include "mne_surface_old.h"
#include "mne_source_space_old.h"
#include "mne_patch_info.h"
//#include "fwd_bem_model.h"
#include "mne_nearest.h"
#include "filter_thread_arg.h"
#include "mne_triangle.h"
#include "mne_msh_display_surface.h"
#include "mne_proj_data.h"
#include "mne_vol_geom.h"
#include "mne_mgh_tag_group.h"
#include "mne_mgh_tag.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_dig_point.h>

#include <utils/sphere.h>
#include <utils/ioutils.h>

#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QtConcurrent>

#define _USE_MATH_DEFINES
#include <math.h>

#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <algorithm>

//ToDo don't use access and unlink -> use QT stuff instead
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#else
#include <unistd.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//============================= dot.h =============================

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

#define MALLOC_17(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_17(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

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

float** mne_cmatrix_17(int numPoints,int numDim)
{
    float** m;
    float*  whole;

    m = MALLOC_17(numPoints, float *);
    if (!m)
    {
        matrix_error_17( 1, numPoints, numDim);
    }

    whole = MALLOC_17( numPoints * numDim, float);
    if (!whole)
    {
        matrix_error_17(2, numPoints, numDim);
    }

    for(int i = 0; i < numPoints; ++i)
    {
        m[i] = &whole[ i * numDim ];
    }

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

void mne_free_icmatrix_17 (int **m)

{
    if (m) {
        FREE_17(*m);
        FREE_17(m);
    }
}

#define NNEIGHBORS 26

#define CURVATURE_FILE_MAGIC_NUMBER  (16777215)

#define TAG_MGH_XFORM               31
#define TAG_SURF_GEOM               21
#define TAG_OLD_USEREALRAS          2
#define TAG_COLORTABLE              5
#define TAG_OLD_MGH_XFORM           30
#define TAG_OLD_COLORTABLE          1

#define TAG_USEREALRAS              4

#define ALLOC_ICMATRIX_17(x,y) mne_imatrix_17((x),(y))

int **mne_imatrix_17(int nr,int nc)

{
    int i,**m;
    int *whole;

    m = MALLOC_17(nr,int *);
    if (!m) matrix_error_17(1,nr,nc);
    whole = MALLOC_17(nr*nc,int);
    if (!whole) matrix_error_17(2,nr,nc);
    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

//float
Eigen::MatrixXf toFloatEigenMatrix_17(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenMatrix_17(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_17(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_17(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

void fromIntEigenMatrix_17(const Eigen::MatrixXi& from_mat, int **&to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromIntEigenMatrix_17(const Eigen::MatrixXi& from_mat, int **&to_mat)
{
    fromIntEigenMatrix_17(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

//============================= make_volume_source_space.c =============================

static FiffCoordTrans::UPtr make_voxel_ras_trans(float *r0,
                                                  float *x_ras,
                                                  float *y_ras,
                                                  float *z_ras,
                                                  float *voxel_size)

{
    float rot[3][3],move[3];
    int   j,k;

    VEC_COPY_17(move,r0);

    for (j = 0; j < 3; j++) {
        rot[j][0] = x_ras[j];
        rot[j][1] = y_ras[j];
        rot[j][2] = z_ras[j];
    }

    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            rot[j][k]    = voxel_size[k]*rot[j][k];

    return std::make_unique<FiffCoordTrans>(FIFFV_MNE_COORD_MRI_VOXEL,FIFFV_COORD_MRI,
                                            Eigen::Map<Eigen::Matrix3f>(&rot[0][0]),
                                            Eigen::Map<Eigen::Vector3f>(move));
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSurfaceOrVolume::MneSurfaceOrVolume()
{
}

//=============================================================================================================

MneSurfaceOrVolume::~MneSurfaceOrVolume()
{
    // rr, nn, itris, use_itris are Eigen matrices — auto-cleanup
    // inuse, vertno are Eigen VectorXi — auto-cleanup
    // tris, use_tris are std::vector<MneTriangle> — auto-cleanup
    // neighbor_tri is std::vector<VectorXi> — auto-cleanup
    // nneighbor_tri is Eigen VectorXi — auto-cleanup
    // curv is Eigen VectorXf — auto-cleanup

    // neighbor_vert is std::vector<VectorXi> — auto-cleanup
    // nneighbor_vert is Eigen VectorXi — auto-cleanup
    // vert_dist is std::vector<VectorXf> — auto-cleanup
    // nearest is std::vector<MneNearest> — auto-cleanup
    // patches is std::vector<unique_ptr<MnePatchInfo>> — auto-cleanup
    // dist, interpolator, vol_geom, mgh_tags are unique_ptr — auto-cleanup
    // voxel_surf_RAS_t, MRI_voxel_surf_RAS_t, MRI_surf_RAS_RAS_t are unique_ptr — auto-cleanup
    this->MRI_volume.clear();
}

//=============================================================================================================

double MneSurfaceOrVolume::solid_angle(const Eigen::Vector3f& from, const MneTriangle& tri)	/* ...to this triangle */
/*
     * Compute the solid angle according to van Oosterom's
     * formula
     */
{
    double v1[3],v2[3],v3[3];
    double l1,l2,l3,s,triple;
    double cross[3];

    VEC_DIFF_17 (from,tri.r1,v1);
    VEC_DIFF_17 (from,tri.r2,v2);
    VEC_DIFF_17 (from,tri.r3,v3);

    CROSS_PRODUCT_17(v1,v2,cross);
    triple = VEC_DOT_17(cross,v3);

    l1 = VEC_LEN_17(v1);
    l2 = VEC_LEN_17(v2);
    l3 = VEC_LEN_17(v3);
    s = (l1*l2*l3+VEC_DOT_17(v1,v2)*l3+VEC_DOT_17(v1,v3)*l2+VEC_DOT_17(v2,v3)*l1);

    return (2.0*atan2(triple,s));
}

//=============================================================================================================

double MneSurfaceOrVolume::sum_solids(const Eigen::Vector3f& from, const MneSurfaceOld& surf)
{
    int k;
    double tot_angle, angle;
    for (k = 0, tot_angle = 0.0; k < surf.ntri; k++) {
        angle = solid_angle(from,surf.tris[k]);
        tot_angle += angle;
    }
    return tot_angle;
}

//=============================================================================================================

int MneSurfaceOrVolume::filter_source_spaces(const MneSurfaceOld& surf, float limit, const FiffCoordTrans& mri_head_t, std::vector<std::unique_ptr<MneSourceSpaceOld>>& spaces, QTextStream *filtered)   /* Provide a list of filtered points here */
/*
     * Remove all source space points closer to the surface than a given limit
     */
{
    MneSourceSpaceOld* s;
    int k,p1,p2;
    float r1[3];
    float mindist,dist,diff[3];
    int   minnode;
    int   omit,omit_outside;
    double tot_angle;
    int nspace = static_cast<int>(spaces.size());

    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD && mri_head_t.isEmpty()) {
        printf("Source spaces are in head coordinates and no coordinate transform was provided!");
        return FAIL;
    }
    /*
        * How close are the source points to the surface?
        */
    printf("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        printf("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        printf("MRI coordinates.\n");
    else
        printf("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    printf("Checking that the sources are inside the bounding surface ");
    if (limit > 0.0)
        printf("and at least %6.1f mm away",1000*limit);
    printf(" (will take a few...)\n");
    omit         = 0;
    omit_outside = 0;
    for (k = 0; k < nspace; k++) {
        s = spaces[k].get();
        for (p1 = 0; p1 < s->np; p1++)
            if (s->inuse[p1]) {
                VEC_COPY_17(r1,&s->rr(p1,0));	/* Transform the point to MRI coordinates */
                if (s->coord_frame == FIFFV_COORD_HEAD)
                    FiffCoordTrans::apply_inverse_trans(r1,mri_head_t,FIFFV_MOVE);
                /*
                * Check that the source is inside the inner skull surface
                */
                tot_angle = sum_solids(Eigen::Map<const Eigen::Vector3f>(r1),surf)/(4*M_PI);
                if (std::fabs(tot_angle-1.0) > 1e-5) {
                    omit_outside++;
                    s->inuse[p1] = FALSE;
                    s->nuse--;
                    if (filtered)
                        *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                  << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                }
                else if (limit > 0.0) {
                    /*
                        * Check the distance limit
                        */
                    mindist = 1.0;
                    minnode = 0;
                    for (p2 = 0; p2 < surf.np; p2++) {
                        VEC_DIFF_17(r1,&surf.rr(p2,0),diff);
                        dist = VEC_LEN_17(diff);
                        if (dist < mindist) {
                            mindist = dist;
                            minnode = p2;
                        }
                    }
                    if (mindist < limit) {
                        omit++;
                        s->inuse[p1] = FALSE;
                        s->nuse--;
                        if (filtered)
                            *filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                      << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                    }
                }
            }
    }
    (void)minnode; // squash compiler warning, this is unused
    if (omit_outside > 0)
        printf("%d source space points omitted because they are outside the inner skull surface.\n",
               omit_outside);
    if (omit > 0)
        printf("%d source space points omitted because of the %6.1f-mm distance limit.\n",
               omit,1000*limit);
    printf("Thank you for waiting.\n");
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::add_patch_stats(MneSourceSpaceOld& s)
{
    MneNearest* nearest = s.nearest.data();
    MneNearest* this_patch;
    std::vector<std::unique_ptr<MnePatchInfo>> pinfo(s.nuse);
    int        nave,p,q,k;

    printf("Computing patch statistics...\n");
    if (s.neighbor_tri.empty())
        if (add_geometry_info(s,FALSE) != OK)
            return FAIL;

    if (s.nearest.empty()) {
        qCritical("The patch information is not available.");
        return FAIL;
    }
    if (s.nuse == 0) {
        s.patches.clear();
        return OK;
    }
    /*
       * Calculate the average normals and the patch areas
       */
    printf("\tareas, average normals, and mean deviations...");
    std::sort(s.nearest.begin(), s.nearest.end(),
              [](const MneNearest& a, const MneNearest& b) { return a.nearest < b.nearest; });
    nearest = s.nearest.data();  // refresh after sort
    nave = 1;
    for (p = 1, q = 0; p < s.np; p++) {
        if (nearest[p].nearest != nearest[p-1].nearest) {
            if (nave == 0) {
                qCritical("No vertices belong to the patch of vertex %d",nearest[p-1].nearest);
                return FAIL;
            }
            if (s.vertno[q] == nearest[p-1].nearest) { /* Some source space points may have been omitted since
                               * the patch information was computed */
                pinfo[q] = std::make_unique<MnePatchInfo>();
                pinfo[q]->vert = nearest[p-1].nearest;
                this_patch = nearest+p-nave;
                pinfo[q]->memb_vert.resize(nave);
                for (k = 0; k < nave; k++) {
                    pinfo[q]->memb_vert[k] = this_patch[k].vert;
                    this_patch[k].patch    = pinfo[q].get();
                }
                pinfo[q]->calculate_area(&s);
                pinfo[q]->calculate_normal_stats(&s);
                q++;
            }
            nave = 0;
        }
        nave++;
    }
    if (nave == 0) {
        qCritical("No vertices belong to the patch of vertex %d",nearest[p-1].nearest);
        return FAIL;
    }
    if (s.vertno[q] == nearest[p-1].nearest) {
        pinfo[q]       = std::make_unique<MnePatchInfo>();
        pinfo[q]->vert = nearest[p-1].nearest;
        this_patch = nearest+p-nave;
        pinfo[q]->memb_vert.resize(nave);
        for (k = 0; k < nave; k++) {
            pinfo[q]->memb_vert[k] = this_patch[k].vert;
            this_patch[k].patch = pinfo[q].get();
        }
        pinfo[q]->calculate_area(&s);
        pinfo[q]->calculate_normal_stats(&s);
        q++;
    }
    printf(" %d/%d [done]\n",q,s.nuse);

    s.patches = std::move(pinfo);

    return OK;
}

//=============================================================================================================

void MneSurfaceOrVolume::rearrange_source_space(MneSourceSpaceOld& s)
{
    int k,p;

    for (k = 0, s.nuse = 0; k < s.np; k++)
        if (s.inuse[k])
            s.nuse++;

    if (s.nuse == 0) {
        s.vertno.resize(0);
    }
    else {
        s.vertno.conservativeResize(s.nuse);
        for (k = 0, p = 0; k < s.np; k++)
            if (s.inuse[k])
                s.vertno[p++] = k;
    }
    if (!s.nearest.empty())
        add_patch_stats(s);
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::filter_source_space(FilterThreadArg *arg)
{
    FilterThreadArg* a = arg;
    int    p1,p2;
    double tot_angle;
    int    omit,omit_outside;
    float  r1[3];
    float  mindist,dist,diff[3];
    int    minnode;

    QSharedPointer<MneSurfaceOld> surf = a->surf.toStrongRef();
    if (!surf) {
        a->stat = FAIL;
        return;
    }

    omit         = 0;
    omit_outside = 0;

    for (p1 = 0; p1 < a->s->np; p1++) {
        if (a->s->inuse[p1]) {
            VEC_COPY_17(r1,&a->s->rr(p1,0));	/* Transform the point to MRI coordinates */
            if (a->s->coord_frame == FIFFV_COORD_HEAD) {
                Q_ASSERT(a->mri_head_t);
                FiffCoordTrans::apply_inverse_trans(r1,*a->mri_head_t,FIFFV_MOVE);
            }
            /*
           * Check that the source is inside the inner skull surface
           */
            tot_angle = sum_solids(Eigen::Map<const Eigen::Vector3f>(r1),*surf)/(4*M_PI);
            if (std::fabs(tot_angle-1.0) > 1e-5) {
                omit_outside++;
                a->s->inuse[p1] = FALSE;
                a->s->nuse--;
                if (a->filtered)
                    *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                 << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
            }
            else if (a->limit > 0.0) {
                /*
         * Check the distance limit
         */
                mindist = 1.0;
                minnode = 0;
                for (p2 = 0; p2 < surf->np; p2++) {
                    VEC_DIFF_17(r1,&surf->rr(p2,0),diff);
                    dist = VEC_LEN_17(diff);
                    if (dist < mindist) {
                        mindist = dist;
                        minnode = p2;
                    }
                }
                if (mindist < a->limit) {
                    omit++;
                    a->s->inuse[p1] = FALSE;
                    a->s->nuse--;
                    if (a->filtered)
                        *a->filtered << qSetFieldWidth(10) << qSetRealNumberPrecision(3) << Qt::fixed
                                     << 1000*r1[X_17] << " " << 1000*r1[Y_17] << " " << 1000*r1[Z_17] << "\n" << qSetFieldWidth(0);
                }
            }
        }
    }
    (void)minnode; // squash compiler warning, set but unused
    if (omit_outside > 0)
        printf("%d source space points omitted because they are outside the inner skull surface.\n",
                omit_outside);
    if (omit > 0)
        printf("%d source space points omitted because of the %6.1f-mm distance limit.\n",
                omit,1000*a->limit);
    a->stat = OK;
    return;
}

//=============================================================================================================

int MneSurfaceOrVolume::filter_source_spaces(float limit, const QString& bemfile, const FiffCoordTrans& mri_head_t, std::vector<std::unique_ptr<MneSourceSpaceOld>>& spaces, QTextStream *filtered, bool use_threads)
/*
          * Remove all source space points closer to the surface than a given limit
          */
{
    QSharedPointer<MneSurfaceOld> surf;
    int             k;
    int             nproc = QThread::idealThreadCount();
    FilterThreadArg* a;
    int nspace = static_cast<int>(spaces.size());

    if (bemfile.isEmpty())
        return OK;

    {
        MneSurfaceOld* rawSurf = MneSurfaceOld::read_bem_surface(bemfile,FIFFV_BEM_SURF_ID_BRAIN,FALSE,NULL);
        if (!rawSurf) {
            qCritical("BEM model does not have the inner skull triangulation!");
            return FAIL;
        }
        surf.reset(rawSurf);
    }
    /*
     * How close are the source points to the surface?
     */
    printf("Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        printf("head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        printf("MRI coordinates.\n");
    else
        printf("unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    printf("Checking that the sources are inside the inner skull ");
    if (limit > 0.0)
        printf("and at least %6.1f mm away",1000*limit);
    printf(" (will take a few...)\n");
    if (nproc < 2 || nspace == 1 || !use_threads) {
        /*
        * This is the conventional calculation
        */
        for (k = 0; k < nspace; k++) {
            a = new FilterThreadArg();
            a->s = spaces[k].get();
            a->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a->surf = surf;
            a->limit = limit;
            a->filtered = filtered;
            filter_source_space(a);
            if(a)
                delete a;
            rearrange_source_space(*spaces[k]);
        }
    }
    else {
        /*
        * Calculate all (both) source spaces simultaneously
        */
        QList<FilterThreadArg*> args;//filterThreadArg *args = MALLOC_17(nspace,filterThreadArg);

        for (k = 0; k < nspace; k++) {
            a = new FilterThreadArg();
            a->s = spaces[k].get();
            a->mri_head_t = std::make_unique<FiffCoordTrans>(mri_head_t);
            a->surf = surf;
            a->limit = limit;
            a->filtered = filtered;
            args.append(a);
        }
        /*
        * Ready to start the threads & Wait for them to complete
        */
        QtConcurrent::blockingMap(args, filter_source_space);

        for (k = 0; k < nspace; k++) {
            rearrange_source_space(*spaces[k]);
            if(args[k])
                delete args[k];
        }
    }
    printf("Thank you for waiting.\n\n");

    return OK;
}

//=============================================================================================================

MneSourceSpaceOld* MneSurfaceOrVolume::make_volume_source_space(const MneSurfaceOld& surf, float grid, float exclude, float mindist)
/*
     * Make a source space which covers the volume bounded by surf
     */
{
    float min[3],max[3],cm[3];
    int   minn[3],maxn[3];
    const float *node;
    float maxdist,dist,diff[3];
    int   k,c;
    MneSourceSpaceOld* sp = NULL;
    int np,nplane,nrow;
    int nneigh;
    int x,y,z;
    /*
        * Figure out the grid size
        */
    cm[X_17] = cm[Y_17] = cm[Z_17] = 0.0;
    node = &surf.rr(0,0);
    for (c = 0; c < 3; c++)
        min[c] = max[c] = node[c];

    for (k = 0; k < surf.np; k++) {
        node = &surf.rr(k,0);
        for (c = 0; c < 3; c++) {
            cm[c] += node[c];
            if (node[c] < min[c])
                min[c] = node[c];
            if (node[c] > max[c])
                max[c] = node[c];
        }
    }
    for (c = 0; c < 3; c++)
        cm[c] = cm[c]/surf.np;
    /*
       * Define the sphere which fits the surface
       */
    maxdist = 0.0;
    for (k = 0; k < surf.np; k++) {
        VEC_DIFF_17(cm,&surf.rr(k,0),diff);
        dist = VEC_LEN_17(diff);
        if (dist > maxdist)
            maxdist = dist;
    }
    printf("Surface CM = (%6.1f %6.1f %6.1f) mm\n",
           1000*cm[X_17], 1000*cm[Y_17], 1000*cm[Z_17]);
    printf("Surface fits inside a sphere with radius %6.1f mm\n",1000*maxdist);
    printf("Surface extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*min[X_17],1000*max[X_17],
           1000*min[Y_17],1000*max[Y_17],
           1000*min[Z_17],1000*max[Z_17]);
    for (c = 0; c < 3; c++) {
        if (max[c] > 0)
            maxn[c] = floor(std::fabs(max[c])/grid)+1;
        else
            maxn[c] = -floor(std::fabs(max[c])/grid)-1;
        if (min[c] > 0)
            minn[c] = floor(std::fabs(min[c])/grid)+1;
        else
            minn[c] = -floor(std::fabs(min[c])/grid)-1;
    }
    printf("Grid extent:\n"
           "\tx = %6.1f ... %6.1f mm\n"
           "\ty = %6.1f ... %6.1f mm\n"
           "\tz = %6.1f ... %6.1f mm\n",
           1000*(minn[X_17]*grid),1000*(maxn[X_17]*grid),
           1000*(minn[Y_17]*grid),1000*(maxn[Y_17]*grid),
           1000*(minn[Z_17]*grid),1000*(maxn[Z_17]*grid));
    /*
       * Now make the initial grid
       */
    np = 1;
    for (c = 0; c < 3; c++)
        np = np*(maxn[c]-minn[c]+1);
    nplane = (maxn[X_17]-minn[X_17]+1)*(maxn[Y_17]-minn[Y_17]+1);
    nrow   = (maxn[X_17]-minn[X_17]+1);
    sp = MneSurfaceOrVolume::create_source_space(np);
    sp->type = MNE_SOURCE_SPACE_VOLUME;
    sp->nneighbor_vert = Eigen::VectorXi::Constant(sp->np, NNEIGHBORS);
    sp->neighbor_vert.resize(sp->np);
    for (k = 0; k < sp->np; k++) {
        sp->inuse[k]  = TRUE;
        sp->vertno[k] = k;
        sp->nn(k,X_17) = sp->nn(k,Y_17) = 0.0; /* Source orientation is immaterial */
        sp->nn(k,Z_17) = 1.0;
        sp->neighbor_vert[k] = Eigen::VectorXi::Constant(NNEIGHBORS, -1);
        sp->nuse++;
    }
    for (k = 0, z = minn[Z_17]; z <= maxn[Z_17]; z++) {
        for (y = minn[Y_17]; y <= maxn[Y_17]; y++) {
            for (x = minn[X_17]; x <= maxn[X_17]; x++, k++) {
                sp->rr(k,X_17) = x*grid;
                sp->rr(k,Y_17) = y*grid;
                sp->rr(k,Z_17) = z*grid;
                /*
             * Figure out the neighborhood:
             * 6-neighborhood first
             */
                Eigen::VectorXi& neigh = sp->neighbor_vert[k];
                if (z > minn[Z_17])
                    neigh[0]  = k - nplane;
                if (x < maxn[X_17])
                    neigh[1] = k + 1;
                if (y < maxn[Y_17])
                    neigh[2] = k + nrow;
                if (x > minn[X_17])
                    neigh[3] = k - 1;
                if (y > minn[Y_17])
                    neigh[4] = k - nrow;
                if (z < maxn[Z_17])
                    neigh[5] = k + nplane;
                /*
             * Then the rest to complete the 26-neighborhood
             * First the plane below
             */
                if (z > minn[Z_17]) {
                    if (x < maxn[X_17]) {
                        neigh[6] = k + 1 - nplane;
                        if (y < maxn[Y_17])
                            neigh[7] = k + 1 + nrow - nplane;
                    }
                    if (y < maxn[Y_17])
                        neigh[8] = k + nrow - nplane;
                    if (x > minn[X_17]) {
                        if (y < maxn[Y_17])
                            neigh[9] = k - 1 + nrow - nplane;
                        neigh[10] = k - 1 - nplane;
                        if (y > minn[Y_17])
                            neigh[11] = k - 1 - nrow - nplane;
                    }
                    if (y > minn[Y_17]) {
                        neigh[12] = k - nrow - nplane;
                        if (x < maxn[X_17])
                            neigh[13] = k + 1 - nrow - nplane;
                    }
                }
                /*
             * Then the same plane
             */
                if (x < maxn[X_17] && y < maxn[Y_17])
                    neigh[14] = k + 1 + nrow;
                if (x > minn[X_17]) {
                    if (y < maxn[Y_17])
                        neigh[15] = k - 1 + nrow;
                    if (y > minn[Y_17])
                        neigh[16] = k - 1 - nrow;
                }
                if (y > minn[Y_17] && x < maxn[X_17])
                    neigh[17] = k + 1 - nrow - nplane;
                /*
             * Finally one plane above
             */
                if (z < maxn[Z_17]) {
                    if (x < maxn[X_17]) {
                        neigh[18] = k + 1 + nplane;
                        if (y < maxn[Y_17])
                            neigh[19] = k + 1 + nrow + nplane;
                    }
                    if (y < maxn[Y_17])
                        neigh[20] = k + nrow + nplane;
                    if (x > minn[X_17]) {
                        if (y < maxn[Y_17])
                            neigh[21] = k - 1 + nrow + nplane;
                        neigh[22] = k - 1 + nplane;
                        if (y > minn[Y_17])
                            neigh[23] = k - 1 - nrow + nplane;
                    }
                    if (y > minn[Y_17]) {
                        neigh[24] = k - nrow + nplane;
                        if (x < maxn[X_17])
                            neigh[25] = k + 1 - nrow + nplane;
                    }
                }
            }
        }
    }
    printf("%d sources before omitting any.\n",sp->nuse);
    /*
       * Exclude infeasible points
       */
    for (k = 0; k < sp->np; k++) {
        VEC_DIFF_17(cm,&sp->rr(k,0),diff);
        dist = VEC_LEN_17(diff);
        if (dist < exclude || dist > maxdist) {
            sp->inuse[k] = FALSE;
            sp->nuse--;
        }
    }
    printf("%d sources after omitting infeasible sources.\n",sp->nuse);
    {
        std::vector<std::unique_ptr<MneSourceSpaceOld>> sp_vec;
        sp_vec.push_back(std::unique_ptr<MneSourceSpaceOld>(sp));
        if (filter_source_spaces(surf,mindist,FiffCoordTrans(),sp_vec,NULL) != OK) {
            sp_vec[0].release(); // caller still owns sp
            goto bad;
        }
        sp_vec[0].release(); // caller still owns sp
    }
    printf("%d sources remaining after excluding the sources outside the surface and less than %6.1f mm inside.\n",sp->nuse,1000*mindist);
    /*
       * Omit unused vertices from the neighborhoods
       */
    printf("Adjusting the neighborhood info...");
    for (k = 0; k < sp->np; k++) {
        Eigen::VectorXi& neigh = sp->neighbor_vert[k];
        nneigh = sp->nneighbor_vert[k];
        if (sp->inuse[k]) {
            for (c = 0; c < nneigh; c++)
                if (!sp->inuse[neigh[c]])
                    neigh[c] = -1;
        }
        else {
            for (c = 0; c < nneigh; c++)
                neigh[c] = -1;
        }
    }
    printf("[done]\n");
    /*
     * Set up the volume data (needed for creating the interpolation matrix)
     */
    {
        float r0[3],voxel_size[3],x_ras[3],y_ras[3],z_ras[3];
        int   width,height,depth;

        r0[X_17] = minn[X_17]*grid;
        r0[Y_17] = minn[Y_17]*grid;
        r0[Z_17] = minn[Z_17]*grid;

        voxel_size[0] = grid;
        voxel_size[1] = grid;
        voxel_size[2] = grid;

        width  = (maxn[X_17]-minn[X_17]+1);
        height = (maxn[Y_17]-minn[Y_17]+1);
        depth  = (maxn[Z_17]-minn[Z_17]+1);

        for (k = 0; k < 3; k++)
            x_ras[k] = y_ras[k] = z_ras[k] = 0.0;

        x_ras[0] = 1.0;
        y_ras[1] = 1.0;
        z_ras[2] = 1.0;

        sp->voxel_surf_RAS_t = make_voxel_ras_trans(r0,x_ras,y_ras,z_ras,voxel_size);
        if (!sp->voxel_surf_RAS_t || sp->voxel_surf_RAS_t->isEmpty())
            goto bad;

        sp->vol_dims[0] = width;
        sp->vol_dims[1] = height;
        sp->vol_dims[2] = depth;
        VEC_COPY_17(sp->voxel_size,voxel_size);
    }

    return sp;

bad : {
        if(sp)
            delete sp;
        return NULL;
    }
}

//=============================================================================================================

MneSourceSpaceOld* MneSurfaceOrVolume::create_source_space(int np)
/*
          * Create a new source space and all associated data
          */
{
    MneSourceSpaceOld* res = new MneSourceSpaceOld();
    res->np      = np;
    if (np > 0) {
        res->rr      = PointsT::Zero(np, 3);
        res->nn      = NormalsT::Zero(np, 3);
        res->inuse   = VectorXi::Zero(np);
        res->vertno  = VectorXi::Zero(np);
    }
    res->nuse     = 0;
    res->ntri     = 0;
    // res->tris is std::vector<MneTriangle> — default-constructed empty
    res->tot_area = 0.0;

    res->nuse_tri  = 0;
    // res->use_tris is std::vector<MneTriangle> — default-constructed empty

    // neighbor_tri, nneighbor_tri, curv, val,
    // neighbor_vert, nneighbor_vert, vert_dist
    // are Eigen/std::vector types — default-constructed empty

    res->sigma       = -1.0;
    res->coord_frame = FIFFV_COORD_MRI;
    res->id          = FIFFV_MNE_SURF_UNKNOWN;
    res->subject.clear();
    res->type        = FIFFV_MNE_SPACE_SURFACE;

    // res->nearest is std::vector<MneNearest> — default-constructed empty
    // res->patches is std::vector<unique_ptr<MnePatchInfo>> — default-constructed empty

    res->dist       = nullptr;
    res->dist_limit = -1.0;

    res->voxel_surf_RAS_t.reset();
    res->vol_dims[0] = res->vol_dims[1] = res->vol_dims[2] = 0;

    res->MRI_volume.clear();
    res->MRI_surf_RAS_RAS_t.reset();
    res->MRI_voxel_surf_RAS_t.reset();
    res->MRI_vol_dims[0] = res->MRI_vol_dims[1] = res->MRI_vol_dims[2] = 0;
    res->interpolator         = nullptr;

    res->vol_geom         = nullptr;
    res->mgh_tags         = nullptr;

    res->cm[X_17] = res->cm[Y_17] = res->cm[Z_17] = 0.0;

    return res;
}

//=============================================================================================================

MneSurfaceOld* MneSurfaceOrVolume::read_bem_surface(const QString &name, int which, int add_geometry, float *sigmap)          /* Conductivity? */
{
    return read_bem_surface(name,which,add_geometry,sigmap,true);
}

//=============================================================================================================

MneSurfaceOld* MneSurfaceOrVolume::read_bem_surface2(const QString& name, int  which, int  add_geometry, float *sigmap)
{
  return read_bem_surface(name,which,add_geometry,sigmap,FALSE);
}

//=============================================================================================================

MneSurfaceOld* MneSurfaceOrVolume::read_bem_surface(const QString &name, int which, int add_geometry, float *sigmap, bool check_too_many_neighbors)
/*
     * Read a Neuromag-style BEM surface description
     */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffDirNode::SPtr> surfs;
    QList<FiffDirNode::SPtr> bems;
    FiffDirNode::SPtr node;
    FiffTag::SPtr t_pTag;

    int     id = -1;
    int     nnode,ntri;
    MneSurfaceOld* s = NULL;
    int k;
    MatrixXf tmp_node_normals;
    int coord_frame = FIFFV_COORD_MRI;
    float sigma = -1.0;
    MatrixXf tmp_nodes;
    MatrixXi tmp_triangles;

    if(!stream->open())
        goto bad;
    /*
        * Check for the existence of BEM coord frame
        */
    bems = stream->dirtree()->dir_tree_find(FIFFB_BEM);
    if (bems.size() > 0) {
        node = bems[0];
        if (node->find_tag(stream, FIFF_BEM_COORD_FRAME, t_pTag)) {
            coord_frame = *t_pTag->toInt();
        }
    }
    surfs = stream->dirtree()->dir_tree_find(FIFFB_BEM_SURF);
    if (surfs.size() == 0) {
        printf ("No BEM surfaces found in %s",name.toUtf8().constData());
        goto bad;
    }
    if (which >= 0) {
        for (k = 0; k < surfs.size(); ++k) {
            node = surfs[k];
            /*
                * Read the data from this node
                */
            if (node->find_tag(stream, FIFF_BEM_SURF_ID, t_pTag)) {
                id = *t_pTag->toInt();
                if (id == which)
                    break;
            }
        }
        if (id != which) {
            printf("Desired surface not found in %s",name.toUtf8().constData());
            goto bad;
        }
    }
    else
        node = surfs[0];
    /*
       * Get the compulsory tags
       */
    if (!node->find_tag(stream, FIFF_BEM_SURF_NNODE, t_pTag))
        goto bad;
    nnode = *t_pTag->toInt();

    if (!node->find_tag(stream, FIFF_BEM_SURF_NTRI, t_pTag))
        goto bad;
    ntri = *t_pTag->toInt();

    if (!node->find_tag(stream, FIFF_BEM_SURF_NODES, t_pTag))
        goto bad;
    tmp_nodes = t_pTag->toFloatMatrix().transpose();

    if (node->find_tag(stream, FIFF_BEM_SURF_NORMALS, t_pTag)) {
        tmp_node_normals = t_pTag->toFloatMatrix().transpose();
    }

    if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag))
        goto bad;
    tmp_triangles = t_pTag->toIntMatrix().transpose();

    if (node->find_tag(stream, FIFF_MNE_COORD_FRAME, t_pTag)) {
        coord_frame = *t_pTag->toInt();
    }
    else if (node->find_tag(stream, FIFF_BEM_COORD_FRAME, t_pTag)) {
        coord_frame = *t_pTag->toInt();
    }
    if (node->find_tag(stream, FIFF_BEM_SIGMA, t_pTag)) {
        sigma = *t_pTag->toFloat();
    }

    stream->close();

    s = (MneSurfaceOld*)create_source_space(0);
    tmp_triangles.array() -= 1;
    s->itris       = tmp_triangles;
    s->id          = which;
    s->sigma       = sigma;
    s->coord_frame = coord_frame;
    s->rr          = tmp_nodes;
    s->nn          = tmp_node_normals;
    s->ntri        = ntri;
    s->np          = nnode;
    // curv and val are Eigen VectorXf — default-constructed empty

    if (add_geometry) {
        if (check_too_many_neighbors) {
            if (add_geometry_info(reinterpret_cast<MneSourceSpaceOld&>(*s),s->nn.rows() == 0) != OK)
                goto bad;
        }
        else {
            if (add_geometry_info2(reinterpret_cast<MneSourceSpaceOld&>(*s),s->nn.rows() == 0) != OK)
                goto bad;
        }
    }
    else if (s->nn.rows() == 0) {       /* Normals only */
        if (add_vertex_normals(reinterpret_cast<MneSourceSpaceOld&>(*s)) != OK)
            goto bad;
    }
    else
        add_triangle_data(reinterpret_cast<MneSourceSpaceOld&>(*s));

    s->nuse   = s->np;
    s->inuse  = Eigen::VectorXi::Ones(s->np);
    s->vertno = Eigen::VectorXi::LinSpaced(s->np, 0, s->np - 1);
    if (sigmap)
        *sigmap = sigma;

    return s;

bad : {
        stream->close();
        return NULL;
    }
}

//=============================================================================================================

void MneSurfaceOrVolume::triangle_coords(const Eigen::Vector3f& r, const MneSurfaceOld& s, int tri, float &x, float &y, float &z)
/*
          * Compute the coordinates of a point within a triangle
          */
{
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    const MneTriangle* this_tri;

    this_tri = &s.tris[tri];

    VEC_DIFF_17(this_tri->r1,r,rr);
    z = VEC_DOT_17(rr,this_tri->nn);

    a =  VEC_DOT_17(this_tri->r12,this_tri->r12);
    b =  VEC_DOT_17(this_tri->r13,this_tri->r13);
    c =  VEC_DOT_17(this_tri->r12,this_tri->r13);

    v1 = VEC_DOT_17(rr,this_tri->r12);
    v2 = VEC_DOT_17(rr,this_tri->r13);

    det = a*b - c*c;

    x = (b*v1 - c*v2)/det;
    y = (a*v2 - c*v1)/det;

    return;
}

//=============================================================================================================

int MneSurfaceOrVolume::nearest_triangle_point(const Eigen::Vector3f& r, const MneSurfaceOld& s, const MneProjData *user, int tri, float &x, float &y, float &z)
/*
          * Find the nearest point from a triangle
          */
{

    double p,q,p0,q0,t0;
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    double best,dist,dist0;
    const MneProjData*    pd = user;
    const MneTriangle* this_tri;

    this_tri = &s.tris[tri];
    VEC_DIFF_17(this_tri->r1,r,rr);
    dist  = VEC_DOT_17(rr,this_tri->nn);

    if (pd) {
        if (!pd->act[tri])
            return FALSE;
        a = pd->a[tri];
        b = pd->b[tri];
        c = pd->c[tri];
    }
    else {
        a =  VEC_DOT_17(this_tri->r12,this_tri->r12);
        b =  VEC_DOT_17(this_tri->r13,this_tri->r13);
        c =  VEC_DOT_17(this_tri->r12,this_tri->r13);
    }

    v1 = VEC_DOT_17(rr,this_tri->r12);
    v2 = VEC_DOT_17(rr,this_tri->r13);

    det = a*b - c*c;

    p = (b*v1 - c*v2)/det;
    q = (a*v2 - c*v1)/det;
    /*
       * If the point projects into the triangle we are done
       */
    if (p >= 0.0 && p <= 1.0 &&
            q >= 0.0 && q <= 1.0 &&
            q <= 1.0 - p) {
        x = p;
        y = q;
        z = dist;
        return TRUE;
    }
    /*
       * Tough: must investigate the sides
       * We might do something intelligent here. However, for now it is ok
       * to do it in the hard way
       */
    /*
       * Side 1 -> 2
       */
    p0 = p + 0.5*(q * c)/a;
    if (p0 < 0.0)
        p0 = 0.0;
    else if (p0 > 1.0)
        p0 = 1.0;
    q0 = 0.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    best = dist0;
    x = p0;
    y = q0;
    z = dist0;
    /*
       * Side 2 -> 3
       */
    t0 = 0.5*((2.0*a-c)*(1.0-p) + (2.0*b-c)*q)/(a+b-c);
    if (t0 < 0.0)
        t0 = 0.0;
    else if (t0 > 1.0)
        t0 = 1.0;
    p0 = 1.0 - t0;
    q0 = t0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        x = p0;
        y = q0;
        z = dist0;
    }
    /*
       * Side 1 -> 3
       */
    p0 = 0.0;
    q0 = q + 0.5*(p * c)/b;
    if (q0 < 0.0)
        q0 = 0.0;
    else if (q0 > 1.0)
        q0 = 1.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        x = p0;
        y = q0;
        z = dist0;
    }
    return TRUE;
}

//=============================================================================================================

Eigen::Vector3f MneSurfaceOrVolume::project_to_triangle(const MneSurfaceOld& s, int tri, float p, float q)
{
    const MneTriangle* this_tri = &s.tris[tri];

    return Eigen::Vector3f(
        this_tri->r1[0] + p*this_tri->r12[0] + q*this_tri->r13[0],
        this_tri->r1[1] + p*this_tri->r12[1] + q*this_tri->r13[1],
        this_tri->r1[2] + p*this_tri->r12[2] + q*this_tri->r13[2]
    );
}

//=============================================================================================================

int MneSurfaceOrVolume::nearest_triangle_point(const Eigen::Vector3f& r, const MneSurfaceOld& s, int tri, float &x, float &y, float &z)
/*
     * This is for external use
     */
{
    return nearest_triangle_point(r,s,nullptr,tri,x,y,z);
}

//=============================================================================================================

int MneSurfaceOrVolume::project_to_surface(const MneSurfaceOld& s, const MneProjData *proj_data, const Eigen::Vector3f& r, float &distp)
/*
          * Project the point onto the closest point on the surface
          */
{
    float dist;			/* Distance to the triangle */
    float p,q;			/* Coordinates on the triangle */
    float p0,q0,dist0;
    int   best;
    int   k;

    p0 = q0 = 0.0;
    dist0 = 0.0;
    for (best = -1, k = 0; k < s.ntri; k++) {
        if (nearest_triangle_point(r,s,proj_data,k,p,q,dist)) {
            if (best < 0 || std::fabs(dist) < std::fabs(dist0)) {
                dist0 = dist;
                best = k;
                p0 = p;
                q0 = q;
            }
        }
    }
    distp = dist0;
    return best;
}

//=============================================================================================================

Eigen::Vector3f MneSurfaceOrVolume::project_to_triangle(const MneSurfaceOld& s,
                                                 int        best,
                                                 const Eigen::Vector3f& r)
/*
      * Project to a triangle provided that we know the best match already
      */
{
    float p,q,dist;

    nearest_triangle_point(r,s,best,p,q,dist);
    return project_to_triangle(s,best,p,q);
}

//=============================================================================================================

void MneSurfaceOrVolume::find_closest_on_surface_approx(const MneSurfaceOld& s, const PointsT& r, int np, Eigen::VectorXi& nearest, Eigen::VectorXf& dist, int nstep)
/*
      * Find the closest triangle on the surface for each point and the distance to it
      * This uses the values in nearest as approximations of the closest triangle
      */
{
    MneProjData* p = new MneProjData(&s);
    int k,was;

    printf("%s for %d points %d steps...",nearest[0] < 0 ? "Closest" : "Approx closest",np,nstep);

    for (k = 0; k < np; k++) {
        was = nearest[k];
        Eigen::Vector3f pt = Eigen::Map<const Eigen::Vector3f>(r.row(k).data());
        decide_search_restriction(s,*p,nearest[k],nstep,pt);
        nearest[k] =  project_to_surface(s,p,pt,dist[k]);
        if (nearest[k] < 0) {
            decide_search_restriction(s,*p,-1,nstep,pt);
            nearest[k] =  project_to_surface(s,p,pt,dist[k]);
        }
    }
    (void)was; // squash compiler warning, set but unused

    printf("[done]\n");
    delete p;
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::decide_search_restriction(const MneSurfaceOld& s,
                                                   MneProjData&   p,
                                                   int        approx_best, /* We know the best triangle approximately
                                                                                      * already */
                                                   int        nstep,
                                                   const Eigen::Vector3f& r)
/*
      * Restrict the search only to feasible triangles
      */
{
    int k;
    float diff[3],dist,mindist;
    int minvert;

    for (k = 0; k < s.ntri; k++)
        p.act[k] = FALSE;

    if (approx_best < 0) {
        /*
        * Search for the closest vertex
        */
        mindist = 1000.0;
        minvert = 0;
        for (k = 0; k < s.np; k++) {
            VEC_DIFF_17(r,&s.rr(k,0),diff);
            dist = VEC_LEN_17(diff);
            if (dist < mindist && s.nneighbor_tri[k] > 0) {
                mindist = dist;
                minvert = k;
            }
        }
    }
    else {
        /*
     * Just use this triangle
     */
        const MneTriangle* this_tri = NULL;

        this_tri = &s.tris[approx_best];
        VEC_DIFF_17(r,this_tri->r1,diff);
        mindist = VEC_LEN_17(diff);
        minvert = this_tri->vert[0];

        VEC_DIFF_17(r,this_tri->r2,diff);
        dist = VEC_LEN_17(diff);
        if (dist < mindist) {
            mindist = dist;
            minvert = this_tri->vert[1];
        }
        VEC_DIFF_17(r,this_tri->r3,diff);
        dist = VEC_LEN_17(diff);
        if (dist < mindist) {
            mindist = dist;
            minvert = this_tri->vert[2];
        }
    }
    /*
     * Activate triangles in the neighborhood
     */
    activate_neighbors(s,minvert,p.act,nstep);

    for (k = 0, p.nactive = 0; k < s.ntri; k++)
        if (p.act[k])
            p.nactive++;
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::activate_neighbors(const MneSurfaceOld& s, int start, Eigen::VectorXi &act, int nstep)
/*
      * Blessed recursion...
      */
{
    int k;

    if (nstep == 0)
        return;

    for (k = 0; k < s.nneighbor_tri[start]; k++)
        act[s.neighbor_tri[start][k]] = TRUE;
    for (k = 0; k < s.nneighbor_vert[start]; k++)
        activate_neighbors(s,s.neighbor_vert[start][k],act,nstep-1);

    return;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_source_spaces(const QString &name, std::vector<std::unique_ptr<MneSourceSpaceOld>>& spaces)
/*
 * Read source spaces from a FIFF file
 */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    std::vector<std::unique_ptr<MneSourceSpaceOld>> local_spaces;
    std::unique_ptr<MneSourceSpaceOld> new_space;
    QList<FiffDirNode::SPtr> sources;
    FiffDirNode::SPtr     node;
    FiffTag::SPtr t_pTag;
    int             j,k,p,q;
    int             ntri;
    int             *nearest = NULL;
    float           *nearest_dist = NULL;
    int             *nneighbors = NULL;
    int             *neighbors  = NULL;
    int             *vol_dims = NULL;

    if(!stream->open())
        goto bad;

    sources = stream->dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (sources.size() == 0) {
        printf("No source spaces available here");
        goto bad;
    }
    for (j = 0; j < sources.size(); j++) {
        new_space.reset(MneSurfaceOrVolume::create_source_space(0));
        node = sources[j];
        /*
            * Get the mandatory data first
            */
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag)) {
            goto bad;
        }
        new_space->np = *t_pTag->toInt();
        if (new_space->np == 0) {
            printf("No points in this source space");
            goto bad;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_POINTS, t_pTag)) {
            goto bad;
        }
        MatrixXf tmp_rr = t_pTag->toFloatMatrix().transpose();
        new_space->rr = tmp_rr;
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag)) {
            goto bad;
        }
        MatrixXf tmp_nn = t_pTag->toFloatMatrix().transpose();
        new_space->nn = tmp_nn;
        if (!node->find_tag(stream, FIFF_MNE_COORD_FRAME, t_pTag)) {
            new_space->coord_frame = FIFFV_COORD_MRI;
        }
        else {
            new_space->coord_frame = *t_pTag->toInt();
        }
        if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_ID, t_pTag)) {
            new_space->id = *t_pTag->toInt();
        }
        if (node->find_tag(stream, FIFF_SUBJ_HIS_ID, t_pTag)) {
            new_space->subject = (char *)t_pTag->data();
        }
        if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TYPE, t_pTag)) {
            new_space->type = *t_pTag->toInt();
        }
        ntri = 0;
        if (node->find_tag(stream, FIFF_BEM_SURF_NTRI, t_pTag)) {
            ntri = *t_pTag->toInt();
        }
        else if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NTRI, t_pTag)) {
            ntri = *t_pTag->toInt();
        }
        if (ntri > 0) {

            if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag)) {
                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, t_pTag)) {
                    goto bad;
                }
            }

            MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
            tmp_itris.array() -= 1;
            new_space->itris = tmp_itris;
            new_space->ntri = ntri;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE, t_pTag)) {
            if (new_space->type == FIFFV_MNE_SPACE_VOLUME) {
                /*
                    * Use all
                    */
                new_space->nuse   = new_space->np;
                new_space->inuse  = Eigen::VectorXi::Ones(new_space->nuse);
                new_space->vertno = Eigen::VectorXi::LinSpaced(new_space->nuse, 0, new_space->nuse - 1);
            }
            else {
                /*
                    * None in use
                    * NOTE: The consequences of this change have to be evaluated carefully
                    */
                new_space->nuse   = 0;
                new_space->inuse  = Eigen::VectorXi::Zero(new_space->np);
                new_space->vertno.resize(0);
            }
        }
        else {
            new_space->nuse = *t_pTag->toInt();
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_SELECTION, t_pTag)) {
                goto bad;
            }

            qDebug() << "ToDo: Check whether new_space->inuse contains the right stuff!!! - use VectorXi instead";
            new_space->inuse = Eigen::VectorXi::Zero(new_space->np);
            if (new_space->nuse > 0) {
                new_space->vertno = Eigen::VectorXi::Zero(new_space->nuse);
                for (k = 0, p = 0; k < new_space->np; k++) {
                    new_space->inuse[k] = t_pTag->toInt()[k]; //DEBUG
                    if (new_space->inuse[k])
                        new_space->vertno[p++] = k;
                }
            }
            else {
                new_space->vertno.resize(0);
            }
            /*
                * Selection triangulation
                */
            ntri = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI, t_pTag)) {
                ntri = *t_pTag->toInt();
            }
            if (ntri > 0) {

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, t_pTag)) {
                    goto bad;
                }

                MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
                tmp_itris.array() -= 1;
                new_space->use_itris = tmp_itris;
                new_space->nuse_tri = ntri;
            }
            /*
                * The patch information becomes relevant here
                */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST, t_pTag)) {
                nearest  = t_pTag->toInt();
                new_space->nearest.resize(new_space->np);
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].vert = k;
                    new_space->nearest[k].nearest = nearest[k];
                    new_space->nearest[k].patch = nullptr;
                }

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag)) {
                    goto bad;
                }
                qDebug() << "ToDo: Check whether nearest_dist contains the right stuff!!! - use VectorXf instead";
                nearest_dist = t_pTag->toFloat();
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].dist = nearest_dist[k];
                }
                //                FREE_17(nearest); nearest = NULL;
                //                FREE_17(nearest_dist); nearest_dist = NULL;
            }
            /*
            * We may have the distance matrix
            */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, t_pTag)) {
                new_space->dist_limit = *t_pTag->toFloat();
                if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST, t_pTag)) {
                    //                    SparseMatrix<double> tmpSparse = t_pTag->toSparseFloatMatrix();
                    auto dist_lower = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
                    if (!dist_lower) {
                        goto bad;
                    }
                    auto dist_full = dist_lower->mne_add_upper_triangle_rcs();
                    if (!dist_full) {
                        goto bad;
                    }
                    new_space->dist.reset(dist_full.release());
                }
                else
                    new_space->dist_limit = 0.0;
            }
        }
        /*
            * For volume source spaces we might have the neighborhood information
            */
        if (new_space->type == FIFFV_MNE_SPACE_VOLUME) {
            int ntot,nvert,ntot_count,nneigh;
            int *neigh;

            nneighbors = neighbors = NULL;
            ntot = nvert = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEIGHBORS, t_pTag)) {
                qDebug() << "ToDo: Check whether neighbors contains the right stuff!!! - use VectorXi instead";
                neighbors = t_pTag->toInt();
                ntot      = t_pTag->size()/sizeof(fiff_int_t);
            }
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NNEIGHBORS, t_pTag)) {
                qDebug() << "ToDo: Check whether nneighbors contains the right stuff!!! - use VectorXi instead";
                nneighbors = t_pTag->toInt();
                nvert      = t_pTag->size()/sizeof(fiff_int_t);
            }
            if (neighbors && nneighbors) {
                if (nvert != new_space->np) {
                    printf("Inconsistent neighborhood data in file.");
                    goto bad;
                }
                for (k = 0, ntot_count = 0; k < nvert; k++)
                    ntot_count += nneighbors[k];
                if (ntot_count != ntot) {
                    printf("Inconsistent neighborhood data in file.");
                    goto bad;
                }
                new_space->nneighbor_vert = Eigen::VectorXi::Zero(nvert);
                new_space->neighbor_vert.resize(nvert);
                for (k = 0, q = 0; k < nvert; k++) {
                    new_space->nneighbor_vert[k] = nneigh = nneighbors[k];
                    new_space->neighbor_vert[k] = Eigen::VectorXi(nneigh);
                    for (p = 0; p < nneigh; p++,q++)
                        new_space->neighbor_vert[k][p] = neighbors[q];
                }
            }
            FREE_17(neighbors);
            FREE_17(nneighbors);
            nneighbors = neighbors = NULL;
            /*
                * There might be a coordinate transformation and dimensions
                */
            new_space->voxel_surf_RAS_t   = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS));
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, t_pTag)) {
                qDebug() << "ToDo: Check whether vol_dims contains the right stuff!!! - use VectorXi instead";
                vol_dims = t_pTag->toInt();
            }
            if (vol_dims)
                VEC_COPY_17(new_space->vol_dims,vol_dims);
            {
                QList<FiffDirNode::SPtr>  mris = node->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);

                if (mris.size() == 0) { /* The old way */
                    new_space->MRI_surf_RAS_RAS_t = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readTransformFromNode(stream, node, FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS));
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_MRI_FILE, t_pTag)) {
                        qDebug() << "ToDo: Check whether new_space->MRI_volume  contains the right stuff!!! - use QString instead";
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator.reset(FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag).release());
                    }
                }
                else {
                    if (node->find_tag(stream, FIFF_MNE_FILE_NAME, t_pTag)) {
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    new_space->MRI_surf_RAS_RAS_t = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readTransformFromNode(stream, mris[0], FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS));
                    new_space->MRI_voxel_surf_RAS_t   = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readTransformFromNode(stream, mris[0], FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS));

                    if (mris[0]->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator.reset(FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag).release());
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_WIDTH, t_pTag)) {
                        new_space->MRI_vol_dims[0] = *t_pTag->toInt();
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_HEIGHT, t_pTag)) {
                        new_space->MRI_vol_dims[1] = *t_pTag->toInt();
                    }
                    if (mris[0]->find_tag(stream, FIFF_MRI_DEPTH, t_pTag)) {
                        new_space->MRI_vol_dims[2] = *t_pTag->toInt();
                    }
                }
            }
        }
        add_triangle_data(*new_space);
        local_spaces.push_back(std::move(new_space));
    }
    stream->close();

     spaces = std::move(local_spaces);

    return FIFF_OK;

bad : {
        stream->close();
        // new_space and local_spaces auto-cleanup via unique_ptr
        FREE_17(nearest);
        FREE_17(nearest_dist);
        FREE_17(neighbors);
        FREE_17(nneighbors);
        FREE_17(vol_dims);

        return FIFF_FAIL;
    }
}

//=============================================================================================================

void MneSurfaceOrVolume::update_inuse(MneSourceSpaceOld& s, Eigen::VectorXi new_inuse)
/*
 * Update the active vertices
 */
{
    int k,p,nuse;

    s.inuse = std::move(new_inuse);

    for (k = 0, nuse = 0; k < s.np; k++)
        if (s.inuse[k])
            nuse++;

    s.nuse   = nuse;
    if (s.nuse > 0) {
        s.vertno.conservativeResize(s.nuse);
        for (k = 0, p = 0; k < s.np; k++)
            if (s.inuse[k])
                s.vertno[p++] = k;
    }
    else {
        s.vertno.resize(0);
    }
    return;
}

//=============================================================================================================

int MneSurfaceOrVolume::is_left_hemi(const MneSourceSpaceOld& s)
/*
 * Left or right hemisphere?
 */
{
    int k;
    float xave;

    for (k = 0, xave = 0.0; k < s.np; k++)
        xave += s.rr(k,0);
    if (xave < 0.0)
        return TRUE;
    else
        return FALSE;
}

//=============================================================================================================

int MneSurfaceOrVolume::transform_source_space(MneSourceSpaceOld& ss, const FiffCoordTrans& t)
/*
     * Transform source space data into another coordinate frame
     */
{
    int k;
    if (ss.coord_frame == t.to)
        return OK;
    if (ss.coord_frame != t.from) {
        printf("Coordinate transformation does not match with the source space coordinate system.");
        return FAIL;
    }
    for (k = 0; k < ss.np; k++) {
        FiffCoordTrans::apply_trans(&ss.rr(k,0),t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(&ss.nn(k,0),t,FIFFV_NO_MOVE);
    }
    if (!ss.tris.empty()) {
        for (k = 0; k < ss.ntri; k++)
            FiffCoordTrans::apply_trans(ss.tris[k].nn.data(),t,FIFFV_NO_MOVE);
    }
    ss.coord_frame = t.to;
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::transform_source_spaces_to(int coord_frame, const FiffCoordTrans& t, std::vector<std::unique_ptr<MneSourceSpaceOld>>& spaces)
/*
 * Facilitate the transformation of the source spaces
 */
{
    MneSourceSpaceOld* s;
    int k;
    int nspace = static_cast<int>(spaces.size());

    for (k = 0; k < nspace; k++) {
        s = spaces[k].get();
        if (s->coord_frame != coord_frame) {
            if (!t.isEmpty()) {
                if (s->coord_frame == t.from && t.to == coord_frame) {
                    if (transform_source_space(*s,t) != OK)
                        return FAIL;
                }
                else if (s->coord_frame == t.to && t.from == coord_frame) {
                    FiffCoordTrans my_t = t.inverted();
                    if (transform_source_space(*s,my_t) != OK) {
                        return FAIL;
                    }
                }
                else {
                    printf("Could not transform a source space because of transformation incompatibility.");
                    return FAIL;
                }
            }
            else {
                printf("Could not transform a source space because of missing coordinate transformation.");
                return FAIL;
            }
        }
    }
    return OK;
}

//=============================================================================================================

void MneSurfaceOrVolume::enable_all_sources(MneSourceSpaceOld& s)
{
    int k;
    for (k = 0; k < s.np; k++)
        s.inuse[k] = TRUE;
    s.nuse = s.np;
    return;
}

//=============================================================================================================

#define LH_LABEL_TAG "-lh.label"
#define RH_LABEL_TAG "-rh.label"

int MneSurfaceOrVolume::restrict_sources_to_labels(std::vector<std::unique_ptr<MneSourceSpaceOld>>& spaces, const QStringList& labels, int nlabel)
/*
 * Pick only sources within a label
 */
{
    MneSourceSpaceOld* lh = NULL;
    MneSourceSpaceOld* rh = NULL;
    MneSourceSpaceOld* sp;
    Eigen::VectorXi lh_inuse;
    Eigen::VectorXi rh_inuse;
    Eigen::VectorXi sel;
    Eigen::VectorXi *inuse = nullptr;
    int            k,p;
    int nspace = static_cast<int>(spaces.size());

    if (nlabel == 0)
        return OK;

    for (k = 0; k < nspace; k++) {
        if (is_left_hemi(*spaces[k])) {
            lh = spaces[k].get();
            lh_inuse = Eigen::VectorXi::Zero(lh->np);
        }
        else {
            rh = spaces[k].get();
            rh_inuse = Eigen::VectorXi::Zero(rh->np);
        }
    }
    /*
       * Go through each label file
       */
    for (k = 0; k < nlabel; k++) {
        /*
         * Which hemi?
         */
        if (labels[k].contains(LH_LABEL_TAG)){ //strstr(labels[k],LH_LABEL_TAG) != NULL) {
            sp = lh;
            inuse = &lh_inuse;
        }
        else if (labels[k].contains(RH_LABEL_TAG)){ //strstr(labels[k],RH_LABEL_TAG) != NULL) {
            sp = rh;
            inuse = &rh_inuse;
        }
        else {
            printf("\tWarning: cannot assign label file %s to a hemisphere.\n",labels[k].toUtf8().constData());
            continue;
        }
        if (sp) {
            if (read_label(labels[k],sel) == FAIL)
                goto bad;
            for (p = 0; p < sel.size(); p++) {
                if (sel[p] >= 0 && sel[p] < sp->np)
                    (*inuse)[sel[p]] = sp->inuse[sel[p]];
                else
                    printf("vertex number out of range in %s (%d vs %d)\n",
                           labels[k].toUtf8().constData(),sel[p],sp->np);
            }
            printf("Processed label file %s\n",labels[k].toUtf8().constData());
        }
    }
    if (lh) update_inuse(*lh, std::move(lh_inuse));
    if (rh) update_inuse(*rh, std::move(rh_inuse));
    return OK;

bad : {
        return FAIL;
    }
}

//=============================================================================================================

int MneSurfaceOrVolume::read_label(const QString& label, Eigen::VectorXi& sel)
/*
          * Find the source points within a label
          */
{
    int  res = FAIL;

    int k,p,nlabel;
    char c;
    float fdum;
    /*
       * Read the label file
       */
    QFile inFile(label);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << label;//err_set_sys_error(label);
        goto out;
    }
    inFile.getChar(&c);
    if (c !='#') {
        qCritical("Label file does not start correctly.");
        goto out;
    }
    /*
       * Skip the comment line
       */
    while (inFile.getChar(&c) && c != '\n')
        ;
    {
    QTextStream in(&inFile);
    in >> nlabel;
    if (in.status() != QTextStream::Ok) {
        qCritical("Could not read the number of labelled points.");
        goto out;
    }
    sel.resize(nlabel);
    for (k = 0; k < nlabel; k++) {
        in >> p >> fdum >> fdum >> fdum >> fdum;
        if (in.status() != QTextStream::Ok) {
            qCritical("Could not read label point # %d",k+1);
            goto out;
        }
        sel[k] = p;
    }
    res = OK;
    }

out : {
        if (res != OK) {
            sel.resize(0);
        }
        return res;
    }
}

//=============================================================================================================

void MneSurfaceOrVolume::add_triangle_data(MneSourceSpaceOld& s)
/*
     * Add the triangle data structures
     */
{
    int k;
    MneTriangle* tri;

    if (s.type != MNE_SOURCE_SPACE_SURFACE)
        return;

    s.tris.clear();
    s.use_tris.clear();
    /*
        * Add information for the complete triangulation
        */
    if (s.itris.rows() > 0 && s.ntri > 0) {
        s.tris.resize(s.ntri);
        s.tot_area = 0.0;
        for (k = 0, tri = s.tris.data(); k < s.ntri; k++, tri++) {
            tri->vert = &s.itris(k,0);
            tri->r1   = &s.rr(tri->vert[0],0);
            tri->r2   = &s.rr(tri->vert[1],0);
            tri->r3   = &s.rr(tri->vert[2],0);
            tri->compute_data();
            s.tot_area += tri->area;
        }
#ifdef TRIANGLE_SIZE_WARNING
        for (k = 0, tri = s.tris.data(); k < s.ntri; k++, tri++)
            if (tri->area < 1e-5*s.tot_area/s.ntri)
                printf("Warning: Triangle area is only %g um^2 (%.5f %% of expected average)\n",
                       1e12*tri->area,100*s.ntri*tri->area/s.tot_area);
#endif
    }
#ifdef DEBUG
    printf("\ttotal area = %-.1f cm^2\n",1e4*s.tot_area);
#endif
    /*
       * Add information for the selected subset if applicable
       */
    if (s.use_itris.rows() > 0 && s.nuse_tri > 0) {
        s.use_tris.resize(s.nuse_tri);
        for (k = 0, tri = s.use_tris.data(); k < s.nuse_tri; k++, tri++) {
            tri->vert = &s.use_itris(k,0);
            tri->r1   = &s.rr(tri->vert[0],0);
            tri->r2   = &s.rr(tri->vert[1],0);
            tri->r3   = &s.rr(tri->vert[2],0);
            tri->compute_data();
        }
    }
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::compute_cm(const MneSurfaceOrVolume::PointsT& rr, int np, float (&cm)[3])
/*
 * Compute the center of mass of a set of points
 */
{
    int q;
    cm[0] = cm[1] = cm[2] = 0.0;
    for (q = 0; q < np; q++) {
        cm[0] += rr(q,0);
        cm[1] += rr(q,1);
        cm[2] += rr(q,2);
    }
    if (np > 0) {
        cm[0] = cm[0]/np;
        cm[1] = cm[1]/np;
        cm[2] = cm[2]/np;
    }
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::compute_surface_cm(MneSurfaceOld &s)
/*
     * Compute the center of mass of a surface
     */
{
    compute_cm(s.rr,s.np,s.cm);
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::calculate_vertex_distances(MneSourceSpaceOld& s)
{
    int   k,p,ndist;
    float diff[3];
    int   nneigh;

    if (s.neighbor_vert.empty() || s.nneighbor_vert.size() == 0)
        return;

    s.vert_dist.clear();
    s.vert_dist.resize(s.np);
    printf("\tDistances between neighboring vertices...");
    for (k = 0, ndist = 0; k < s.np; k++) {
        nneigh = s.nneighbor_vert[k];
        s.vert_dist[k] = Eigen::VectorXf(nneigh);
        const Eigen::VectorXi& neigh = s.neighbor_vert[k];
        for (p = 0; p < nneigh; p++) {
            if (neigh[p] >= 0) {
                VEC_DIFF_17(&s.rr(k,0),&s.rr(neigh[p],0),diff);
                s.vert_dist[k][p] = VEC_LEN_17(diff);
            }
            else
                s.vert_dist[k][p] = -1.0;
            ndist++;
        }
    }
    printf("[%d distances done]\n",ndist);
    return;
}

//=============================================================================================================

int MneSurfaceOrVolume::add_vertex_normals(MneSourceSpaceOld& s)
{
    int k,c,p;
    int *ii;
    float w,size;
    MneTriangle* tri;

    if (s.type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
       * Reallocate the stuff and initialize
       */
    s.nn = MneSurfaceOrVolume::NormalsT::Zero(s.np,3);

    for (k = 0; k < s.np; k++) {
        s.nn(k,X_17) = s.nn(k,Y_17) = s.nn(k,Z_17) = 0.0;
    }
    /*
       * One pass through the triangles will do it
       */
    MneSurfaceOrVolume::add_triangle_data(s);
    for (p = 0, tri = s.tris.data(); p < s.ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        /*
         * Then the vertex normals
         */
        for (k = 0; k < 3; k++)
            for (c = 0; c < 3; c++)
                s.nn(ii[k],c) += w*tri->nn[c];
    }
    for (k = 0; k < s.np; k++) {
        size = VEC_LEN_17(&s.nn(k,0));
        if (size > 0.0)
            for (c = 0; c < 3; c++)
                s.nn(k,c) = s.nn(k,c)/size;
    }
    compute_surface_cm(reinterpret_cast<MneSurfaceOld&>(s));
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::add_geometry_info(MneSourceSpaceOld& s, int do_normals, int check_too_many_neighbors)
/*
          * Add vertex normals and neighbourhood information
          */
{
    int k,c,p,q;
    int vert;
    int *ii;
    int nneighbors;
    float w,size;
    int   found;
    int   nfix_distinct,nfix_no_neighbors,nfix_defect;
    MneTriangle* tri;

    if (s.type == MNE_SOURCE_SPACE_VOLUME) {
        calculate_vertex_distances(s);
        return OK;
    }
    if (s.type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
       * Reallocate the stuff and initialize
       */
    if (do_normals) {
        s.nn = MneSurfaceOrVolume::NormalsT::Zero(s.np,3);
    }
    s.neighbor_tri.clear();
    s.neighbor_tri.resize(s.np);
    s.nneighbor_tri = Eigen::VectorXi::Zero(s.np);

    for (k = 0; k < s.np; k++) {
        if (do_normals)
            s.nn(k,X_17) = s.nn(k,Y_17) = s.nn(k,Z_17) = 0.0;
    }
    /*
       * One pass through the triangles will do it
       */
    add_triangle_data(s);
    for (p = 0, tri = s.tris.data(); p < s.ntri; p++, tri++)
        if (tri->area == 0)
            printf("\tWarning : zero size triangle # %d\n",p);
    printf("\tTriangle ");
    if (do_normals)
        printf("and vertex ");
    printf("normals and neighboring triangles...");
    for (p = 0, tri = s.tris.data(); p < s.ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        for (k = 0; k < 3; k++) {
            /*
           * Then the vertex normals
           */
            if (do_normals)
                for (c = 0; c < 3; c++)
                    s.nn(ii[k],c) += w*tri->nn[c];
            /*
           * Add to the list of neighbors
           */
            s.neighbor_tri[ii[k]].conservativeResize(s.nneighbor_tri[ii[k]]+1);
            s.neighbor_tri[ii[k]][s.nneighbor_tri[ii[k]]] = p;
            s.nneighbor_tri[ii[k]]++;
        }
    }
    nfix_no_neighbors = 0;
    nfix_defect = 0;
    for (k = 0; k < s.np; k++) {
        if (s.nneighbor_tri[k] <= 0) {
#ifdef STRICT_ERROR
            err_printf_set_error("Vertex %d does not have any neighboring triangles!",k);
            return FAIL;
#else
#ifdef REPORT_WARNINGS
            printf("Warning: Vertex %d does not have any neighboring triangles!\n",k);
#endif
#endif
            nfix_no_neighbors++;
        }
        else if (s.nneighbor_tri[k] < 3) {
#ifdef REPORT_WARNINGS
            printf("\n\tTopological defect: Vertex %d has only %d neighboring triangle%s Vertex omitted.\n\t",
                   k,s.nneighbor_tri[k],s.nneighbor_tri[k] > 1 ? "s." : ".");
#endif
            nfix_defect++;
            s.nneighbor_tri[k] = 0;
            s.neighbor_tri[k].resize(0);
        }
    }
    /*
       * Scale the vertex normals to unit length
       */
    for (k = 0; k < s.np; k++)
        if (s.nneighbor_tri[k] > 0) {
            size = VEC_LEN_17(&s.nn(k,0));
            if (size > 0.0)
                for (c = 0; c < 3; c++)
                    s.nn(k,c) = s.nn(k,c)/size;
        }
    printf("[done]\n");
    /*
       * Determine the neighboring vertices
       */
    printf("\tVertex neighbors...");
    s.neighbor_vert.clear();
    s.neighbor_vert.resize(s.np);
    s.nneighbor_vert = VectorXi::Zero(s.np);
    /*
       * We know the number of neighbors beforehand
       */
    for (k = 0; k < s.np; k++) {
        if (s.nneighbor_tri[k] > 0) {
            s.neighbor_vert[k]  = VectorXi(s.nneighbor_tri[k]);
            s.nneighbor_vert[k] = s.nneighbor_tri[k];
        }
        else {
            s.nneighbor_vert[k] = 0;
        }
    }
    nfix_distinct = 0;
    for (k = 0; k < s.np; k++) {
        Eigen::VectorXi& neighbors = s.neighbor_vert[k];
        nneighbors = 0;
        for (p = 0; p < s.nneighbor_tri[k]; p++) {
            /*
           * Fit in the other vertices of the neighboring triangle
           */
            for (c = 0; c < 3; c++) {
                vert = s.tris[s.neighbor_tri[k][p]].vert[c];
                if (vert != k) {
                    for (q = 0, found = FALSE; q < nneighbors; q++) {
                        if (neighbors[q] == vert) {
                            found = TRUE;
                            break;
                        }
                    }
                    if (!found) {
                        if (nneighbors < s.nneighbor_vert[k])
                            neighbors[nneighbors++] = vert;
                        else {
                            if (check_too_many_neighbors) {
                                printf("Too many neighbors for vertex %d.",k);
                                return FAIL;
                            }
                            else
                                printf("\tWarning: Too many neighbors for vertex %d\n",k);
                        }
                    }
                }
            }
        }
        if (nneighbors != s.nneighbor_vert[k]) {
#ifdef REPORT_WARNINGS
            printf("\n\tIncorrect number of distinct neighbors for vertex %d (%d instead of %d) [fixed].",
                   k,nneighbors,s.nneighbor_vert[k]);
#endif
            nfix_distinct++;
            s.nneighbor_vert[k] = nneighbors;
        }
    }
    printf("[done]\n");
    /*
       * Distance calculation follows
       */
    calculate_vertex_distances(s);
    compute_surface_cm(reinterpret_cast<MneSurfaceOld&>(s));
    /*
       * Summarize the defects
       */
    if (nfix_defect > 0)
        printf("\tWarning: %d topological defects were fixed.\n",nfix_defect);
    if (nfix_distinct > 0)
        printf("\tWarning: %d vertices had incorrect number of distinct neighbors (fixed).\n",nfix_distinct);
    if (nfix_no_neighbors > 0)
        printf("\tWarning: %d vertices did not have any neighboring triangles (fixed)\n",nfix_no_neighbors);
#ifdef DEBUG
    for (k = 0; k < s.np; k++) {
        if (s.nneighbor_vert[k] <= 0)
            printf("No neighbors for vertex %d\n",k);
        if (s.nneighbor_tri[k] <= 0)
            printf("No neighbor tris for vertex %d\n",k);
    }
#endif
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::add_geometry_info(MneSourceSpaceOld& s, int do_normals)
{
    return add_geometry_info(s,do_normals,TRUE);
}

//=============================================================================================================

int MneSurfaceOrVolume::add_geometry_info2(MneSourceSpaceOld& s, int do_normals)

{
    return add_geometry_info(s,do_normals,FALSE);
}

//=============================================================================================================

// Align the MEG fiducials to the MRI fiducials
int MneSurfaceOrVolume::align_fiducials(FiffDigitizerData& head_dig,
                                        const FiffDigitizerData& mri_dig,
                                        MneMshDisplaySurface* head_surf,
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
        get_head_scale(head_dig,mri_fid,head_surf,scales);
        printf("xscale = %.3f yscale = %.3f zscale = %.3f\n",scales[0],scales[1],scales[2]);

        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                mri_fid(j,k) = mri_fid(j,k)*scales[k];

        if (head_surf)
            scale_display_surface(*head_surf,scales);
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
        discard_outlier_digitizer_points(head_dig,head_surf,omit_dist);

    // Optional iterative refinement
    if (niter > 0 && head_surf) {
        for (k = 0; k < niter; k++) {
            if (iterate_alignment_once(head_dig,*head_surf,nasion_weight,Eigen::Vector3f(mri_fid.row(1).transpose()),k == niter-1 && niter > 1) == FAIL)
                goto bad;
        }

        printf("%d / %d iterations done. RMS dist = %7.1f mm\n",k,niter,
                1000.0*rms_digitizer_distance(head_dig,*head_surf));
        printf("After refinement :\n");
        head_dig.head_mri_t_adj->print();
    }

    return OK;

bad :
    return FAIL;
}

//=============================================================================================================

// Simple head size fit
void MneSurfaceOrVolume::get_head_scale(FIFFLIB::FiffDigitizerData& dig,
                                        const Eigen::Matrix<float, 3, 3, Eigen::RowMajor>& mri_fid,
                                        MneMshDisplaySurface* head_surf,
                                        Eigen::Vector3f& scales)
{
    float **dig_rr  = NULL;
    float **head_rr = NULL;
    int   k,ndig,nhead;
    float simplex_size = 2e-2;
    float r0[3],Rdig,Rscalp;
    float LR[3],LN[3],len,norm[3],diff[3];

    scales[0] = scales[1] = scales[2] = 1.0;
    if (!head_surf){
        return;
    }

    dig_rr  = MALLOC_17(dig.npoint,float *);
    head_rr = MALLOC_17(head_surf->s->np,float *);

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

    for (k = 0, nhead = 0; k < head_surf->s->np; k++) {
        VEC_DIFF_17(mri_fid.row(0).data(),&head_surf->s->rr(k,0),diff);
        if (VEC_DOT_17(diff,norm) > 0) {
            head_rr[nhead++] = &head_surf->s->rr(k,0);
        }
    }

    if (!UTILSLIB::Sphere::fit_sphere_to_points(head_rr,nhead,simplex_size,r0,&Rscalp)) {
        goto out;
    }

    printf("Scalp : (%.1f %.1f %.1f) mm R = %.1f mm\n",1000*r0[X_17],1000*r0[Y_17],1000*r0[Z_17],1000*Rscalp);

    scales[0] = scales[1] = scales[2] = Rdig/Rscalp;

out : {
        FREE_17(dig_rr);
        FREE_17(head_rr);
        return;
    }
}

//=============================================================================================================

int MneSurfaceOrVolume::discard_outlier_digitizer_points(FIFFLIB::FiffDigitizerData& d,
                                                         const MneMshDisplaySurface* head,
                                                         float maxdist)
/*
      * Discard outlier digitizer points
      */
{
    int discarded = 0;
    int k;

    if (head) {
        d.dist_valid = false;
        calculate_digitizer_distances(d,*head,TRUE,TRUE);
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
    }
    return discarded;
}

//=============================================================================================================

void MneSurfaceOrVolume::calculate_digitizer_distances(FIFFLIB::FiffDigitizerData& dig, const MneMshDisplaySurface& head,
                                                       int do_all, int do_approx)
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

    find_closest_on_surface_approx(*head.s,rr,nactive,closest,dists,nstep);
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
                Eigen::Vector3f proj = project_to_triangle(*head.s,dig.closest(k),pt);
                dig.closest_point.row(k) = proj.transpose();
            }
            /*
            * The above distance is with respect to the closest triangle only
            * We need to use the solid angle criterion to decide the sign reliably
            */
            if (!do_approx && false) {
                Eigen::Vector3f pt = Eigen::Map<const Eigen::Vector3f>(rr.row(nactive).data());
                if (sum_solids(pt,*head.s)/(4*M_PI) > 0.9)
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

int MneSurfaceOrVolume::iterate_alignment_once(FIFFLIB::FiffDigitizerData& dig,	   /* The digitizer data */
                                               const MneMshDisplaySurface& head, /* The head surface */
                                               int nasion_weight,	   /* Weight for the nasion */
                                               const std::optional<Eigen::Vector3f>& nasion_mri,   /* Fixed correspondence point for the nasion (optional) */
                                               int last_step)          /* Is this the last iteration step */
/*
 * Find the best alignment of the coordinate frames
 */
{
    int   res       = FAIL;
    float **rr_head = NULL;
    float **rr_mri  = NULL;
    float *w        = NULL;
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
    calculate_digitizer_distances(dig,head,FALSE,TRUE);

    /*
     * Set up the alignment
     */
    rr_head = ALLOC_CMATRIX_17(dig.npoint,3);
    rr_mri  = ALLOC_CMATRIX_17(dig.npoint,3);
    w       = MALLOC_17(dig.npoint,float);

    for (k = 0, nactive = 0; k < dig.npoint; k++) {
        if (dig.active[k] && !dig.discard[k]) {
            point = dig.points.at(k);
            VEC_COPY_17(rr_head[nactive],point.r);
            VEC_COPY_17(rr_mri[nactive],dig.closest_point.row(k).data());
            /*
            * Special handling for the nasion
            */
            if (point.kind == FIFFV_POINT_CARDINAL &&
                    point.ident == FIFFV_POINT_NASION) {
                w[nactive] = nasion_weight;
                if (nasion_mri) {
                    VEC_COPY_17(rr_mri[nactive],nasion_mri->data());
                    VEC_COPY_17(rr_head[nactive],nasion_mri->data());
                    Q_ASSERT(dig.head_mri_t || dig.head_mri_t_adj);
                    FiffCoordTrans::apply_inverse_trans(rr_head[nactive],
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
                                                 rr_head, rr_mri, w, nactive, max_diff)).isEmpty())
        goto out;

    if (dig.head_mri_t_adj)
        dig.head_mri_t_adj = std::make_unique<FiffCoordTrans>(t);
    /*
     * Calculate final distances
     */
    dig.dist_valid = false;
    calculate_digitizer_distances(dig,head,FALSE,!last_step);
    res = OK;
    goto out;

out : {
        FREE_CMATRIX_17(rr_head);
        FREE_CMATRIX_17(rr_mri);
        FREE_17(w);
        return res;
    }
}

//=============================================================================================================

float MneSurfaceOrVolume::rms_digitizer_distance(FIFFLIB::FiffDigitizerData& dig, const MneMshDisplaySurface& head)
{
    float rms;
    int   k,nactive;

    calculate_digitizer_distances(dig,head,FALSE,TRUE);

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

void MneSurfaceOrVolume::scale_display_surface(MneMshDisplaySurface& surf,
                                               const Eigen::Vector3f& scales)
/*
 * Not quite complete yet
 */
{
    int j,k;

    for (k = 0; k < 3; k++) {
        surf.minv[k] = scales[k]*surf.minv[k];
        surf.maxv[k] = scales[k]*surf.maxv[k];
    }
    for (j = 0; j < surf.s->np; j++)
        for (k = 0; k < 3; k++)
            surf.s->rr(j,k) = surf.s->rr(j,k)*scales[k];
    return;
}

//=============================================================================================================

void MneSurfaceOrVolume::add_uniform_curv(MneSurfaceOld &s)
{
    if (s.curv.size() > 0)
        return;
    s.curv = VectorXf::Ones(s.np);
    return;
}

//=============================================================================================================

MneSourceSpaceOld* MneSurfaceOrVolume::load_surface(const QString& surf_file,
                                                        const QString& curv_file)
{
    return load_surface_geom(surf_file,curv_file,TRUE,TRUE);
}

//=============================================================================================================

MneSourceSpaceOld* MneSurfaceOrVolume::load_surface_geom(const QString& surf_file,
                                                             const QString& curv_file,
                                                             int  add_geometry,
                                                             int  check_too_many_neighbors)
    /*
     * Load the surface and add the geometry information
     */
{
    int   k;
    MneSourceSpaceOld* s = Q_NULLPTR;
    std::unique_ptr<MneMghTagGroup> tags;
    Eigen::VectorXf curvs;
    PointsT verts;
    TrianglesT tris;

    if (read_triangle_file(surf_file,
                               verts,
                               tris,
                               &tags) == -1)
        goto bad;

    if (!curv_file.isEmpty()) {
        if (read_curvature_file(curv_file, curvs) == -1)
            goto bad;
        if (curvs.size() != verts.rows()) {
            qCritical()<<"Incorrect number of vertices in the curvature file.";
            goto bad;
        }
    }

    s = new MneSourceSpaceOld(0);
    s->rr   = std::move(verts);
    s->itris = std::move(tris);
    s->ntri = s->itris.rows();
    s->np   = s->rr.rows();
    if (curvs.size() > 0) {
        s->curv = std::move(curvs);
    }
    s->val = Eigen::VectorXf::Zero(s->np);
    if (add_geometry) {
        if (check_too_many_neighbors) {
            if (add_geometry_info(*s,TRUE) != OK)
                goto bad;
        }
        else {
            if (add_geometry_info2(*s,TRUE) != OK)
                goto bad;
        }
    }
    else if (s->nn.rows() == 0) {			/* Normals only */
        if (add_vertex_normals(*s) != OK)
            goto bad;
    }
    else
        add_triangle_data(*s);
    s->nuse   = s->np;
    s->inuse  = Eigen::VectorXi::Ones(s->np);
    s->vertno = Eigen::VectorXi::LinSpaced(s->np, 0, s->np - 1);
    s->mgh_tags = std::move(tags);
    s->vol_geom = get_volume_geom_from_tag(s->mgh_tags.get());

    return s;

bad : {
        delete s;
        return Q_NULLPTR;
    }
}

//=============================================================================================================

int MneSurfaceOrVolume::read_triangle_file(const QString& fname,
                                               PointsT& vertices,
                                               TrianglesT& triangles,
                                               std::unique_ptr<MneMghTagGroup>* tagsp)
/*
      * Read the FS triangulated surface
      */
{
    QFile fp(fname);
    int  magic;
    char c;

    qint32  nvert,ntri,nquad;
    float **vert = NULL;
    int   **tri  = NULL;
    int   k,p;
    int   quad[4];
    int   val;
    float *rr[5];
    int   which;

    if (!fp.open(QIODevice::ReadOnly)) {
        qCritical() << fname;
        goto bad;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic != TRIANGLE_FILE_MAGIC_NUMBER &&
            magic != QUAD_FILE_MAGIC_NUMBER &&
            magic != NEW_QUAD_FILE_MAGIC_NUMBER) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic == TRIANGLE_FILE_MAGIC_NUMBER) {
        /*
     * Get the comment
     */
        printf("Triangle file : ");
        for (fp.getChar(&c); c != '\n'; fp.getChar(&c)) {
            if (fp.atEnd()) {
                qCritical()<<"Bad triangle file.";
                goto bad;
            }
            putc(c,stderr);
        }
        fp.getChar(&c);
        /*
     * How many vertices and triangles?
     */
        if (read_int(fp,nvert) != 0)
            goto bad;
        if (read_int(fp,ntri) != 0)
            goto bad;
        printf(" nvert = %d ntri = %d\n",nvert,ntri);
        vert = ALLOC_CMATRIX_17(nvert,3);
        tri  = ALLOC_ICMATRIX_17(ntri,3);
        /*
     * Read the vertices
     */
        for (k = 0; k < nvert; k++) {
            if (read_float(fp,vert[k][X_17]) != 0)
                goto bad;
            if (read_float(fp,vert[k][Y_17]) != 0)
                goto bad;
            if (read_float(fp,vert[k][Z_17]) != 0)
                goto bad;
        }
        /*
     * Read the triangles
     */
        for (k = 0; k < ntri; k++) {
            if (read_int(fp,tri[k][X_17]) != 0)
                goto bad;
            if (check_vertex(tri[k][X_17],nvert) != OK)
                goto bad;
            if (read_int(fp,tri[k][Y_17]) != 0)
                goto bad;
            if (check_vertex(tri[k][Y_17],nvert) != OK)
                goto bad;
            if (read_int(fp,tri[k][Z_17]) != 0)
                goto bad;
            if (check_vertex(tri[k][Z_17],nvert) != OK)
                goto bad;
        }
    }
    else if (magic == QUAD_FILE_MAGIC_NUMBER ||
             magic == NEW_QUAD_FILE_MAGIC_NUMBER) {
        if (read_int3(fp,nvert) != 0)
            goto bad;
        if (read_int3(fp,nquad) != 0)
            goto bad;
        printf("%s file : nvert = %d nquad = %d\n",
                magic == QUAD_FILE_MAGIC_NUMBER ? "Quad" : "New quad",
                nvert,nquad);
        vert = ALLOC_CMATRIX_17(nvert,3);
        if (magic == QUAD_FILE_MAGIC_NUMBER) {
            for (k = 0; k < nvert; k++) {
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert[k][X_17] = val/100.0;
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert[k][Y_17] = val/100.0;
                if (read_int2(fp,val) != 0)
                    goto bad;
                vert[k][Z_17] = val/100.0;
            }
        }
        else {			/* NEW_QUAD_FILE_MAGIC_NUMBER */
            for (k = 0; k < nvert; k++) {
                if (read_float(fp,vert[k][X_17]) != 0)
                    goto bad;
                if (read_float(fp,vert[k][Y_17]) != 0)
                    goto bad;
                if (read_float(fp,vert[k][Z_17]) != 0)
                    goto bad;
            }
        }
        ntri = 2*nquad;
        tri  = ALLOC_ICMATRIX_17(ntri,3);
        for (k = 0, ntri = 0; k < nquad; k++) {
            for (p = 0; p < 4; p++) {
                if (read_int3(fp,quad[p]) != 0)
                    goto bad;
                rr[p] = vert[quad[p]];
            }
            rr[4] = vert[quad[0]];

            /*
     * The randomization is borrowed from FreeSurfer code
     * Strange...
     */
#define EVEN(n)      ((((n) / 2) * 2) == n)
#ifdef FOO
#define WHICH_FACE_SPLIT(vno0, vno1) \
    (1*nearbyint(sqrt(1.9*vno0) + sqrt(3.5*vno1)))

            which = WHICH_FACE_SPLIT(quad[0], quad[1]) ;
#endif
            which = quad[0];
            /*
    printf("%f ",sqrt(1.9*quad[0]) + sqrt(3.5*quad[1]));
     */

            if (EVEN(which)) {
                tri[ntri][X_17] = quad[0];
                tri[ntri][Y_17] = quad[1];
                tri[ntri][Z_17] = quad[3];
                ntri++;

                tri[ntri][X_17] = quad[2];
                tri[ntri][Y_17] = quad[3];
                tri[ntri][Z_17] = quad[1];
                ntri++;
            }
            else {
                tri[ntri][X_17] = quad[0];
                tri[ntri][Y_17] = quad[1];
                tri[ntri][Z_17] = quad[2];
                ntri++;

                tri[ntri][X_17] = quad[0];
                tri[ntri][Y_17] = quad[2];
                tri[ntri][Z_17] = quad[3];
                ntri++;
            }
        }
    }
    /*
     * Optionally read the tags
     */
    if (tagsp) {
        std::unique_ptr<MneMghTagGroup> tags;
        if (read_mgh_tags(fp, tags) == FAIL) {
            goto bad;
        }
        *tagsp = std::move(tags);
    }
    /*
     * Convert mm to m and store as Eigen matrices
     */
    for (k = 0; k < nvert; k++) {
        vert[k][X_17] = vert[k][X_17]/1000.0;
        vert[k][Y_17] = vert[k][Y_17]/1000.0;
        vert[k][Z_17] = vert[k][Z_17]/1000.0;
    }
    vertices = Eigen::Map<PointsT>(vert[0], nvert, 3);
    FREE_CMATRIX_17(vert);
    triangles = Eigen::Map<TrianglesT>(tri[0], ntri, 3);
    FREE_ICMATRIX_17(tri);
    return OK;

bad : {
        FREE_CMATRIX_17(vert);
        FREE_ICMATRIX_17(tri);
        return FAIL;
    }
}

//=============================================================================================================

int MneSurfaceOrVolume::read_curvature_file(const QString& fname,
                                                Eigen::VectorXf& curv)

{
    QFile fp(fname);
    int  magic;

    float curvmin,curvmax;
    int   ncurv  = 0;
    int   nface,val_pervert;
    int   val,k;
    float fval;

    if (!fp.open(QIODevice::ReadOnly)) {
        qCritical() << fname;
        goto bad;
    }
    if (read_int3(fp,magic) != 0) {
        qCritical() << "Bad magic in" << fname;
        goto bad;
    }
    if (magic == CURVATURE_FILE_MAGIC_NUMBER) {	    /* A new-style curvature file */
        /*
 * How many and faces
 */
        if (read_int(fp,ncurv) != 0)
            goto bad;
        if (read_int(fp,nface) != 0)
            goto bad;
#ifdef DEBUG
        printf("nvert = %d nface = %d\n",ncurv,nface);
#endif
        if (read_int(fp,val_pervert) != 0)
            goto bad;
        if (val_pervert != 1) {
            qCritical("Values per vertex not equal to one.");
            goto bad;
        }
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_float(fp,fval) != 0)
                goto bad;
            curv[k] = fval;
            if (curv[k] > curvmax)
                curvmax = curv[k];
            if (curv[k] < curvmin)
                curvmin = curv[k];
        }
    }
    else {			                    /* An old-style curvature file */
        ncurv = magic;
        /*
 * How many vertices
 */
        if (read_int3(fp,nface) != 0)
            goto bad;
#ifdef DEBUG
        printf("nvert = %d nface = %d\n",ncurv,nface);
#endif
        /*
 * Read the curvature values
 */
        curv.resize(ncurv);
        curvmin = curvmax = 0.0;
        for (k = 0; k < ncurv; k++) {
            if (read_int2(fp,val) != 0)
                goto bad;
            curv[k] = (float)val/100.0;
            if (curv[k] > curvmax)
                curvmax = curv[k];
            if (curv[k] < curvmin)
                curvmin = curv[k];

        }
    }
#ifdef DEBUG
    printf("Curvature range: %f...%f\n",curvmin,curvmax);
#endif
    return OK;

bad : {
        curv.resize(0);
        return FAIL;
    }
}

//=============================================================================================================

int MneSurfaceOrVolume::check_vertex(int no, int maxno)

{
    if (no < 0 || no > maxno-1) {
        printf("Illegal vertex number %d (max %d).",no,maxno);
        return FAIL;
    }
    return OK;
}

//=============================================================================================================

std::unique_ptr<MneVolGeom> MneSurfaceOrVolume::get_volume_geom_from_tag(const MneMghTagGroup *tagsp)
{
    MneMghTag*      tag  = NULL;

    if (tagsp) {
        for (const auto &t : tagsp->tags)
            if (t->tag == TAG_OLD_SURF_GEOM) {
                tag = t.get();
                break;
            }
        if (tag)
            return dup_vol_geom(*reinterpret_cast<MneVolGeom*>(tag->data.data()));
    }
    return nullptr;
}

//=============================================================================================================

std::unique_ptr<MneVolGeom> MneSurfaceOrVolume::dup_vol_geom(const MneVolGeom& g)
{
    auto dup = std::make_unique<MneVolGeom>();
    *dup = g;
    dup->filename = g.filename;
    return dup;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_mgh_tags(QFile &fp, std::unique_ptr<MneMghTagGroup>& tagsp)
/*
 * Read all the tags from the file
 */
{
    long long     len;
    int           tag;
    unsigned char *tag_data;

    while (1) {
        if (read_next_tag(fp,tag,len,tag_data) == FAIL)
            return FAIL;
        if (tag == 0)
            break;
        add_mgh_tag_to_group(tagsp,tag,len,tag_data);
    }
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_next_tag(QFile &fp, int &tagp, long long &lenp, unsigned char *&datap)
/*
 * Read the next tag in the file
 */
{
    int       ilen,tag;
    long long len;

    if (read_int(fp,tag) == FAIL) {
        tagp = 0;
        return OK;
    }
    if (fp.atEnd()) {
        tagp = 0;
        return OK;
    }
    switch (tag) {
    case TAG_OLD_MGH_XFORM: /* This is obviously a burden of the past */
        if (read_int(fp,ilen) == FAIL)
            return FAIL;
        len = ilen - 1;
        break ;
    case TAG_OLD_SURF_GEOM:
    case TAG_OLD_USEREALRAS:
    case TAG_OLD_COLORTABLE:
        len = 0 ;
        break ;
    default:
        if (read_long(fp,len) == FAIL)
            return FAIL;
        break;
    }
     lenp = len;
     tagp = tag;
    if (read_tag_data(fp,tag,len,datap,lenp) == FAIL)
        return FAIL;
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_tag_data(QFile &fp, int tag, long long nbytes, unsigned char *&val, long long &nbytesp)
/*
 * Read the data of one tag
 */
{
    unsigned char *dum = NULL;
    size_t snbytes = nbytes;

     val = NULL;
    if (nbytes > 0) {
        dum = MALLOC_17(nbytes+1,unsigned char);
        if (fp.read(reinterpret_cast<char*>(dum), nbytes) != static_cast<qint64>(snbytes)) {
            printf("Failed to read %d bytes of tag data",static_cast<int>(nbytes));
            FREE_17(dum);
            return FAIL;
        }
        dum[nbytes] = '\0'; /* Ensure null termination */
        val     = dum;
        nbytesp = nbytes;
    }
    else {			/* Need to handle special cases */
        if (tag == TAG_OLD_SURF_GEOM) {
            MneVolGeom* g = read_vol_geom(fp);
            if (!g)
                return FAIL;
            val     = (unsigned char *)g;
            nbytesp = sizeof(MneVolGeom);
        }
        else if (tag == TAG_OLD_USEREALRAS || tag == TAG_USEREALRAS) {
            int *vi = MALLOC_17(1,int);
            if (read_int(fp,*vi) == FAIL)
                vi = 0;
            val = (unsigned char *)vi;
            nbytesp = sizeof(int);
        }
        else {
            printf("Encountered an unknown tag with no length specification : %d\n",tag);
            val     = NULL;
            nbytesp = 0;
        }
    }
    return OK;
}

//=============================================================================================================

void MneSurfaceOrVolume::add_mgh_tag_to_group(std::unique_ptr<MneMghTagGroup>& g, int tag, long long len, unsigned char *data)
{
    if (!g)
        g = std::make_unique<MneMghTagGroup>();
    auto new_tag = std::make_unique<MneMghTag>();
    new_tag->tag  = tag;
    new_tag->len  = len;
    new_tag->data = QByteArray(reinterpret_cast<const char*>(data), static_cast<int>(len));
    free(data);
    g->tags.push_back(std::move(new_tag));
}

//=============================================================================================================

MneVolGeom* MneSurfaceOrVolume::read_vol_geom(QFile &fp)
/*
 * This the volume geometry reading code from FreeSurfer
 */
{
    char param[64];
    char eq[2];
    char buf[256];
    int vgRead = 0;
    int counter = 0;
    qint64 pos = 0;

    MneVolGeom* vg = new MneVolGeom();

    while (!fp.atEnd() && counter < 8)
    {
        QByteArray lineData = fp.readLine(256);
        if (lineData.isEmpty())
            break;
        const char *line = lineData.constData();
        if (strlen(line) == 0)
            break ;
        sscanf(line, "%s %s %*s", param, eq);
        if (!strcmp(param, "valid")) {
            sscanf(line, "%s %s %d", param, eq, &vg->valid);
            vgRead = 1;
            counter++;
        }
        else if (!strcmp(param, "filename")) {
            if (sscanf(line, "%s %s %s", param, eq, buf) >= 3)
                vg->filename = strdup(buf);
            counter++;
        }
        else if (!strcmp(param, "volume")) {
            sscanf(line, "%s %s %d %d %d",
                   param, eq, &vg->width, &vg->height, &vg->depth);
            counter++;
        }
        else if (!strcmp(param, "voxelsize")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, &vg->xsize, &vg->ysize, &vg->zsize);
            /*
       * We like these to be in meters
       */
            vg->xsize = vg->xsize/1000.0;
            vg->ysize = vg->ysize/1000.0;
            vg->zsize = vg->zsize/1000.0;
            counter++;
        }
        else if (!strcmp(param, "xras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->x_ras, vg->x_ras+1, vg->x_ras+2);
            counter++;
        }
        else if (!strcmp(param, "yras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->y_ras, vg->y_ras+1, vg->y_ras+2);
            counter++;
        }
        else if (!strcmp(param, "zras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->z_ras, vg->z_ras+1, vg->z_ras+2);
            counter++;
        }
        else if (!strcmp(param, "cras")) {
            sscanf(line, "%s %s %f %f %f",
                   param, eq, vg->c_ras, vg->c_ras+1, vg->c_ras+2);
            vg->c_ras[0] = vg->c_ras[0]/1000.0;
            vg->c_ras[1] = vg->c_ras[1]/1000.0;
            vg->c_ras[2] = vg->c_ras[2]/1000.0;
            counter++;
        }
        /* remember the current position */
        pos = fp.pos();
    };
    if (!fp.atEnd()) { /* we read one more line */
        if (pos > 0 ) /* if success in getting pos, then */
            fp.seek(pos); /* restore the position */
        /* note that this won't allow compression using pipe */
    }
    if (!vgRead) {
        delete vg;
        vg = new MneVolGeom();
    }
    return vg;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_int3(QFile &in, int &ival)
/*
 * Read the strange 3-byte integer
 */
{
    unsigned int s = 0;

    if (in.read(reinterpret_cast<char*>(&s), 3) != 3) {
        qCritical("read_int3 could not read data");
        return FAIL;
    }
    s = (unsigned int)UTILSLIB::IOUtils::swap_int(s);
    ival = ((s >> 8) & 0xffffff);
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_int(QFile &in, qint32 &ival)
/*
 * Read a 32-bit integer
 */
{
    qint32 s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(qint32)) != static_cast<qint64>(sizeof(qint32))) {
        qCritical("read_int could not read data");
        return FAIL;
    }
    ival = UTILSLIB::IOUtils::swap_int(s);
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_int2(QFile &in, int &ival)
/*
      * Read int from short
      */
{
    short s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(short)) != static_cast<qint64>(sizeof(short))) {
        qCritical("read_int2 could not read data");
        return FAIL;
    }
    ival = UTILSLIB::IOUtils::swap_short(s);
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_float(QFile &in, float &fval)
/*
      * Read float
      */
{
    float f ;
    if (in.read(reinterpret_cast<char*>(&f), sizeof(float)) != static_cast<qint64>(sizeof(float))) {
        qCritical("read_float could not read data");
        return FAIL;
    }
    fval = UTILSLIB::IOUtils::swap_float(f);
    return OK;
}

//=============================================================================================================

int MneSurfaceOrVolume::read_long(QFile &in, long long &lval)
/*
 * Read a 64-bit integer
 */
{
    long long s ;
    if (in.read(reinterpret_cast<char*>(&s), sizeof(long long)) != static_cast<qint64>(sizeof(long long))) {
        qCritical("read_long could not read data");
        return FAIL;
    }
    lval = UTILSLIB::IOUtils::swap_long(s);
    return OK;
}

