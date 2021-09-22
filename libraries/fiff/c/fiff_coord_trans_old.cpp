//=============================================================================================================
/**
 * @file     fiff_coord_trans_old.cpp
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
 * @brief    Definition of the FiffCoordTransOld Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans_old.h"

#include <fiff/fiff_tag.h>

#include <QFile>

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FIFFLIB;

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

#define X_20 0
#define Y_20 1
#define Z_20 2

#define FREE_20(x) if ((char *)(x) != NULL) free((char *)(x))

#define MALLOC_20(x,t) (t *)malloc((x)*sizeof(t))

static void matrix_error_20(int kind, int nr, int nc)

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

float **mne_cmatrix_20(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_20(nr,float *);
    if (!m) matrix_error_20(1,nr,nc);
    whole = MALLOC_20(nr*nc,float);
    if (!whole) matrix_error_20(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

#define VEC_DIFF_20(from,to,diff) {\
    (diff)[X_20] = (to)[X_20] - (from)[X_20];\
    (diff)[Y_20] = (to)[Y_20] - (from)[Y_20];\
    (diff)[Z_20] = (to)[Z_20] - (from)[Z_20];\
    }

#define ALLOC_CMATRIX_20(x,y) mne_cmatrix_20((x),(y))

#define MAXWORD 1000

#define VEC_DOT_20(x,y) ((x)[X_20]*(y)[X_20] + (x)[Y_20]*(y)[Y_20] + (x)[Z_20]*(y)[Z_20])

#define VEC_LEN_20(x) sqrt(VEC_DOT_20(x,x))

#define CROSS_PRODUCT_20(x,y,xy) {\
    (xy)[X_20] =   (x)[Y_20]*(y)[Z_20]-(y)[Y_20]*(x)[Z_20];\
    (xy)[Y_20] = -((x)[X_20]*(y)[Z_20]-(y)[X_20]*(x)[Z_20]);\
    (xy)[Z_20] =   (x)[X_20]*(y)[Y_20]-(y)[X_20]*(x)[Y_20];\
    }

#define FREE_CMATRIX_20(m) mne_free_cmatrix_20((m))

void mne_free_cmatrix_20(float **m)
{
    if (m) {
        FREE_20(*m);
        FREE_20(m);
    }
}

#define MIN_20(a,b) ((a) < (b) ? (a) : (b))

//float
Eigen::MatrixXf toFloatEigenMatrix_20(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenVector_20(const Eigen::VectorXf& from_vec, float *to_vec, const int n)
{
    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
}

void fromFloatEigenMatrix_20(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

int mne_svd_20(float **mat,	/* The matrix */
            int   m,int n,	/* m rows n columns */
            float *sing,	/* Singular values (must have size
                             * MIN(m,n)+1 */
            float **uu,		/* Left eigenvectors */
            float **vv)		/* Right eigenvectors */
/*
      * Compute the SVD of mat.
      * The singular vector calculations depend on whether
      * or not u and v are given.
      *
      * The allocations should be done as follows
      *
      * mat = ALLOC_CMATRIX_3(m,n);
      * vv  = ALLOC_CMATRIX_3(MIN(m,n),n);
      * uu  = ALLOC_CMATRIX_3(MIN(m,n),m);
      * sing = MALLOC_3(MIN(m,n),float);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */

{
    int    udim = MIN_20(m,n);

    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_20(mat, m, n);

    //ToDo Optimize computation depending of whether uu or vv are defined
    Eigen::JacobiSVD< Eigen::MatrixXf > svd(eigen_mat ,Eigen::ComputeFullU | Eigen::ComputeFullV);

    fromFloatEigenVector_20(svd.singularValues(), sing, svd.singularValues().size());

    if (uu != NULL)
        fromFloatEigenMatrix_20(svd.matrixU().transpose(), uu, udim, m);

    if (vv != NULL)
        fromFloatEigenMatrix_20(svd.matrixV().transpose(), vv, m, n);

    return 0;
    //  return info;
}

void mne_matt_mat_mult2_20 (float **m1,float **m2,float **result,
                         int d1,int d2,int d3)
     /* Matrix multiplication
      * result(d1 x d3) = m1(d2 x d1)^T * m2(d2 x d3) */

{
    #ifdef BLAS
    char  *transa = "N";
    char  *transb = "T";
    float zero = 0.0;
    float one  = 1.0;

    sgemm (transa,transb,&d3,&d1,&d2,
            &one,m2[0],&d3,m1[0],&d1,&zero,result[0],&d3);

    return;
    #else
    int j,k,p;
    float sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
        for (p = 0; p < d2; p++)
            sum = sum + m1[p][j]*m2[p][k];
        result[j][k] = sum;
    }
    return;
    #endif
}

float **mne_matt_mat_mult_20 (float **m1,float **m2,int d1,int d2,int d3)

{
  float **result = ALLOC_CMATRIX_20(d1,d3);
  mne_matt_mat_mult2_20(m1,m2,result,d1,d2,d3);
  return result;
}

static void skip_comments(FILE *in)

{
    int c;

    while (1) {
        c = fgetc(in);
        if (c == '#') {
            for (c = fgetc(in); c != EOF && c != '\n'; c = fgetc(in))
                ;
        }
        else {
            ungetc(c,in);
            return;
        }
    }
}

static int whitespace(int c)

{
    if (c == '\t' || c == '\n' || c == ' ')
        return TRUE;
    else
        return FALSE;
}

static int whitespace_quote(int c, int inquote)

{
    if (inquote)
        return (c == '"');
    else
        return (c == '\t' || c == '\n' || c == ' ');
}

static char *next_word_20(FILE *in)

{
    char *next = MALLOC_20(MAXWORD,char);
    int c;
    int  p,k;
    int  inquote;

    skip_comments(in);

    inquote = FALSE;
    for (k = 0, p = 0, c = fgetc(in); c != EOF && !whitespace_quote(c,inquote) ; c = fgetc(in), k++) {
        if (k == 0 && c == '"')
            inquote = TRUE;
        else
            next[p++] = c;
    }
    if (c == EOF && k == 0) {
        FREE_20(next);
        return NULL;
    }
    else
        next[p] = '\0';
    if (c != EOF) {
        for (k = 0, c = fgetc(in); whitespace(c) ; c = fgetc(in), k++)
            ;
        if (c != EOF)
            ungetc(c,in);
    }
#ifdef DEBUG
    if (next)
        printf("<%s>\n",next);
#endif
    return next;
}

static int get_fval_20(FILE *in, float *fval)
{
    char *next = next_word_20(in);
    if (next == NULL) {
        qWarning("bad integer");
        return FAIL;
    }
    else if (sscanf(next,"%g",fval) != 1) {
        qWarning("bad floating point number : %s",next);
        FREE_20(next);
        return FAIL;
    }
    FREE_20(next);
    return OK;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTransOld::FiffCoordTransOld()
{
}

//=============================================================================================================

FiffCoordTransOld::FiffCoordTransOld(const FiffCoordTransOld &p_FiffCoordTransOld)
{
    this->from = p_FiffCoordTransOld.from;
    this->to = p_FiffCoordTransOld.to;

    for (int j = 0; j < 3; j++) {
        this->move[j] = p_FiffCoordTransOld.move[j];
        this->invmove[j] = p_FiffCoordTransOld.invmove[j];
        for (int k = 0; k < 3; k++) {
            this->rot(j,k) = p_FiffCoordTransOld.rot(j,k);
            this->invrot(j,k) = p_FiffCoordTransOld.invrot(j,k);
        }
    }
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::catenate(FiffCoordTransOld *t1, FiffCoordTransOld *t2)
{
    FiffCoordTransOld* t = new FiffCoordTransOld;
    int j,k,p;

    t->to   = t1->to;
    t->from = t2->from;

    for (j = 0; j < 3; j++) {
        t->move[j] = t1->move[j];
        for (k = 0; k < 3; k++) {
            t->rot(j,k) = 0.0;
            t->move[j] += t1->rot(j,k)*t2->move[k];
            for (p = 0; p < 3; p++)
                t->rot(j,k) += t1->rot(j,p)*t2->rot(p,k);
        }
    }
    add_inverse(t);
    return (t);
}

//=============================================================================================================

FiffCoordTransOld::FiffCoordTransOld(int from, int to, float rot[3][3], float move[3])
{
    this->from = from;
    this->to   = to;

    for (int j = 0; j < 3; j++) {
        this->move[j] = move[j];
        for (int k = 0; k < 3; k++)
            this->rot(j,k) = rot[j][k];
    }
    add_inverse(this);
}

//=============================================================================================================

FiffCoordTransOld::~FiffCoordTransOld()
{
}

//=============================================================================================================

FiffCoordTrans FiffCoordTransOld::toNew()
{
    FiffCoordTrans t = *new FiffCoordTrans;
    MatrixXf trans = MatrixXf::Identity(4,4);
    trans.block(0,0,3,3) = this->rot;
    trans.block(0,3,3,1) = this->move;
    t.to = this->to;
    t.from = this->from;
    t.trans = trans;
    t.addInverse(t);
    return t;
}

//=============================================================================================================

int FiffCoordTransOld::add_inverse(FiffCoordTransOld *t)
{
    Matrix4f m = Matrix4f::Identity(4,4);

    m.block(0,0,3,3) = t->rot;
    m.block(0,3,3,1) = t->move;

    m = m.inverse().eval();

    t->invrot = m.block(0,0,3,3);
    t->invmove = m.block(0,3,3,1);

    return OK;
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::fiff_invert_transform() const
{
    FiffCoordTransOld* ti = new FiffCoordTransOld;
    ti->move = this->invmove;
    ti->invmove = this ->move;
    ti->rot = this->invrot;
    ti->invrot = this ->rot;
    ti->from = this->to;
    ti->to   = this->from;
    return (ti);
}

//=============================================================================================================

void FiffCoordTransOld::fiff_coord_trans(float r[], const FiffCoordTransOld *t, int do_move)
/*
 * Apply coordinate transformation
 */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->move[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->rot(j,k)*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::fiff_combine_transforms(int from, int to, FiffCoordTransOld *t1, FiffCoordTransOld *t2)
/*
 * Combine two coordinate transformations
 * to yield a transform from 'from' system
 * to 'to' system.
 *
 * Return NULL if this fails
 *
 */
{
    FiffCoordTransOld* t = NULL;
    int swapped = 0;
    FiffCoordTransOld* temp;
    /*
       * We have a total of eight possibilities:
       * four without swapping and four with
       */
    while (1) {
        if (t1->to == to && t2->from == from) {
            t1 = new FiffCoordTransOld(*t1);
            t2 = new FiffCoordTransOld(*t2);
            break;
        }
        else if (t1->from == to && t2->from == from) {
            FiffCoordTransOld* tmp_t1 = t1;
            t1 = tmp_t1->fiff_invert_transform();//Memory leak here!!
            delete tmp_t1;
            t2 = new FiffCoordTransOld(*t2);
            break;
        }
        else if (t1->to == to && t2->to == from) {
            t1 = new FiffCoordTransOld(*t1);
            FiffCoordTransOld* tmp_t2 = t2;
            t2 = tmp_t2->fiff_invert_transform();//Memory leak here!!
            delete tmp_t2;
            break;
        }
        else if (t1->from == to && t2->to == from) {
            FiffCoordTransOld* tmp_t1 = t1;
            t1 = tmp_t1->fiff_invert_transform();//Memory leak here!!
            delete tmp_t1;
            FiffCoordTransOld* tmp_t2 = t2;
            t2 = tmp_t2->fiff_invert_transform();//Memory leak here!!
            delete tmp_t2;
            break;
        }
        if (swapped) {  /* Already swapped and not found */
            qCritical("Cannot combine coordinate transforms");
            return (t);
        }
        temp = t1;      /* Try it swapped as well */
        t1 = t2;
        t2 = temp;
        swapped = 1;
    }
    if (t1->from  != t2->to)    /* Transforms must match on the other side as well */
        qCritical ("Cannot combine coordinate transforms");
    else                        /* We can do it directly */
        t = catenate(t1,t2);
    FREE_20(t1); FREE_20(t2);
    return (t);
}

//=============================================================================================================

void FiffCoordTransOld::fiff_coord_trans_inv(float r[], FiffCoordTransOld *t, int do_move)
/*
 * Apply inverse coordinate transformation
 */
{
    int j,k;
    float res[3];

    for (j = 0; j < 3; j++) {
        res[j] = (do_move ? t->invmove[j] :  0.0);
        for (k = 0; k < 3; k++)
            res[j] += t->invrot(j,k)*r[k];
    }
    for (j = 0; j < 3; j++)
        r[j] = res[j];
}

//=============================================================================================================

namespace FIFFLIB
{

typedef struct {
    int frame;
    const char *name;
} frameNameRec;

}

const char *FiffCoordTransOld::mne_coord_frame_name(int frame)
{
    static frameNameRec frames[] = {
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

//=============================================================================================================

void FiffCoordTransOld::mne_print_coord_transform_label(FILE *log, char *label, FiffCoordTransOld *t)
{
    int k,p;
    int frame;
    if (!label || strlen(label) == 0)
        fprintf(log,"Coordinate transformation: ");
    else
        fprintf(log,"%s",label);
    for (frame = t->from, k = 0; k < 2; k++) {
        if (k == 0) {
            fprintf(log,"%s -> ",mne_coord_frame_name(frame));
            frame = t->to;
        }
        else {
            fprintf(log,"%s\n",mne_coord_frame_name(frame));
            for (p = 0; p < 3; p++)
                fprintf(log,"\t% 8.6f % 8.6f % 8.6f\t% 7.2f mm\n",
                        t->rot(p,X_20),t->rot(p,Y_20),t->rot(p,Z_20),1000*t->move[p]);
            fprintf(log,"\t% 8.6f % 8.6f % 8.6f  % 7.2f\n",0.0,0.0,0.0,1.0);
        }
    }
}

//=============================================================================================================

void FiffCoordTransOld::mne_print_coord_transform(FILE *log, FiffCoordTransOld *t)
{
    mne_print_coord_transform_label(log,NULL,t);
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_transform(const QString &name, int from, int to)
/*
          * Read the specified coordinate transformation
          */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffCoordTransOld* res = NULL;
    //    fiffFile       in = NULL;
    FiffTag::SPtr t_pTag;
    //    fiffTagRec     tag;
    //    fiffDirEntry   dir;
    fiff_int_t kind, pos;
    int k;

    //    tag.data = NULL;
    //    if ((in = fiff_open(name.toUtf8().data())) == NULL)
    //        goto out;
    if(!stream->open())
        goto out;

    for (k = 0; k < stream->dir().size(); k++) {
        kind = stream->dir()[k]->kind;
        pos  = stream->dir()[k]->pos;
        if (kind == FIFF_COORD_TRANS) {
            //            if (fiff_read_this_tag (in->fd,dir->pos,&tag) == FIFF_FAIL)
            //                goto out;
            //            res = (fiffCoordTrans)tag.data;
            if (!stream->read_tag(t_pTag,pos))
                goto out;

            res = FiffCoordTransOld::read_helper( t_pTag );
            if (res->from == from && res->to == to) {
                //                tag.data = NULL;
                goto out;
            }
            else if (res->from == to && res->to == from) {
                FiffCoordTransOld* tmp_res = res;
                res = tmp_res->fiff_invert_transform();//Memory leak here!!
                delete tmp_res;
                goto out;
            }
            res = NULL;
        }
    }
    qCritical("No suitable coordinate transformation found in %s.",name.toUtf8().data());
    goto out;

out : {
        //        FREE(tag.data);
        //        fiff_close(in);
        stream->close();
        return res;
    }

    return res;
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_transform_from_node(FiffStream::SPtr &stream, const FiffDirNode::SPtr &node, int from, int to)
/*
 * Read the specified coordinate transformation
 */
{
    FiffCoordTransOld* res = NULL;
    FiffTag::SPtr t_pTag;
    //    fiffTagRec     tag;
    //    fiffDirEntry   dir;
    fiff_int_t kind, pos;
    int k;

    //    tag.data = NULL;
    for (k = 0; k < node->nent(); k++)
        kind = node->dir[k]->kind;
    pos  = node->dir[k]->pos;
    if (kind == FIFF_COORD_TRANS) {
        //            if (fiff_read_this_tag (in->fd,dir->pos,&tag) == FIFF_FAIL)
        //                goto out;
        //            res = (fiffCoordTrans)tag.data;
        if (!stream->read_tag(t_pTag,pos))
            goto out;
        res = FiffCoordTransOld::read_helper( t_pTag );
        if (res->from == from && res->to == to) {
            //                tag.data = NULL;
            goto out;
        }
        else if (res->from == to && res->to == from) {
            FiffCoordTransOld* tmp_res = res;
            res = tmp_res->fiff_invert_transform();//Memory leak here!!
            delete tmp_res;
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

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_mri_transform(const QString &name)
/*
          * Read the MRI -> HEAD coordinate transformation
          */
{
    return mne_read_transform(name,FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_meas_transform(const QString &name)
/*
 * Read the MEG device -> HEAD coordinate transformation
 */
{
    return mne_read_transform(name,FIFFV_COORD_DEVICE,FIFFV_COORD_HEAD);
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_transform_ascii(char *name, int from, int to)
/*
 * Read the Neuromag -> FreeSurfer transformation matrix
 */
{
    FILE *in = NULL;
    FiffCoordTransOld* t = NULL;
    float rot[3][3];
    float move[3];
    int   k;
    float dum;

    if ((in = fopen(name,"r")) == NULL) {
        qCritical(name);
        goto bad;
    }
    for (k = 0; k < 3; k++) {
        if (get_fval_20(in,rot[k]+X_20) == FAIL)
            goto noread;
        if (get_fval_20(in,rot[k]+Y_20) == FAIL)
            goto noread;
        if (get_fval_20(in,rot[k]+Z_20) == FAIL)
            goto noread;
        if (get_fval_20(in,move+k) == FAIL)
            goto noread;
    }
    for (k = 0; k < 4; k++) {
        if (get_fval_20(in,&dum) == FAIL)
            goto noread;
    }
    fclose(in);
    for (k = 0; k < 3; k++)
        move[k] = move[k]/1000.0;
    t  = new FiffCoordTransOld(from, to, rot, move );
    return t;

noread : {
        qCritical("Cannot read the coordinate transformation");
        goto bad;
    }

bad : {
        if(t)
            delete t;
        if (in != NULL)
            fclose(in);
        return NULL;
    }
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_read_FShead2mri_transform(char *name)
{
    return mne_read_transform_ascii(name,FIFFV_COORD_HEAD,FIFFV_COORD_MRI);
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::mne_identity_transform(int from, int to)
{
    float rot[3][3] = { { 1.0, 0.0, 0.0 },
                        { 0.0, 1.0, 0.0 },
                        { 0.0, 0.0, 1.0 } };
    float move[] = { 0.0, 0.0, 0.0 };
    return new FiffCoordTransOld(from,to,rot,move);
}

//=============================================================================================================

FiffCoordTransOld * FiffCoordTransOld::fiff_make_transform_card (int from,int to,
                                                                 float *rL,
                                                                 float *rN,
                                                                 float *rR)
/* 'from' coordinate system
 * cardinal points expressed in
 * the 'to' system */

{
    FiffCoordTransOld* t = new FiffCoordTransOld();
    float ex[3],ey[3],ez[3];	/* The unit vectors */
    float alpha,alpha1,len;
    float diff1[3],diff2[3];
    int   k;
    float r0[3];

    t->from = from;
    t->to   = to;
    for (k = 0; k < 3; k++) {
        diff1[k] = rN[k] - rL[k];
        diff2[k] = rR[k] - rL[k];
    }
    alpha = VEC_DOT_20(diff1,diff2)/VEC_DOT_20(diff2,diff2);
    len = VEC_LEN_20(diff2);
    alpha1 = 1.0 - alpha;

    for (k = 0; k < 3; k++) {
        r0[k] = alpha1*rL[k] + alpha*rR[k];
        ex[k] = diff2[k]/len;
        ey[k] = rN[k] - r0[k];
        t->move[k] = r0[k];
    }

    len = VEC_LEN_20(ey);

    for (k = 0; k < 3; k++)
        ey[k] = ey[k]/len;

    CROSS_PRODUCT_20 (ex,ey,ez);

    for (k = 0; k < 3; k++) {
        t->rot(k,X_20) = ex[k];
        t->rot(k,Y_20) = ey[k];
        t->rot(k,Z_20) = ez[k];
    }

    add_inverse (t);

    return (t);
}

//=============================================================================================================

FiffCoordTransOld* FiffCoordTransOld::procrustes_align(int   from_frame,  /* The coordinate frames */
                       int   to_frame,
                       float **fromp,     /* Point locations in these two coordinate frames */
                       float **top,
                       float *w,	  /* Optional weights */
                       int   np,	  /* How many points */
                       float max_diff)	  /* Maximum allowed difference */
/*
 * Perform an alignment using the the solution of the orthogonal (weighted) Procrustes problem
 */
{
    float **from = ALLOC_CMATRIX_20(np,3);
    float **to   = ALLOC_CMATRIX_20(np,3);
    float from0[3],to0[3],rr[3],diff[3];
    int   j,k,c,p;
    float rot[3][3];
    float move[3];

    /*
     * Calculate the centroids and subtract;
     */
    for (c = 0; c < 3; c++)
        from0[c] = to0[c] = 0.0;
    for (j = 0; j < np; j++) {
        for (c = 0; c < 3; c++) {
            from0[c] += fromp[j][c];
            to0[c] += top[j][c];
        }
    }
    for (c = 0; c < 3; c++) {
        from0[c] = from0[c]/np;
        to0[c] = to0[c]/np;
    }
    for (j = 0; j < np; j++) {
        for (c = 0; c < 3; c++) {
            from[j][c] = fromp[j][c] - from0[c];
            to[j][c]   = top[j][c]    - to0[c];
        }
    }
    /*
     * Compute the solution of the orthogonal Proscrustes problem
     */
    {
        float **S;
        float **uu = ALLOC_CMATRIX_20(3,3);
        float **vv = ALLOC_CMATRIX_20(3,3);
        float **R = NULL;
        float sing[3];

        if (w) {
            /*
            * This is the weighted version which allows multiplicity of points
            */
            S = ALLOC_CMATRIX_20(3,3);
            for (j = 0; j < 3; j++) {
                for (k = 0; k < 3; k++) {
                    S[j][k] = 0.0;
                    for (p = 0; p < np; p++)
                        S[j][k] += w[p]*from[p][j]*to[p][k];
                }
            }
        }
        else
            S = mne_matt_mat_mult_20(from,to,3,np,3);
        if (mne_svd_20(S,3,3,sing,uu,vv) != 0) {
            FREE_CMATRIX_20(S);
            FREE_CMATRIX_20(uu);
            FREE_CMATRIX_20(vv);
            goto bad;
        }
        R = mne_matt_mat_mult_20(vv,uu,3,3,3);
        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                rot[j][k] = R[j][k];
        FREE_CMATRIX_20(R);
        FREE_CMATRIX_20(S);
        FREE_CMATRIX_20(uu);
        FREE_CMATRIX_20(vv);
    }
    /*
     * Now we need to generate a transformed translation vector
     */
    for (j = 0; j < 3; j++) {
        move[j] = to0[j];
        for (k = 0; k < 3; k++)
            move[j] = move[j] - rot[j][k]*from0[k];
    }
    /*
     * Test the transformation and print the results
     */
    #ifdef DEBUG
    fprintf(stderr,"Procrustes matching (desired vs. transformed) :\n");
    #endif
    for (p = 0; p < np; p++) {
        for (j = 0; j < 3; j++) {
            rr[j] = move[j];
        for (k = 0; k < 3; k++)
            rr[j] += rot[j][k]*fromp[p][k];
        }
        VEC_DIFF_20(top[p],rr,diff);
        #ifdef DEBUG
        fprintf(stderr,"\t%7.2f %7.2f %7.2f mm <-> %7.2f %7.2f %7.2f mm diff = %8.3f mm\n",
        1000*top[p][0],1000*top[p][1],1000*top[p][2],
        1000*rr[0],1000*rr[1],1000*rr[2],1000*VEC_LEN(diff));
        #endif
        if (VEC_LEN_20(diff) > max_diff) {
            printf("To large difference in matching : %7.1f > %7.1f mm", 1000*VEC_LEN_20(diff),1000*max_diff);
            goto bad;
        }
    }
    #ifdef DEBUG
    fprintf(stderr,"\n");
    #endif

    FREE_CMATRIX_20(from);
    FREE_CMATRIX_20(to);

    return new FiffCoordTransOld(from_frame,to_frame,rot,move);

    bad : {
        FREE_CMATRIX_20(from);
        FREE_CMATRIX_20(to);
        return NULL;
    }
}

//=============================================================================================================

FiffCoordTransOld *FiffCoordTransOld::read_helper( FIFFLIB::FiffTag::SPtr& tag)
{
    FiffCoordTransOld* p_FiffCoordTrans = NULL;
    if(tag->isMatrix() || tag->getType() != FIFFT_COORD_TRANS_STRUCT || tag->data() == NULL)
        return p_FiffCoordTrans;
    else
    {
        p_FiffCoordTrans = new FiffCoordTransOld;
        qint32* t_pInt32 = (qint32*)tag->data();
        p_FiffCoordTrans->from = t_pInt32[0];
        p_FiffCoordTrans->to = t_pInt32[1];

        float* t_pFloat = (float*)tag->data();
        int count = 0;
        int r, c;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans->move[r] = t_pFloat[11+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans->rot(r,c) = t_pFloat[2+count];
                ++count;
            }
        }

        count = 0;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans->invmove[r] = t_pFloat[23+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans->invrot(r,c) = t_pFloat[14+count];
                ++count;
            }
        }

        return p_FiffCoordTrans;
    }
}
