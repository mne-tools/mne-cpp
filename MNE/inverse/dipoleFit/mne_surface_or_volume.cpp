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

#include <fiff/fiff_stream.h>


#include <QFile>
#include <QCoreApplication>

#define _USE_MATH_DEFINES
#include <math.h>


#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;


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



#define NNEIGHBORS 26



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





//============================= mne_coord_transforms.c =============================

typedef struct {
    int frame;
    const char *name;
} frameNameRec_17;


const char *mne_coord_frame_name_17(int frame)

{
    static frameNameRec_17 frames[] = {
        {FIFFV_COORD_UNKNOWN,"unknown"},
        {FIFFV_COORD_DEVICE,"MEG device"},
        {FIFFV_COORD_ISOTRAK,"isotrak"},
        {FIFFV_COORD_HPI,"hpi"},
        {FIFFV_COORD_HEAD,"head"},
        {FIFFV_COORD_MRI,"MRI (surface RAS)"},
        {FIFFV_MNE_COORD_MRI_VOXEL, "MRI voxel"},
        {FIFFV_COORD_MRI_SLICE,"MRI slice"},
        {FIFFV_COORD_MRI_DISPLAY,"MRI display"},
        {FIFFV_MNE_COORD_CTF_DEVICE,"CTF MEG device"},
        {FIFFV_MNE_COORD_CTF_HEAD,"CTF/4D/KIT head"},
        {FIFFV_MNE_COORD_RAS,"RAS (non-zero origin)"},
        {FIFFV_MNE_COORD_MNI_TAL,"MNI Talairach"},
        {FIFFV_MNE_COORD_FS_TAL_GTZ,"Talairach (MNI z > 0)"},
        {FIFFV_MNE_COORD_FS_TAL_LTZ,"Talairach (MNI z < 0)"},
        {-1,"unknown"}
    };
    int k;
    for (k = 0; frames[k].frame != -1; k++) {
        if (frame == frames[k].frame)
            return frames[k].name;
    }
    return frames[k].name;
}





//============================= fiff_trans.c =============================





void fiff_coord_trans_inv (float r[3],FiffCoordTransOld* t,int do_move)
/*
      * Apply inverse coordinate transformation
      */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->invmove[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->invrot[j][k]*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}










//============================= mne_add_geometry_info.c =============================


static void add_triangle_data(mneTriangle tri)
/*
* Normal vector of a triangle and other stuff
*/
{
    float size,sizey;
    int   c;
    VEC_DIFF_17 (tri->r1,tri->r2,tri->r12);
    VEC_DIFF_17 (tri->r1,tri->r3,tri->r13);
    CROSS_PRODUCT_17 (tri->r12,tri->r13,tri->nn);
    size = VEC_LEN_17(tri->nn);
    /*
    * Possibly zero area triangles
    */
    if (size > 0) {
        tri->nn[X_17] = tri->nn[X_17]/size;
        tri->nn[Y_17] = tri->nn[Y_17]/size;
        tri->nn[Z_17] = tri->nn[Z_17]/size;
    }
    tri->area = size/2.0;
    sizey = VEC_LEN_17(tri->r13);
    if (sizey <= 0)
        sizey = 1.0;

    for (c = 0; c < 3; c++) {
        tri->ey[c] = tri->r13[c]/sizey;
        tri->cent[c] = (tri->r1[c]+tri->r2[c]+tri->r3[c])/3.0;
    }
    CROSS_PRODUCT_17(tri->ey,tri->nn,tri->ex);

    return;
}

void mne_add_triangle_data(MneSurfaceOrVolume::MneCSourceSpace* s)
/*
* Add the triangle data structures
*/
{
    int k;
    mneTriangle tri;

    if (!s || s->type != MNE_SOURCE_SPACE_SURFACE)
        return;

    FREE_17(s->tris);     s->tris = NULL;
    FREE_17(s->use_tris); s->use_tris = NULL;
    /*
    * Add information for the complete triangulation
    */
    if (s->itris && s->ntri > 0) {
        s->tris = MALLOC_17(s->ntri,mneTriangleRec);
        s->tot_area = 0.0;
        for (k = 0, tri = s->tris; k < s->ntri; k++, tri++) {
            tri->vert = s->itris[k];
            tri->r1   = s->rr[tri->vert[0]];
            tri->r2   = s->rr[tri->vert[1]];
            tri->r3   = s->rr[tri->vert[2]];
            add_triangle_data(tri);
            s->tot_area += tri->area;
        }
#ifdef TRIANGLE_SIZE_WARNING
        for (k = 0, tri = s->tris; k < s->ntri; k++, tri++)
            if (tri->area < 1e-5*s->tot_area/s->ntri)
                fprintf(stderr,"Warning: Triangle area is only %g um^2 (%.5f %% of expected average)\n",
                        1e12*tri->area,100*s->ntri*tri->area/s->tot_area);
#endif
    }
#ifdef DEBUG
    fprintf(stderr,"\ttotal area = %-.1f cm^2\n",1e4*s->tot_area);
#endif
    /*
   * Add information for the selected subset if applicable
   */
    if (s->use_itris && s->nuse_tri > 0) {
        s->use_tris = MALLOC_17(s->nuse_tri,mneTriangleRec);
        for (k = 0, tri = s->use_tris; k < s->nuse_tri; k++, tri++) {
            tri->vert = s->use_itris[k];
            tri->r1   = s->rr[tri->vert[0]];
            tri->r2   = s->rr[tri->vert[1]];
            tri->r3   = s->rr[tri->vert[2]];
            add_triangle_data(tri);
        }
    }
    return;
}


void mne_compute_cm(float **rr, int np, float *cm)
/*
 * Compute the center of mass of a set of points
 */
{
    int q;
    cm[0] = cm[1] = cm[2] = 0.0;
    for (q = 0; q < np; q++) {
        cm[0] += rr[q][0];
        cm[1] += rr[q][1];
        cm[2] += rr[q][2];
    }
    if (np > 0) {
        cm[0] = cm[0]/np;
        cm[1] = cm[1]/np;
        cm[2] = cm[2]/np;
    }
    return;
}


void mne_compute_surface_cm(MneSurfaceOrVolume::MneCSurface* s)
/*
 * Compute the center of mass of a surface
 */
{
    if (!s)
        return;

    mne_compute_cm(s->rr,s->np,s->cm);
    return;
}

static void calculate_vertex_distances(MneSurfaceOrVolume::MneCSourceSpace* s)

{
    int   k,p,ndist;
    float *dist,diff[3];
    int   *neigh, nneigh;

    if (!s->neighbor_vert || !s->nneighbor_vert)
        return;

    if (s->vert_dist) {
        for (k = 0; k < s->np; k++)
            FREE_17(s->vert_dist[k]);
        FREE_17(s->vert_dist);
    }
    s->vert_dist = MALLOC_17(s->np,float *);
    fprintf(stderr,"\tDistances between neighboring vertices...");
    for (k = 0, ndist = 0; k < s->np; k++) {
        s->vert_dist[k]  = dist = MALLOC_17(s->nneighbor_vert[k],float);
        neigh  = s->neighbor_vert[k];
        nneigh = s->nneighbor_vert[k];
        for (p = 0; p < nneigh; p++) {
            if (neigh[p] >= 0) {
                VEC_DIFF_17(s->rr[k],s->rr[neigh[p]],diff);
                dist[p] = VEC_LEN_17(diff);
            }
            else
                dist[p] = -1.0;
            ndist++;
        }
    }
    fprintf(stderr,"[%d distances done]\n",ndist);
    return;
}


int mne_add_vertex_normals(MneSurfaceOrVolume::MneCSourceSpace* s)


{
    int k,c,p;
    int *ii;
    float w,size;
    mneTriangle tri;

    if (!s || s->type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
   * Reallocate the stuff and initialize
   */
    FREE_CMATRIX_17(s->nn);
    s->nn = ALLOC_CMATRIX_17(s->np,3);

    for (k = 0; k < s->np; k++) {
        s->nn[k][X_17] = s->nn[k][Y_17] = s->nn[k][Z_17] = 0.0;
    }
    /*
   * One pass through the triangles will do it
   */
    mne_add_triangle_data(s);
    for (p = 0, tri = s->tris; p < s->ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        /*
     * Then the vertex normals
     */
        for (k = 0; k < 3; k++)
            for (c = 0; c < 3; c++)
                s->nn[ii[k]][c] += w*tri->nn[c];
    }
    for (k = 0; k < s->np; k++) {
        size = VEC_LEN_17(s->nn[k]);
        if (size > 0.0)
            for (c = 0; c < 3; c++)
                s->nn[k][c] = s->nn[k][c]/size;
    }
    mne_compute_surface_cm(s);
    return OK;
}





static int add_geometry_info(MneSurfaceOrVolume::MneCSourceSpace* s, int do_normals, int *border, int check_too_many_neighbors)
/*
      * Add vertex normals and neighbourhood information
      */
{
    int k,c,p,q;
    int vert;
    int *ii;
    int *neighbors,nneighbors;
    float w,size;
    int   found;
    int   nfix_distinct,nfix_no_neighbors,nfix_defect;
    mneTriangle tri;

    if (!s)
        return OK;

    if (s->type == MNE_SOURCE_SPACE_VOLUME) {
        calculate_vertex_distances(s);
        return OK;
    }
    if (s->type != MNE_SOURCE_SPACE_SURFACE)
        return OK;
    /*
   * Reallocate the stuff and initialize
   */
    if (do_normals) {
        FREE_CMATRIX_17(s->nn);
        s->nn = ALLOC_CMATRIX_17(s->np,3);
    }
    if (s->neighbor_tri) {
        for (k = 0; k < s->np; k++)
            FREE_17(s->neighbor_tri[k]);
        FREE_17(s->neighbor_tri);
    }
    FREE_17(s->nneighbor_tri);
    s->neighbor_tri = MALLOC_17(s->np,int *);
    s->nneighbor_tri = MALLOC_17(s->np,int);

    for (k = 0; k < s->np; k++) {
        s->neighbor_tri[k] = NULL;
        s->nneighbor_tri[k] = 0;
        if (do_normals)
            s->nn[k][X_17] = s->nn[k][Y_17] = s->nn[k][Z_17] = 0.0;
    }
    /*
   * One pass through the triangles will do it
   */
    mne_add_triangle_data(s);
    for (p = 0, tri = s->tris; p < s->ntri; p++, tri++)
        if (tri->area == 0)
            fprintf(stderr,"\tWarning : zero size triangle # %d\n",p);
    fprintf(stderr,"\tTriangle ");
    if (do_normals)
        fprintf(stderr,"and vertex ");
    fprintf(stderr,"normals and neighboring triangles...");
    for (p = 0, tri = s->tris; p < s->ntri; p++, tri++) {
        ii = tri->vert;
        w = 1.0;			/* This should be related to the triangle size */
        for (k = 0; k < 3; k++) {
            /*
       * Then the vertex normals
       */
            if (do_normals)
                for (c = 0; c < 3; c++)
                    s->nn[ii[k]][c] += w*tri->nn[c];
            /*
       * Add to the list of neighbors
       */
            s->neighbor_tri[ii[k]] = REALLOC_17(s->neighbor_tri[ii[k]],
                    s->nneighbor_tri[ii[k]]+1,int);
            s->neighbor_tri[ii[k]][s->nneighbor_tri[ii[k]]] = p;
            s->nneighbor_tri[ii[k]]++;
        }
    }
    nfix_no_neighbors = 0;
    nfix_defect = 0;
    for (k = 0; k < s->np; k++) {
        if (s->nneighbor_tri[k] <= 0) {
            if (!border || !border[k]) {
#ifdef STRICT_ERROR
                err_printf_set_error("Vertex %d does not have any neighboring triangles!",k);
                return FAIL;
#else
#ifdef REPORT_WARNINGS
                fprintf(stderr,"Warning: Vertex %d does not have any neighboring triangles!\n",k);
#endif
#endif
                nfix_no_neighbors++;
            }
        }
        else if (s->nneighbor_tri[k] < 3 && !border) {
#ifdef REPORT_WARNINGS
            fprintf(stderr,"\n\tTopological defect: Vertex %d has only %d neighboring triangle%s Vertex omitted.\n\t",
                    k,s->nneighbor_tri[k],s->nneighbor_tri[k] > 1 ? "s." : ".");
#endif
            nfix_defect++;
            s->nneighbor_tri[k] = 0;
            FREE_17(s->neighbor_tri[k]);
            s->neighbor_tri[k] = NULL;
        }
    }
    /*
   * Scale the vertex normals to unit length
   */
    for (k = 0; k < s->np; k++)
        if (s->nneighbor_tri[k] > 0) {
            size = VEC_LEN_17(s->nn[k]);
            if (size > 0.0)
                for (c = 0; c < 3; c++)
                    s->nn[k][c] = s->nn[k][c]/size;
        }
    fprintf(stderr,"[done]\n");
    /*
   * Determine the neighboring vertices
   */
    fprintf(stderr,"\tVertex neighbors...");
    if (s->neighbor_vert) {
        for (k = 0; k < s->np; k++)
            FREE_17(s->neighbor_vert[k]);
        FREE_17(s->neighbor_vert);
    }
    FREE_17(s->nneighbor_vert);
    s->neighbor_vert = MALLOC_17(s->np,int *);
    s->nneighbor_vert = MALLOC_17(s->np,int);
    /*
   * We know the number of neighbors beforehand
   */
    if (border) {
        for (k = 0; k < s->np; k++) {
            if (s->nneighbor_tri[k] > 0) {
                if (border[k]) {
                    s->neighbor_vert[k]  = MALLOC_17(s->nneighbor_tri[k]+1,int);
                    s->nneighbor_vert[k] = s->nneighbor_tri[k]+1;
                }
                else {
                    s->neighbor_vert[k]  = MALLOC_17(s->nneighbor_tri[k],int);
                    s->nneighbor_vert[k] = s->nneighbor_tri[k];
                }
            }
            else {
                s->neighbor_vert[k]  = NULL;
                s->nneighbor_vert[k] = 0;
            }
        }
    }
    else {
        for (k = 0; k < s->np; k++) {
            if (s->nneighbor_tri[k] > 0) {
                s->neighbor_vert[k]  = MALLOC_17(s->nneighbor_tri[k],int);
                s->nneighbor_vert[k] = s->nneighbor_tri[k];
            }
            else {
                s->neighbor_vert[k]  = NULL;
                s->nneighbor_vert[k] = 0;
            }
        }
    }
    nfix_distinct = 0;
    for (k = 0; k < s->np; k++) {
        neighbors  = s->neighbor_vert[k];
        nneighbors = 0;
        for (p = 0; p < s->nneighbor_tri[k]; p++) {
            /*
       * Fit in the other vertices of the neighboring triangle
       */
            for (c = 0; c < 3; c++) {
                vert = s->tris[s->neighbor_tri[k][p]].vert[c];
                if (vert != k) {
                    for (q = 0, found = FALSE; q < nneighbors; q++) {
                        if (neighbors[q] == vert) {
                            found = TRUE;
                            break;
                        }
                    }
                    if (!found) {
                        if (nneighbors < s->nneighbor_vert[k])
                            neighbors[nneighbors++] = vert;
                        else if (!border || !border[k]) {
                            if (check_too_many_neighbors) {
                                printf("Too many neighbors for vertex %d.",k);
                                return FAIL;
                            }
                            else
                                fprintf(stderr,"\tWarning: Too many neighbors for vertex %d\n",k);
                        }
                    }
                }
            }
        }
        if (nneighbors != s->nneighbor_vert[k]) {
#ifdef REPORT_WARNINGS
            fprintf(stderr,"\n\tIncorrect number of distinct neighbors for vertex %d (%d instead of %d) [fixed].",
                    k,nneighbors,s->nneighbor_vert[k]);
#endif
            nfix_distinct++;
            s->nneighbor_vert[k] = nneighbors;
        }
    }
    fprintf(stderr,"[done]\n");
    /*
   * Distance calculation follows
   */
    calculate_vertex_distances(s);
    mne_compute_surface_cm(s);
    /*
   * Summarize the defects
   */
    if (nfix_defect > 0)
        fprintf(stderr,"\tWarning: %d topological defects were fixed.\n",nfix_defect);
    if (nfix_distinct > 0)
        fprintf(stderr,"\tWarning: %d vertices had incorrect number of distinct neighbors (fixed).\n",nfix_distinct);
    if (nfix_no_neighbors > 0)
        fprintf(stderr,"\tWarning: %d vertices did not have any neighboring triangles (fixed)\n",nfix_no_neighbors);
#ifdef DEBUG
    for (k = 0; k < s->np; k++) {
        if (s->nneighbor_vert[k] <= 0)
            fprintf(stderr,"No neighbors for vertex %d\n",k);
        if (s->nneighbor_tri[k] <= 0)
            fprintf(stderr,"No neighbor tris for vertex %d\n",k);
    }
#endif
    return OK;
}

int mne_source_space_add_geometry_info(MneSurfaceOrVolume::MneCSourceSpace* s, int do_normals)
{
    return add_geometry_info(s,do_normals,NULL,TRUE);
}

int mne_source_space_add_geometry_info2(MneSurfaceOrVolume::MneCSourceSpace* s, int do_normals)

{
    return add_geometry_info(s,do_normals,NULL,FALSE);
}






//============================= mne_matop.c =============================

float **mne_lu_invert_17(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_17(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix_17(eigen_mat_inv, mat);
    return mat;
}


//============================= make_volume_source_space.c =============================

static int add_inverse_17(FiffCoordTransOld* t)
/*
      * Add inverse transform to an existing one
      */
{
    int   j,k;
    float **m = ALLOC_CMATRIX_17(4,4);

    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            m[j][k] = t->rot[j][k];
        m[j][3] = t->move[j];
    }
    for (k = 0; k < 3; k++)
        m[3][k] = 0.0;
    m[3][3] = 1.0;
    if (mne_lu_invert_17(m,4) == NULL) {
        FREE_CMATRIX_17(m);
        return FAIL;
    }
    for (j = 0; j < 3; j++) {
        for (k = 0; k < 3; k++)
            t->invrot[j][k] = m[j][k];
        t->invmove[j] = m[j][3];
    }
    FREE_CMATRIX_17(m);
    return OK;
}





//============================= make_volume_source_space.c =============================

static FiffCoordTransOld* fiff_make_transform2 (int from,int to,float rot[3][3],float move[3])
/*
      * Compose the coordinate transformation structure
      * from a known forward transform
      */
{
    FiffCoordTransOld* t = new FiffCoordTransOld;
    int j,k;

    t->from = from;
    t->to   = to;

    for (j = 0; j < 3; j++) {
        t->move[j] = move[j];
        for (k = 0; k < 3; k++)
            t->rot[j][k] = rot[j][k];
    }

    if (add_inverse_17(t) == FAIL) {
        printf("Failed to add the inverse coordinate transformation");
        FREE_17(t);
        return NULL;
    }

    return (t);
}

static FiffCoordTransOld* make_voxel_ras_trans(float *r0,
                                           float *x_ras,
                                           float *y_ras,
                                           float *z_ras,
                                           float *voxel_size)

{
    FiffCoordTransOld* t;
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

    t = fiff_make_transform2(FIFFV_MNE_COORD_MRI_VOXEL,FIFFV_COORD_MRI,rot,move);

    return t;
}









//============================= fwd_bem_model.c =============================

static struct {
    int  kind;
    const char *name;
} surf_expl_17[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };


const char *fwd_bem_explain_surface_17(int kind)

{
    int k;

    for (k = 0; surf_expl_17[k].kind >= 0; k++)
        if (surf_expl_17[k].kind == kind)
            return surf_expl_17[k].name;

    return surf_expl_17[k].name;
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

double MneSurfaceOrVolume::solid_angle(float *from, mneTriangle tri)	/* ...to this triangle */
/*
     * Compute the solid angle according to van Oosterom's
     * formula
     */
{
    double v1[3],v2[3],v3[3];
    double l1,l2,l3,s,triple;
    double cross[3];

    VEC_DIFF_17 (from,tri->r1,v1);
    VEC_DIFF_17 (from,tri->r2,v2);
    VEC_DIFF_17 (from,tri->r3,v3);

    CROSS_PRODUCT_17(v1,v2,cross);
    triple = VEC_DOT_17(cross,v3);

    l1 = VEC_LEN_17(v1);
    l2 = VEC_LEN_17(v2);
    l3 = VEC_LEN_17(v3);
    s = (l1*l2*l3+VEC_DOT_17(v1,v2)*l3+VEC_DOT_17(v1,v3)*l2+VEC_DOT_17(v2,v3)*l1);

    return (2.0*atan2(triple,s));
}


//*************************************************************************************************************

double MneSurfaceOrVolume::sum_solids(float *from, MneSurfaceOrVolume::MneCSurface *surf)
{
    int k;
    double tot_angle, angle;
    for (k = 0, tot_angle = 0.0; k < surf->ntri; k++) {
        angle = solid_angle(from,surf->tris+k);
        tot_angle += angle;
    }
    return tot_angle;
}


//*************************************************************************************************************

int MneSurfaceOrVolume::mne_filter_source_spaces(MneSurfaceOrVolume::MneCSurface *surf, float limit, FiffCoordTransOld* mri_head_t, MneSurfaceOrVolume::MneCSourceSpace **spaces, int nspace, FILE *filtered)	          /* Provide a list of filtered points here */
/*
    * Remove all source space points closer to the surface than a given limit
    */
{
    MneSurfaceOrVolume::MneCSourceSpace* s;
    int k,p1,p2;
    float r1[3];
    float mindist,dist,diff[3];
    int   minnode;
    int   omit,omit_outside;
    double tot_angle;

    if (surf == NULL)
        return OK;
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD && mri_head_t == NULL) {
        printf("Source spaces are in head coordinates and no coordinate transform was provided!");
        return FAIL;
    }
    /*
        * How close are the source points to the surface?
        */
    fprintf(stderr,"Source spaces are in ");
    if (spaces[0]->coord_frame == FIFFV_COORD_HEAD)
        fprintf(stderr,"head coordinates.\n");
    else if (spaces[0]->coord_frame == FIFFV_COORD_MRI)
        fprintf(stderr,"MRI coordinates.\n");
    else
        fprintf(stderr,"unknown (%d) coordinates.\n",spaces[0]->coord_frame);
    fprintf(stderr,"Checking that the sources are inside the bounding surface ");
    if (limit > 0.0)
        fprintf(stderr,"and at least %6.1f mm away",1000*limit);
    fprintf(stderr," (will take a few...)\n");
    omit         = 0;
    omit_outside = 0;
    for (k = 0; k < nspace; k++) {
        s = spaces[k];
        for (p1 = 0; p1 < s->np; p1++)
            if (s->inuse[p1]) {
                VEC_COPY_17(r1,s->rr[p1]);	/* Transform the point to MRI coordinates */
                if (s->coord_frame == FIFFV_COORD_HEAD)
                    fiff_coord_trans_inv(r1,mri_head_t,FIFFV_MOVE);
                /*
                    * Check that the source is inside the inner skull surface
                    */
                tot_angle = sum_solids(r1,surf)/(4*M_PI);
                if (fabs(tot_angle-1.0) > 1e-5) {
                    omit_outside++;
                    s->inuse[p1] = FALSE;
                    s->nuse--;
                    if (filtered)
                        fprintf(filtered,"%10.3f %10.3f %10.3f\n",
                                1000*r1[X_17],1000*r1[Y_17],1000*r1[Z_17]);
                }
                else if (limit > 0.0) {
                    /*
                        * Check the distance limit
                        */
                    mindist = 1.0;
                    minnode = 0;
                    for (p2 = 0; p2 < surf->np; p2++) {
                        VEC_DIFF_17(r1,surf->rr[p2],diff);
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
                            fprintf(filtered,"%10.3f %10.3f %10.3f\n",
                                    1000*r1[X_17],1000*r1[Y_17],1000*r1[Z_17]);
                    }
                }
            }
    }
    if (omit_outside > 0)
        fprintf(stderr,"%d source space points omitted because they are outside the inner skull surface.\n",
                omit_outside);
    if (omit > 0)
        fprintf(stderr,"%d source space points omitted because of the %6.1f-mm distance limit.\n",
                omit,1000*limit);
    fprintf(stderr,"Thank you for waiting.\n");
    return OK;
}


//*************************************************************************************************************

MneSurfaceOrVolume::MneCSourceSpace *MneSurfaceOrVolume::make_volume_source_space(MneSurfaceOrVolume::MneCSurface *surf, float grid, float exclude, float mindist)
/*
    * Make a source space which covers the volume bounded by surf
    */
{
    float min[3],max[3],cm[3];
    int   minn[3],maxn[3];
    float *node,maxdist,dist,diff[3];
    int   k,c;
    MneSurfaceOrVolume::MneCSourceSpace* sp = NULL;
    int np,nplane,nrow;
    int *neigh,nneigh;
    int x,y,z;
    /*
        * Figure out the grid size
        */
    cm[X_17] = cm[Y_17] = cm[Z_17] = 0.0;
    node = surf->rr[0];
    for (c = 0; c < 3; c++)
        min[c] = max[c] = node[c];

    for (k = 0; k < surf->np; k++) {
        node = surf->rr[k];
        for (c = 0; c < 3; c++) {
            cm[c] += node[c];
            if (node[c] < min[c])
                min[c] = node[c];
            if (node[c] > max[c])
                max[c] = node[c];
        }
    }
    for (c = 0; c < 3; c++)
        cm[c] = cm[c]/surf->np;
    /*
       * Define the sphere which fits the surface
       */
    maxdist = 0.0;
    for (k = 0; k < surf->np; k++) {
        VEC_DIFF_17(cm,surf->rr[k],diff);
        dist = VEC_LEN_17(diff);
        if (dist > maxdist)
            maxdist = dist;
    }
    fprintf(stderr,"Surface CM = (%6.1f %6.1f %6.1f) mm\n",
            1000*cm[X_17], 1000*cm[Y_17], 1000*cm[Z_17]);
    fprintf(stderr,"Surface fits inside a sphere with radius %6.1f mm\n",1000*maxdist);
    fprintf(stderr,"Surface extent:\n"
                   "\tx = %6.1f ... %6.1f mm\n"
                   "\ty = %6.1f ... %6.1f mm\n"
                   "\tz = %6.1f ... %6.1f mm\n",
            1000*min[X_17],1000*max[X_17],
            1000*min[Y_17],1000*max[Y_17],
            1000*min[Z_17],1000*max[Z_17]);
    for (c = 0; c < 3; c++) {
        if (max[c] > 0)
            maxn[c] = floor(fabs(max[c])/grid)+1;
        else
            maxn[c] = -floor(fabs(max[c])/grid)-1;
        if (min[c] > 0)
            minn[c] = floor(fabs(min[c])/grid)+1;
        else
            minn[c] = -floor(fabs(min[c])/grid)-1;
    }
    fprintf(stderr,"Grid extent:\n"
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
    sp = MneSurfaceOrVolume::mne_new_source_space(np);
    sp->type = MNE_SOURCE_SPACE_VOLUME;
    sp->nneighbor_vert = MALLOC_17(sp->np,int);
    sp->neighbor_vert = MALLOC_17(sp->np,int *);
    for (k = 0; k < sp->np; k++) {
        sp->inuse[k]  = TRUE;
        sp->vertno[k] = k;
        sp->nn[k][X_17] = sp->nn[k][Y_17] = 0.0; /* Source orientation is immaterial */
        sp->nn[k][Z_17] = 1.0;
        sp->neighbor_vert[k]  = neigh  = MALLOC_17(NNEIGHBORS,int);
        sp->nneighbor_vert[k] = nneigh = NNEIGHBORS;
        for (c = 0; c < nneigh; c++)
            neigh[c] = -1;
        sp->nuse++;
    }
    for (k = 0, z = minn[Z_17]; z <= maxn[Z_17]; z++) {
        for (y = minn[Y_17]; y <= maxn[Y_17]; y++) {
            for (x = minn[X_17]; x <= maxn[X_17]; x++, k++) {
                sp->rr[k][X_17] = x*grid;
                sp->rr[k][Y_17] = y*grid;
                sp->rr[k][Z_17] = z*grid;
                /*
             * Figure out the neighborhood:
             * 6-neighborhood first
             */
                neigh = sp->neighbor_vert[k];
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
    fprintf(stderr,"%d sources before omitting any.\n",sp->nuse);
    /*
       * Exclude infeasible points
       */
    for (k = 0; k < sp->np; k++) {
        VEC_DIFF_17(cm,sp->rr[k],diff);
        dist = VEC_LEN_17(diff);
        if (dist < exclude || dist > maxdist) {
            sp->inuse[k] = FALSE;
            sp->nuse--;
        }
    }
    fprintf(stderr,"%d sources after omitting infeasible sources.\n",sp->nuse);
    if (mne_filter_source_spaces(surf,mindist,NULL,&sp,1,NULL) != OK)
        goto bad;
    fprintf(stderr,"%d sources remaining after excluding the sources outside the surface and less than %6.1f mm inside.\n",sp->nuse,1000*mindist);
    /*
       * Omit unused vertices from the neighborhoods
       */
    fprintf(stderr,"Adjusting the neighborhood info...");
    for (k = 0; k < sp->np; k++) {
        neigh  = sp->neighbor_vert[k];
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
    fprintf(stderr,"[done]\n");
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

        if ((sp->voxel_surf_RAS_t = make_voxel_ras_trans(r0,x_ras,y_ras,z_ras,voxel_size)) == NULL)
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


//*************************************************************************************************************

MneSurfaceOrVolume::MneCSurface *MneSurfaceOrVolume::make_guesses(MneSurfaceOrVolume::MneCSurface *guess_surf, float guessrad, float *guess_r0, float grid, float exclude, float mindist)		   /* Exclude points closer than this to
                                                        * the guess boundary surface */
/*
     * Make a guess space inside a sphere
     */
{
    char *bemname     = NULL;
    MneSurfaceOrVolume::MneCSurface* sphere = NULL;
    MneSurfaceOrVolume::MneCSurface* res    = NULL;
    int        k;
    float      dist;
    float      r0[] = { 0.0, 0.0, 0.0 };

    if (!guess_r0)
        guess_r0 = r0;

    if (!guess_surf) {
        fprintf(stderr,"Making a spherical guess space with radius %7.1f mm...\n",1000*guessrad);
        //#ifdef USE_SHARE_PATH
        //    if ((bemname = mne_compose_mne_name("share/mne","icos.fif")) == NULL)
        //#else
        //    if ((bemname = mne_compose_mne_name("setup/mne","icos.fif")) == NULL)
        //#endif
        //      goto out;

        //    QFile bemFile("/usr/pubsw/packages/mne/stable/share/mne/icos.fif");

        QFile bemFile(QString("./resources/surf2bem/icos.fif"));
        if ( !QCoreApplication::startingUp() )
            bemFile.setFileName(QCoreApplication::applicationDirPath() + QString("/resources/surf2bem/icos.fif"));
        else if (!bemFile.exists())
            bemFile.setFileName("./bin/resources/surf2bem/icos.fif");

        if( !bemFile.exists () ){
            qDebug() << bemFile.fileName() << "does not exists.";
            goto out;
        }

        bemname = MALLOC_17(strlen(bemFile.fileName().toLatin1().data())+1,char);
        strcpy(bemname,bemFile.fileName().toLatin1().data());

        if ((sphere = MneSurfaceOrVolume::MneCSourceSpace::read_bem_surface(bemname,9003,FALSE,NULL)) == NULL)
            goto out;

        for (k = 0; k < sphere->np; k++) {
            dist = VEC_LEN_17(sphere->rr[k]);
            sphere->rr[k][X_17] = guessrad*sphere->rr[k][X_17]/dist + guess_r0[X_17];
            sphere->rr[k][Y_17] = guessrad*sphere->rr[k][Y_17]/dist + guess_r0[Y_17];
            sphere->rr[k][Z_17] = guessrad*sphere->rr[k][Z_17]/dist + guess_r0[Z_17];
        }
        if (mne_source_space_add_geometry_info(sphere,TRUE) == FAIL)
            goto out;
        guess_surf = sphere;
    }
    else {
        fprintf(stderr,"Guess surface (%d = %s) is in %s coordinates\n",
                guess_surf->id,fwd_bem_explain_surface_17(guess_surf->id),
                mne_coord_frame_name_17(guess_surf->coord_frame));
    }
    fprintf(stderr,"Filtering (grid = %6.f mm)...\n",1000*grid);
    res = make_volume_source_space(guess_surf,grid,exclude,mindist);

out : {
        FREE_17(bemname);
        if(sphere)
            delete sphere;
        return res;
    }
}


//*************************************************************************************************************

MneSurfaceOrVolume::MneCSurface *MneSurfaceOrVolume::read_bem_surface(const QString &name, int which, int add_geometry, float *sigmap)          /* Conductivity? */
{
    return read_bem_surface(name,which,add_geometry,sigmap,true);
}


//*************************************************************************************************************

MneSurfaceOrVolume::MneCSurface *MneSurfaceOrVolume::read_bem_surface(const QString &name, int which, int add_geometry, float *sigmap, bool check_too_many_neighbors)
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
    float   **nodes        = NULL;
    float   **node_normals = NULL;
    int     **triangles    = NULL;
    int     nnode,ntri;
    MneSurfaceOrVolume::MneCSurface* s = NULL;
    int k;
    int coord_frame = FIFFV_COORD_MRI;
    float sigma = -1.0;
    MatrixXf tmp_nodes;
    MatrixXi tmp_triangles;

    if(!stream->open())
        goto bad;
    /*
        * Check for the existence of BEM coord frame
        */
    bems = stream->tree()->dir_tree_find(FIFFB_BEM);
    if (bems.size() > 0) {
        node = bems[0];
        if (node->find_tag(stream, FIFF_BEM_COORD_FRAME, t_pTag)) {
            coord_frame = *t_pTag->toInt();
        }
    }
    surfs = stream->tree()->dir_tree_find(FIFFB_BEM_SURF);
    if (surfs.size() == 0) {
        printf ("No BEM surfaces found in %s",name.toLatin1().constData());
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
            printf("Desired surface not found in %s",name.toLatin1().constData());
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
    nodes = ALLOC_CMATRIX_17(tmp_nodes.rows(),tmp_nodes.cols());
    fromFloatEigenMatrix_17(tmp_nodes, nodes);

    if (node->find_tag(stream, FIFF_BEM_SURF_NORMALS, t_pTag)) {\
        MatrixXf tmp_node_normals = t_pTag->toFloatMatrix().transpose();
        node_normals = ALLOC_CMATRIX_17(tmp_node_normals.rows(),tmp_node_normals.cols());
        fromFloatEigenMatrix_17(tmp_node_normals, node_normals);
    }

    if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag))
        goto bad;
    tmp_triangles = t_pTag->toIntMatrix().transpose();
    triangles = (int **)malloc(tmp_triangles.rows() * sizeof(int *));
    for (int i = 0; i < tmp_triangles.rows(); ++i)
        triangles[i] = (int *)malloc(tmp_triangles.cols() * sizeof(int));
    fromIntEigenMatrix_17(tmp_triangles, triangles);

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

    s = mne_new_source_space(0);
    for (k = 0; k < ntri; k++) {
        triangles[k][0]--;
        triangles[k][1]--;
        triangles[k][2]--;
    }
    s->itris       = triangles;
    s->id          = which;
    s->coord_frame = coord_frame;
    s->rr          = nodes;      nodes = NULL;
    s->nn          = node_normals; node_normals = NULL;
    s->ntri        = ntri;
    s->np          = nnode;
    s->curv        = NULL;
    s->val         = NULL;

    if (add_geometry) {
        if (check_too_many_neighbors) {
            if (mne_source_space_add_geometry_info(s,!s->nn) != OK)
                goto bad;
        }
        else {
            if (mne_source_space_add_geometry_info2(s,!s->nn) != OK)
                goto bad;
        }
    }
    else if (s->nn == NULL) {       /* Normals only */
        if (mne_add_vertex_normals(s) != OK)
            goto bad;
    }
    else
        mne_add_triangle_data(s);

    s->nuse   = s->np;
    s->inuse  = MALLOC_17(s->np,int);
    s->vertno = MALLOC_17(s->np,int);
    for (k = 0; k < s->np; k++) {
        s->inuse[k]  = TRUE;
        s->vertno[k] = k;
    }
    if (sigmap)
        *sigmap = sigma;

    return s;

bad : {
        FREE_CMATRIX_17(nodes);
        FREE_CMATRIX_17(node_normals);
        FREE_ICMATRIX_17(triangles);
        stream->close();
        return NULL;
    }
}
