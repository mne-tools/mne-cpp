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
                guess_surf->id,fwd_bem_explain_surface(guess_surf->id),
                mne_coord_frame_name(guess_surf->coord_frame));
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
    s->inuse  = MALLOC(s->np,int);
    s->vertno = MALLOC(s->np,int);
    for (k = 0; k < s->np; k++) {
        s->inuse[k]  = TRUE;
        s->vertno[k] = k;
    }
    if (sigmap)
        *sigmap = sigma;

    return s;

bad : {
        FREE_CMATRIX(nodes);
        FREE_CMATRIX(node_normals);
        FREE_ICMATRIX(triangles);
        stream->close();
        return NULL;
    }
}
