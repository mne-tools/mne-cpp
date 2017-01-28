//=============================================================================================================
/**
* @file     guess_data.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the GuessData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "guess_data.h"
#include "dipole_fit_data.h"

#include "dipole_forward.h"

#include "mne_surface_or_volume.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;



//============================= mne_fiff.h =============================

#define FIFF_MNE_SOURCE_SPACE_NNEIGHBORS    3594    /* Number of neighbors for each source space point (used for volume source spaces) */
#define FIFF_MNE_SOURCE_SPACE_NEIGHBORS     3595    /* Neighbors for each source space point (used for volume source spaces) */

#define FIFFV_MNE_COORD_SURFACE_RAS   FIFFV_COORD_MRI    /* The surface RAS coordinates */




//ToDo remove later on
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






#define X_16 0
#define Y_16 1
#define Z_16 2


#define VEC_COPY_16(to,from) {\
    (to)[X_16] = (from)[X_16];\
    (to)[Y_16] = (from)[Y_16];\
    (to)[Z_16] = (from)[Z_16];\
    }



#define MALLOC_16(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_16(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define ALLOC_CMATRIX_16(x,y) mne_cmatrix_16((x),(y))



static void matrix_error_16(int kind, int nr, int nc)

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


float **mne_cmatrix_16(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_16(nr,float *);
    if (!m) matrix_error_16(1,nr,nc);
    whole = MALLOC_16(nr*nc,float);
    if (!whole) matrix_error_16(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}




#define FREE_16(x) if ((char *)(x) != NULL) free((char *)(x))
#define FREE_CMATRIX_16(m) mne_free_cmatrix_16((m))

void mne_free_cmatrix_16 (float **m)
{
    if (m) {
        FREE_16(*m);
        FREE_16(m);
    }
}








void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}


//int
void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat)
{
    fromIntEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}








//============================= mne_coord_transforms.c =============================

fiffCoordTrans fiff_invert_transform_16 (fiffCoordTrans t)

{
    fiffCoordTrans ti = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
    int j,k;

    for (j = 0; j < 3; j++) {
        ti->move[j] = t->invmove[j];
        ti->invmove[j] = t->move[j];
        for (k = 0; k < 3; k++) {
            ti->rot[j][k]    = t->invrot[j][k];
            ti->invrot[j][k] = t->rot[j][k];
        }
    }
    ti->from = t->to;
    ti->to   = t->from;
    return (ti);
}



fiffCoordTrans mne_read_transform_from_node(//fiffFile in,
                                            FiffStream::SPtr& stream,
                                            const FiffDirNode::SPtr& node,
                                            int from, int to)
/*
      * Read the specified coordinate transformation
      */
{
    fiffCoordTrans res = NULL;
    FiffTag::SPtr t_pTag;
//    fiffTagRec     tag;
//    fiffDirEntry   dir;
    fiff_int_t kind, pos;
    int k;

//    tag.data = NULL;
    for (k = 0; k < node->nent; k++)
        kind = node->dir[k]->kind;
        pos  = node->dir[k]->pos;
        if (kind == FIFF_COORD_TRANS) {
//            if (fiff_read_this_tag (in->fd,dir->pos,&tag) == FIFF_FAIL)
//                goto out;
//            res = (fiffCoordTrans)tag.data;
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto out;
            res = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
            *res = *(fiffCoordTrans)t_pTag->data();
            if (res->from == from && res->to == to) {
//                tag.data = NULL;
                goto out;
            }
            else if (res->from == to && res->to == from) {
                res = fiff_invert_transform_16(res);
                goto out;
            }
            res = NULL;
        }
    printf("No suitable coordinate transformation found");
    goto out;

out : {
//        FREE(tag.data);
        return res;
    }
}




//============================= mne_source_space.c =============================


int mne_read_source_spaces(const QString& name,               /* Read from here */
                           MneSurfaceOrVolume::MneCSourceSpace* **spacesp, /* These are the results */
                           int            *nspacep)
/*
      * Read source spaces from a FIFF file
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int            nspace = 0;
    MneSurfaceOrVolume::MneCSourceSpace* *spaces = NULL;
    MneSurfaceOrVolume::MneCSourceSpace*  new_space = NULL;
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

    extern void mne_add_triangle_data(MneSurfaceOrVolume::MneCSourceSpace* s);

    if(!stream->open())
        goto bad;

    sources = stream->tree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (sources.size() == 0) {
        printf("No source spaces available here");
        goto bad;
    }
    for (j = 0; j < sources.size(); j++) {
        new_space = MneSurfaceOrVolume::mne_new_source_space(0);
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
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_POINTS, t_pTag))
            goto bad;
        MatrixXf tmp_rr = t_pTag->toFloatMatrix().transpose();
        new_space->rr = ALLOC_CMATRIX_16(tmp_rr.rows(),tmp_rr.cols());
        fromFloatEigenMatrix_16(tmp_rr,new_space->rr);
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NORMALS, t_pTag))
            goto bad;
        MatrixXf tmp_nn = t_pTag->toFloatMatrix().transpose();
        new_space->nn = ALLOC_CMATRIX_16(tmp_nn.rows(),tmp_nn.cols());
        fromFloatEigenMatrix_16(tmp_nn,new_space->nn);
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
            int **itris = NULL;

            if (!node->find_tag(stream, FIFF_BEM_SURF_TRIANGLES, t_pTag)) {
                if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, t_pTag))
                    goto bad;
            }

            MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
            itris = (int **)malloc(tmp_itris.rows() * sizeof(int *));
            for (int i = 0; i < tmp_itris.rows(); ++i)
                itris[i] = (int *)malloc(tmp_itris.cols() * sizeof(int));
            fromIntEigenMatrix_16(tmp_itris, itris);

            for (p = 0; p < ntri; p++) { /* Adjust the numbering */
                itris[p][X_16]--;
                itris[p][Y_16]--;
                itris[p][Z_16]--;
            }
            new_space->itris = itris; itris = NULL;
            new_space->ntri = ntri;
        }
        if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE, t_pTag)) {
            if (new_space->type == FIFFV_MNE_SPACE_VOLUME) {
                /*
                * Use all
                */
                new_space->nuse   = new_space->np;
                new_space->inuse  = MALLOC_16(new_space->nuse,int);
                new_space->vertno = MALLOC_16(new_space->nuse,int);
                for (k = 0; k < new_space->nuse; k++) {
                    new_space->inuse[k]  = TRUE;
                    new_space->vertno[k] = k;
                }
            }
            else {
                /*
                * None in use
                * NOTE: The consequences of this change have to be evaluated carefully
                */
                new_space->nuse   = 0;
                new_space->inuse  = MALLOC_16(new_space->np,int);
                new_space->vertno = NULL;
                for (k = 0; k < new_space->np; k++)
                    new_space->inuse[k]  = FALSE;
            }
        }
        else {
            new_space->nuse = *t_pTag->toInt();
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_SELECTION, t_pTag)) {
                goto bad;
            }

            qDebug() << "ToDo: Check whether new_space->inuse contains the right stuff!!! - use VectorXi instead";
            new_space->inuse  = t_pTag->toInt();
            if (new_space->nuse > 0) {
                new_space->vertno = MALLOC_16(new_space->nuse,int);
                for (k = 0, p = 0; k < new_space->np; k++)
                    if (new_space->inuse[k])
                        new_space->vertno[p++] = k;
            }
            else {
                FREE_16(new_space->vertno);
                new_space->vertno = NULL;
            }
            /*
            * Selection triangulation
            */
            ntri = 0;
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI, t_pTag)) {
                ntri = *t_pTag->toInt();
            }
            if (ntri > 0) {
                int **itris = NULL;

                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, t_pTag))
                    goto bad;

                MatrixXi tmp_itris = t_pTag->toIntMatrix().transpose();
                itris = (int **)malloc(tmp_itris.rows() * sizeof(int *));
                for (int i = 0; i < tmp_itris.rows(); ++i)
                    itris[i] = (int *)malloc(tmp_itris.cols() * sizeof(int));
                fromIntEigenMatrix_16(tmp_itris, itris);
                for (p = 0; p < ntri; p++) { /* Adjust the numbering */
                    itris[p][X_16]--;
                    itris[p][Y_16]--;
                    itris[p][Z_16]--;
                }
                new_space->use_itris = itris; itris = NULL;
                new_space->nuse_tri = ntri;
            }
            /*
            * The patch information becomes relevant here
            */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST, t_pTag)) {
                nearest  = t_pTag->toInt();
                if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, t_pTag)) {
                    goto bad;
                }
                qDebug() << "ToDo: Check whether nearest_dist contains the right stuff!!! - use VectorXf instead";
                nearest_dist = t_pTag->toFloat();
                new_space->nearest = MALLOC_16(new_space->np,mneNearestRec);
                for (k = 0; k < new_space->np; k++) {
                    new_space->nearest[k].vert = k;
                    new_space->nearest[k].nearest = nearest[k];
                    new_space->nearest[k].dist = nearest_dist[k];
                    new_space->nearest[k].patch = NULL;
                }
                FREE_16(nearest); nearest = NULL;
                FREE_16(nearest_dist); nearest_dist = NULL;
            }
            /*
            * We may have the distance matrix
            */
            if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, t_pTag)) {
                new_space->dist_limit = *t_pTag->toInt();
                if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST, t_pTag)) {
                    INVERSELIB::FiffSparseMatrix* dist = INVERSELIB::FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
                    new_space->dist = dist->mne_add_upper_triangle_rcs();
                    delete dist;
                    if (!new_space->dist)
                        goto bad;
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
                new_space->nneighbor_vert = MALLOC_16(nvert,int);
                new_space->neighbor_vert  = MALLOC_16(nvert,int *);
                for (k = 0, q = 0; k < nvert; k++) {
                    new_space->nneighbor_vert[k] = nneigh = nneighbors[k];
                    new_space->neighbor_vert[k] = neigh = MALLOC_16(nneigh,int);
                    for (p = 0; p < nneigh; p++,q++)
                        neigh[p] = neighbors[q];
                }
            }
            FREE_16(neighbors);
            FREE_16(nneighbors);
            nneighbors = neighbors = NULL;
            /*
            * There might be a coordinate transformation and dimensions
            */
            new_space->voxel_surf_RAS_t   = mne_read_transform_from_node(stream, node, FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS);
            if (!node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, t_pTag)) {
                qDebug() << "ToDo: Check whether vol_dims contains the right stuff!!! - use VectorXi instead";
                vol_dims = t_pTag->toInt();
            }
            if (vol_dims)
                VEC_COPY_16(new_space->vol_dims,vol_dims);
            {
                QList<FiffDirNode::SPtr>  mris = node->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);

                if (mris.size() == 0) { /* The old way */
                    new_space->MRI_surf_RAS_RAS_t = mne_read_transform_from_node(stream, node, FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS);
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_MRI_FILE, t_pTag)) {
                        qDebug() << "ToDo: Check whether new_space->MRI_volume  contains the right stuff!!! - use QString instead";
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    if (node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator = INVERSELIB::FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
                    }
                }
                else {
                    if (node->find_tag(stream, FIFF_MNE_FILE_NAME, t_pTag)) {
                        new_space->MRI_volume = (char *)t_pTag->data();
                    }
                    new_space->MRI_surf_RAS_RAS_t = mne_read_transform_from_node(stream, mris[0], FIFFV_MNE_COORD_SURFACE_RAS, FIFFV_MNE_COORD_RAS);
                    new_space->MRI_voxel_surf_RAS_t   = mne_read_transform_from_node(stream, mris[0], FIFFV_MNE_COORD_MRI_VOXEL, FIFFV_MNE_COORD_SURFACE_RAS);

                    if (mris[0]->find_tag(stream, FIFF_MNE_SOURCE_SPACE_INTERPOLATOR, t_pTag)) {
                        new_space->interpolator = INVERSELIB::FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
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
        mne_add_triangle_data(new_space);
        spaces = REALLOC_16(spaces,nspace+1,MneSurfaceOrVolume::MneCSourceSpace*);
        spaces[nspace++] = new_space;
        new_space = NULL;
    }
    stream->close();

    *spacesp = spaces;
    *nspacep = nspace;

    return FIFF_OK;

bad : {
        stream->close();
        delete new_space;
        for (k = 0; k < nspace; k++)
            delete spaces[k];
        FREE_16(spaces);
        FREE_16(nearest);
        FREE_16(nearest_dist);
        FREE_16(neighbors);
        FREE_16(nneighbors);
        FREE_16(vol_dims);

        return FIFF_FAIL;
    }
}




//============================= fiff_trans.c =============================

void fiff_coord_trans_inv_16 (float r[3],fiffCoordTrans t,int do_move)
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



//============================= fwd_bem_model.c =============================


static struct {
    int  kind;
    const char *name;
} surf_expl_16[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };


const char *fwd_bem_explain_surface_16(int kind)

{
    int k;

    for (k = 0; surf_expl_16[k].kind >= 0; k++)
        if (surf_expl_16[k].kind == kind)
            return surf_expl_16[k].name;

    return surf_expl_16[k].name;
}


MneSurfaceOrVolume::MneCSurface* fwd_bem_find_surface_16(fwdBemModel model, int kind)
/*
 * Return a pointer to a specific surface in a BEM
 */
{
    int k;
    if (!model) {
        printf("No model specified for fwd_bem_find_surface");
        return NULL;
    }
    for (k = 0; k < model->nsurf; k++)
        if (model->surfs[k]->id == kind)
            return model->surfs[k];
    printf("Desired surface (%d = %s) not found.",
           kind,fwd_bem_explain_surface_16(kind));
    return NULL;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GuessData::GuessData()
: rr(NULL)
, guess_fwd(NULL)
, nguess(0)
{

}


//*************************************************************************************************************

//GuessData::GuessData(const GuessData& p_GuessData)
//{
//}


//*************************************************************************************************************

GuessData::GuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, DipoleFitData *f)
{
    MneSurfaceOrVolume::MneCSourceSpace* *sp = NULL;
    int            nsp = 0;
//    GuessData*      res = new GuessData();
    int            k,p;
    float          guessrad = 0.080;
    MneSurfaceOrVolume::MneCSourceSpace* guesses = NULL;
    dipoleFitFuncs orig;

    if (!guessname.isEmpty()) {
        /*
            * Read the guesses and transform to the appropriate coordinate frame
            */
        if (mne_read_source_spaces(guessname,&sp,&nsp) == FAIL)
            goto bad;
        if (nsp != 1) {
            printf("Incorrect number of source spaces in guess file");
            for (k = 0; k < nsp; k++)
                delete sp[k];
            FREE_16(sp);
            goto bad;
        }
        fprintf(stderr,"Read guesses from %s\n",guessname.toLatin1().constData());
        guesses = sp[0]; FREE_16(sp);
    }
    else {
        MneSurfaceOrVolume::MneCSurface*    inner_skull = NULL;
        int            free_inner_skull = FALSE;
        float          r0[3];

        VEC_COPY_16(r0,f->r0);
        fiff_coord_trans_inv_16(r0,f->mri_head_t,TRUE);
        if (f->bem_model) {
            fprintf(stderr,"Using inner skull surface from the BEM (%s)...\n",f->bemname);
            if ((inner_skull = fwd_bem_find_surface_16(f->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            fprintf(stderr,"Reading inner skull surface from %s...\n",guess_surfname.toLatin1().data());
            if ((inner_skull = mne_read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,TRUE,NULL)) == NULL)
                goto bad;
            free_inner_skull = TRUE;
        }
        if ((guesses = make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist)) == NULL)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    if (mne_transform_source_spaces_to(f->coord_frame,f->mri_head_t,&guesses,1) != OK)
        goto bad;
    fprintf(stderr,"Guess locations are now in %s coordinates.\n",mne_coord_frame_name(f->coord_frame));
    this->nguess  = guesses->nuse;
    this->rr      = ALLOC_CMATRIX_16(guesses->nuse,3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            VEC_COPY_16(this->rr[p],guesses->rr[k]);
            p++;
        }
    delete guesses; guesses = NULL;

    fprintf(stderr,"Go through all guess source locations...");
    this->guess_fwd = MALLOC_16(this->nguess,DipoleForward*);
    for (k = 0; k < this->nguess; k++)
        this->guess_fwd[k] = NULL;
    /*
        * Compute the guesses using the sphere model for speed
        */
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs;
    else
        f->funcs = f->sphere_funcs;

    for (k = 0; k < this->nguess; k++) {
        if ((this->guess_fwd[k] = DipoleFitData::dipole_forward_one(f,this->rr[k],NULL)) == NULL)
            goto bad;
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;

    fprintf(stderr,"[done %d sources]\n",p);

    return;
//    return res;

bad : {
        if(guesses)
            delete guesses;

        return;
//        return NULL;
    }
}


//*************************************************************************************************************

GuessData::GuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, DipoleFitData *f, char *guess_save_name)
{
    MneSurfaceOrVolume::MneCSourceSpace* *sp = NULL;
    int             nsp = 0;
    GuessData*      res = NULL;
    int             k,p;
    float           guessrad = 0.080f;
    MneSurfaceOrVolume::MneCSourceSpace*  guesses = NULL;

    if (!guessname.isEmpty()) {
        /*
         * Read the guesses and transform to the appropriate coordinate frame
         */
        if (mne_read_source_spaces(guessname,&sp,&nsp) == FIFF_FAIL)
            goto bad;
        if (nsp != 1) {
            qCritical("Incorrect number of source spaces in guess file");
            for (k = 0; k < nsp; k++)
                delete sp[k];
            FREE_16(sp);
            goto bad;
        }
        printf("Read guesses from %s\n",guessname.toLatin1().constData());
        guesses = sp[0]; FREE_16(sp);
    }
    else {
        MneSurfaceOrVolume::MneCSurface*     inner_skull = NULL;
        int            free_inner_skull = FALSE;
        float          r0[3];

        VEC_COPY_16(r0,f->r0);
        fiff_coord_trans_inv_16(r0,f->mri_head_t,TRUE);
        if (f->bem_model) {
            printf("Using inner skull surface from the BEM (%s)...\n",f->bemname);
            if ((inner_skull = fwd_bem_find_surface(f->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            printf("Reading inner skull surface from %s...\n",guess_surfname.toLatin1().data());
            if ((inner_skull = mne_read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,TRUE,NULL)) == NULL)
                goto bad;
            free_inner_skull = TRUE;
        }
        if ((guesses = make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist)) == NULL)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    /*
       * Save the guesses for future use
       */
    if (guesses->nuse == 0) {
        qCritical("No active guess locations remaining.");
        goto bad;
    }
    if (guess_save_name) {
        printf("###################DEBUG writing source spaces not yet implemented.");
        //    if (mne_write_source_spaces(guess_save_name,&guesses,1,FALSE) != OK)
        //      goto bad;
        //    printf("Wrote guess locations to %s\n",guess_save_name);
    }
    /*
       * Transform the guess locations to the appropriate coordinate frame
       */
    if (mne_transform_source_spaces_to(f->coord_frame,f->mri_head_t,&guesses,1) != OK)
        goto bad;
    printf("Guess locations are now in %s coordinates.\n",mne_coord_frame_name(f->coord_frame));

    res = new GuessData();
    this->nguess  = guesses->nuse;
    this->rr      = ALLOC_CMATRIX_16(guesses->nuse,3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            VEC_COPY_16(this->rr[p],guesses->rr[k]);
            p++;
        }
    if(guesses)
        delete guesses;
    guesses = NULL;

    this->guess_fwd = MALLOC_16(this->nguess,DipoleForward*);
    for (k = 0; k < this->nguess; k++)
        this->guess_fwd[k] = NULL;
    /*
        * Compute the guesses using the sphere model for speed
        */
    if (!this->compute_guess_fields(f))
        goto bad;

    return;
//    return res;

bad : {
        if(guesses)
            delete guesses;
        delete res;
        return;
//        return NULL;
    }
}


//*************************************************************************************************************

GuessData::~GuessData()
{
    FREE_CMATRIX_16(rr);
    if (guess_fwd) {
        for (int k = 0; k < nguess; k++)
            delete guess_fwd[k];
        FREE_16(guess_fwd);
    }
    return;
}


//*************************************************************************************************************

bool GuessData::compute_guess_fields(DipoleFitData* f)
{
    dipoleFitFuncs orig = NULL;

    if (!f) {
        qCritical("Data missing in compute_guess_fields");
        return false;
    }
    if (!f->noise) {
        qCritical("Noise covariance missing in compute_guess_fields");
        return false;
    }
    printf("Go through all guess source locations...");
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs;
    else
        f->funcs = f->sphere_funcs;
    for (int k = 0; k < this->nguess; k++) {
        if ((this->guess_fwd[k] = DipoleFitData::dipole_forward_one(f,this->rr[k],this->guess_fwd[k])) == NULL){
            if (orig)
                f->funcs = orig;
            return false;
        }
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;
    printf("[done %d sources]\n",this->nguess);

    return true;
}
