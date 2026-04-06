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
 * @brief    Definition of the MNE FsSurface or Volume (MNESurfaceOrVolume) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surface_or_volume.h"
#include "mne_surface.h"
#include "mne_source_space.h"
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

#include <math/sphere.h>
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


#define NNEIGHBORS 26

#define CURVATURE_FILE_MAGIC_NUMBER  (16777215)

#define TAG_MGH_XFORM               31
#define TAG_SURF_GEOM               21
#define TAG_OLD_USEREALRAS          2
#define TAG_COLORTABLE              5
#define TAG_OLD_MGH_XFORM           30
#define TAG_OLD_COLORTABLE          1

#define TAG_USEREALRAS              4


//===
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

static std::optional<FiffCoordTrans> make_voxel_ras_trans(float *r0,
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

    return FiffCoordTrans(FIFFV_MNE_COORD_MRI_VOXEL,FIFFV_COORD_MRI,
                                            Eigen::Map<Eigen::Matrix3f>(&rot[0][0]),
                                            Eigen::Map<Eigen::Vector3f>(move));
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurfaceOrVolume::MNESurfaceOrVolume()
{
}

//=============================================================================================================

void MNESurfaceOrVolume::add_uniform_curv()
{
    if (curv.size() > 0)
        return;
    curv = Eigen::VectorXf::Ones(np);
}

//=============================================================================================================

MNESurfaceOrVolume::~MNESurfaceOrVolume()
{
    // rr, nn, itris, use_itris are Eigen matrices — auto-cleanup
    // inuse, vertno are Eigen VectorXi — auto-cleanup
    // tris, use_tris are std::vector<MNETriangle> — auto-cleanup
    // neighbor_tri is std::vector<VectorXi> — auto-cleanup
    // nneighbor_tri is Eigen VectorXi — auto-cleanup
    // curv is Eigen VectorXf — auto-cleanup

    // neighbor_vert is std::vector<VectorXi> — auto-cleanup
    // nneighbor_vert is Eigen VectorXi — auto-cleanup
    // vert_dist is std::vector<VectorXf> — auto-cleanup
    // nearest is std::vector<MNENearest> — auto-cleanup
    // patches is std::vector<optional<MNEPatchInfo>> — auto-cleanup
    // dist is FiffSparseMatrix value, interpolator/vol_geom/mgh_tags are optional — auto-cleanup
    // voxel_surf_RAS_t, MRI_voxel_surf_RAS_t, MRI_surf_RAS_RAS_t are optional — auto-cleanup
    this->MRI_volume.clear();
}

//=============================================================================================================

Eigen::VectorXi MNESurfaceOrVolume::nearestVertIdx() const
{
    Eigen::VectorXi result(static_cast<int>(nearest.size()));
    for (int i = 0; i < result.size(); ++i)
        result[i] = nearest[i].nearest;
    return result;
}

//=============================================================================================================

Eigen::VectorXd MNESurfaceOrVolume::nearestDistVec() const
{
    Eigen::VectorXd result(static_cast<int>(nearest.size()));
    for (int i = 0; i < result.size(); ++i)
        result[i] = static_cast<double>(nearest[i].dist);
    return result;
}

//=============================================================================================================

void MNESurfaceOrVolume::setNearestData(const Eigen::VectorXi& nearestIdx, const Eigen::VectorXd& nearestDist)
{
    const int n = nearestIdx.size();
    nearest.resize(n);
    for (int i = 0; i < n; ++i) {
        nearest[i].vert = i;
        nearest[i].nearest = nearestIdx[i];
        nearest[i].dist = static_cast<float>(nearestDist[i]);
        nearest[i].patch = nullptr;
    }
}

//=============================================================================================================

double MNESurfaceOrVolume::solid_angle(const Eigen::Vector3f& from, const MNETriangle& tri)	/* ...to this triangle */
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

void MNESurfaceOrVolume::add_triangle_data()
/*
     * Add the triangle data structures
     */
{
    int k;
    MNETriangle* tri;

    if (type != MNE_SOURCE_SPACE_SURFACE)
        return;

    tris.clear();
    use_tris.clear();
    /*
        * Add information for the complete triangulation
        */
    if (itris.rows() > 0 && ntri > 0) {
        tris.resize(ntri);
        tot_area = 0.0;
        for (k = 0, tri = tris.data(); k < ntri; k++, tri++) {
            tri->vert = &itris(k,0);
            tri->r1   = rr.row(tri->vert[0]).transpose();
            tri->r2   = rr.row(tri->vert[1]).transpose();
            tri->r3   = rr.row(tri->vert[2]).transpose();
            tri->compute_data();
            tot_area += tri->area;
        }
#ifdef TRIANGLE_SIZE_WARNING
        for (k = 0, tri = tris.data(); k < ntri; k++, tri++)
            if (tri->area < 1e-5*tot_area/ntri)
                printf("Warning: Triangle area is only %g um^2 (%.5f %% of expected average)\n",
                       1e12*tri->area,100*ntri*tri->area/tot_area);
#endif
    }
#ifdef DEBUG
    printf("\ttotal area = %-.1f cm^2\n",1e4*tot_area);
#endif
    /*
       * Add information for the selected subset if applicable
       */
    if (use_itris.rows() > 0 && nuse_tri > 0) {
        use_tris.resize(nuse_tri);
        for (k = 0, tri = use_tris.data(); k < nuse_tri; k++, tri++) {
            tri->vert = &use_itris(k,0);
            tri->r1   = rr.row(tri->vert[0]).transpose();
            tri->r2   = rr.row(tri->vert[1]).transpose();
            tri->r3   = rr.row(tri->vert[2]).transpose();
            tri->compute_data();
        }
    }
    return;
}

//=============================================================================================================

void MNESurfaceOrVolume::compute_cm(const MNESurfaceOrVolume::PointsT& rr, int np, float (&cm)[3])
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

void MNESurfaceOrVolume::compute_surface_cm()
/*
     * Compute the center of mass of a surface
     */
{
    compute_cm(rr,np,cm);
    return;
}

//=============================================================================================================

void MNESurfaceOrVolume::calculate_vertex_distances()
{
    int   k,p,ndist;
    float diff[3];
    int   nneigh;

    if (neighbor_vert.empty() || nneighbor_vert.size() == 0)
        return;

    vert_dist.clear();
    vert_dist.resize(np);
    printf("\tDistances between neighboring vertices...");
    for (k = 0, ndist = 0; k < np; k++) {
        nneigh = nneighbor_vert[k];
        vert_dist[k] = Eigen::VectorXf(nneigh);
        const Eigen::VectorXi& neigh = neighbor_vert[k];
        for (p = 0; p < nneigh; p++) {
            if (neigh[p] >= 0) {
                VEC_DIFF_17(&rr(k,0),&rr(neigh[p],0),diff);
                vert_dist[k][p] = VEC_LEN_17(diff);
            }
            else
                vert_dist[k][p] = -1.0;
            ndist++;
        }
    }
    printf("[%d distances done]\n",ndist);
    return;
}

//=============================================================================================================

int MNESurfaceOrVolume::add_vertex_normals()
{
    int k,c,p;
    int *ii;
    float w,size;
    MNETriangle* tri;

    if (type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
       * Reallocate the stuff and initialize
       */
    nn = MNESurfaceOrVolume::NormalsT::Zero(np,3);

    for (k = 0; k < np; k++) {
        nn(k,X_17) = nn(k,Y_17) = nn(k,Z_17) = 0.0;
    }
    /*
       * One pass through the triangles will do it
       */
    add_triangle_data();
    for (p = 0, tri = tris.data(); p < ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        /*
         * Then the vertex normals
         */
        for (k = 0; k < 3; k++)
            for (c = 0; c < 3; c++)
                nn(ii[k],c) += w*tri->nn[c];
    }
    for (k = 0; k < np; k++) {
        size = VEC_LEN_17(&nn(k,0));
        if (size > 0.0)
            for (c = 0; c < 3; c++)
                nn(k,c) = nn(k,c)/size;
    }
    compute_surface_cm();
    return OK;
}

//=============================================================================================================

int MNESurfaceOrVolume::add_geometry_info(int do_normals, int check_too_many_neighbors)
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
    MNETriangle* tri;

    if (type == MNE_SOURCE_SPACE_VOLUME) {
        calculate_vertex_distances();
        return OK;
    }
    if (type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
       * Reallocate the stuff and initialize
       */
    if (do_normals) {
        nn = MNESurfaceOrVolume::NormalsT::Zero(np,3);
    }
    neighbor_tri.clear();
    neighbor_tri.resize(np);
    nneighbor_tri = Eigen::VectorXi::Zero(np);

    for (k = 0; k < np; k++) {
        if (do_normals)
            nn(k,X_17) = nn(k,Y_17) = nn(k,Z_17) = 0.0;
    }
    /*
       * One pass through the triangles will do it
       */
    add_triangle_data();
    for (p = 0, tri = tris.data(); p < ntri; p++, tri++)
        if (tri->area == 0)
            printf("\tWarning : zero size triangle # %d\n",p);
    printf("\tTriangle ");
    if (do_normals)
        printf("and vertex ");
    printf("normals and neighboring triangles...");
    for (p = 0, tri = tris.data(); p < ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        for (k = 0; k < 3; k++) {
            /*
           * Then the vertex normals
           */
            if (do_normals)
                for (c = 0; c < 3; c++)
                    nn(ii[k],c) += w*tri->nn[c];
            /*
           * Add to the list of neighbors
           */
            neighbor_tri[ii[k]].conservativeResize(nneighbor_tri[ii[k]]+1);
            neighbor_tri[ii[k]][nneighbor_tri[ii[k]]] = p;
            nneighbor_tri[ii[k]]++;
        }
    }
    nfix_no_neighbors = 0;
    nfix_defect = 0;
    for (k = 0; k < np; k++) {
        if (nneighbor_tri[k] <= 0) {
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
        else if (nneighbor_tri[k] < 3) {
#ifdef REPORT_WARNINGS
            printf("\n\tTopological defect: Vertex %d has only %d neighboring triangle%s Vertex omitted.\n\t",
                   k,nneighbor_tri[k],nneighbor_tri[k] > 1 ? "s." : ".");
#endif
            nfix_defect++;
            nneighbor_tri[k] = 0;
            neighbor_tri[k].resize(0);
        }
    }
    /*
       * Scale the vertex normals to unit length
       */
    for (k = 0; k < np; k++)
        if (nneighbor_tri[k] > 0) {
            size = VEC_LEN_17(&nn(k,0));
            if (size > 0.0)
                for (c = 0; c < 3; c++)
                    nn(k,c) = nn(k,c)/size;
        }
    printf("[done]\n");
    /*
       * Determine the neighboring vertices
       */
    printf("\tVertex neighbors...");
    neighbor_vert.clear();
    neighbor_vert.resize(np);
    nneighbor_vert = VectorXi::Zero(np);
    /*
       * We know the number of neighbors beforehand
       */
    for (k = 0; k < np; k++) {
        if (nneighbor_tri[k] > 0) {
            neighbor_vert[k]  = VectorXi(nneighbor_tri[k]);
            nneighbor_vert[k] = nneighbor_tri[k];
        }
        else {
            nneighbor_vert[k] = 0;
        }
    }
    nfix_distinct = 0;
    for (k = 0; k < np; k++) {
        Eigen::VectorXi& neighbors = neighbor_vert[k];
        nneighbors = 0;
        for (p = 0; p < nneighbor_tri[k]; p++) {
            /*
           * Fit in the other vertices of the neighboring triangle
           */
            for (c = 0; c < 3; c++) {
                vert = tris[neighbor_tri[k][p]].vert[c];
                if (vert != k) {
                    for (q = 0, found = FALSE; q < nneighbors; q++) {
                        if (neighbors[q] == vert) {
                            found = TRUE;
                            break;
                        }
                    }
                    if (!found) {
                        if (nneighbors < nneighbor_vert[k])
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
        if (nneighbors != nneighbor_vert[k]) {
#ifdef REPORT_WARNINGS
            printf("\n\tIncorrect number of distinct neighbors for vertex %d (%d instead of %d) [fixed].",
                   k,nneighbors,nneighbor_vert[k]);
#endif
            nfix_distinct++;
            nneighbor_vert[k] = nneighbors;
        }
    }
    printf("[done]\n");
    /*
       * Distance calculation follows
       */
    calculate_vertex_distances();
    compute_surface_cm();
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
    for (k = 0; k < np; k++) {
        if (nneighbor_vert[k] <= 0)
            printf("No neighbors for vertex %d\n",k);
        if (nneighbor_tri[k] <= 0)
            printf("No neighbor tris for vertex %d\n",k);
    }
#endif
    return OK;
}

//=============================================================================================================

int MNESurfaceOrVolume::add_geometry_info(int do_normals)
{
    return add_geometry_info(do_normals,TRUE);
}

//=============================================================================================================

int MNESurfaceOrVolume::add_geometry_info2(int do_normals)

{
    return add_geometry_info(do_normals,FALSE);
}

//=============================================================================================================
