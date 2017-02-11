//=============================================================================================================
/**
* @file     fwd_bem_model.cpp
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
* @brief    Implementation of the FwdBemModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_bem_model.h"

#include "mne_surface_or_volume.h"


#include <fiff/fiff_stream.h>

#include <QFile>


#define _USE_MATH_DEFINES
#include <math.h>


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





#define X_40 0
#define Y_40 1
#define Z_40 2


#define VEC_DIFF_40(from,to,diff) {\
    (diff)[X_40] = (to)[X_40] - (from)[X_40];\
    (diff)[Y_40] = (to)[Y_40] - (from)[Y_40];\
    (diff)[Z_40] = (to)[Z_40] - (from)[Z_40];\
    }


#define VEC_COPY_40(to,from) {\
    (to)[X_40] = (from)[X_40];\
    (to)[Y_40] = (from)[Y_40];\
    (to)[Z_40] = (from)[Z_40];\
    }



#define VEC_DOT_40(x,y) ((x)[X_40]*(y)[X_40] + (x)[Y_40]*(y)[Y_40] + (x)[Z_40]*(y)[Z_40])

#define VEC_LEN_40(x) sqrt(VEC_DOT_40(x,x))

#define CROSS_PRODUCT_40(x,y,xy) {\
    (xy)[X_40] =   (x)[Y_40]*(y)[Z_40]-(y)[Y_40]*(x)[Z_40];\
    (xy)[Y_40] = -((x)[X_40]*(y)[Z_40]-(y)[X_40]*(x)[Z_40]);\
    (xy)[Z_40] =   (x)[X_40]*(y)[Y_40]-(y)[X_40]*(x)[Y_40];\
    }




#define MALLOC_40(x,t) (t *)malloc((x)*sizeof(t))

#define ALLOC_CMATRIX_40(x,y) mne_cmatrix_40((x),(y))

#define FREE_40(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_40(m) mne_free_cmatrix_40((m))




void mne_free_cmatrix_40 (float **m)
{
    if (m) {
        FREE_40(*m);
        FREE_40(m);
    }
}


static void matrix_error_40(int kind, int nr, int nc)

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



float **mne_cmatrix_40 (int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_40(nr,float *);
    if (!m) matrix_error_40(1,nr,nc);
    whole = MALLOC_40(nr*nc,float);
    if (!whole) matrix_error_40(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}





//float
Eigen::MatrixXf toFloatEigenMatrix_40(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}

void fromFloatEigenMatrix_40(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_40(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_40(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}








float **mne_lu_invert_40(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_40(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix_40(eigen_mat_inv, mat);
    return mat;
}






void mne_transpose_square_40(float **mat, int n)
/*
      * In-place transpose of a square matrix
      */
{
    int j,k;
    float val;

    for (j = 1; j < n; j++)
        for (k = 0; k < j; k++) {
            val = mat[j][k];
            mat[j][k] = mat[k][j];
            mat[k][j] = val;
        }
    return;
}



float mne_dot_vectors_40(float *v1,
                       float *v2,
                       int   nn)

{
#ifdef BLAS
    int one = 1;
    float res = sdot(&nn,v1,&one,v2,&one);
    return res;
#else
    float res = 0.0;
    int   k;

    for (k = 0; k < nn; k++)
        res = res + v1[k]*v2[k];
    return res;
#endif
}





void mne_add_scaled_vector_to_40(float *v1,float scale, float *v2,int nn)

{
#ifdef BLAS
    float fscale = scale;
    int   one = 1;
    saxpy(&nn,&fscale,v1,&one,v2,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v2[k] = v2[k] + scale*v1[k];
#endif
    return;
}


void mne_scale_vector_40(double scale,float *v,int   nn)

{
#ifdef BLAS
    float  fscale = scale;
    int    one    = 1;
    sscal(&nn,&fscale,v,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v[k] = v[k]*scale;
#endif
}









static struct {
    int  kind;
    const char *name;
} surf_expl[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };


static struct {
    int  method;
    const char *name;
} method_expl[] = { { FWD_BEM_CONSTANT_COLL , "constant collocation" },
{ FWD_BEM_LINEAR_COLL   , "linear collocation" },
{ -1                    , "unknown" } };




//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdBemModel::FwdBemModel()
:surfs      (NULL)
,ntri       (NULL)
,np         (NULL)
,nsurf      (0)
,sigma      (NULL)
,gamma      (NULL)
,source_mult(NULL)
,field_mult (NULL)
,bem_method (FWD_BEM_UNKNOWN)
,solution   (NULL)
,nsol       (0)
,head_mri_t (NULL)
,v0         (NULL)
,use_ip_approach(false)
,ip_approach_limit(FWD_BEM_IP_APPROACH_LIMIT)
{

}


//*************************************************************************************************************

FwdBemModel::~FwdBemModel()
{
    FREE_CMATRIX_40(this->solution); this->solution = NULL;
//    FREE_40(this->sol_name); this->sol_name = NULL;
    FREE_40(this->v0); this->v0 = NULL;
    this->bem_method = FWD_BEM_UNKNOWN;
    this->nsol       = 0;
}


//*************************************************************************************************************

const char *FwdBemModel::fwd_bem_explain_surface(int kind)
{
    int k;

    for (k = 0; surf_expl[k].kind >= 0; k++)
        if (surf_expl[k].kind == kind)
            return surf_expl[k].name;

    return surf_expl[k].name;
}


//*************************************************************************************************************

const char *FwdBemModel::fwd_bem_explain_method(int method)

{
    int k;

    for (k = 0; method_expl[k].method >= 0; k++)
        if (method_expl[k].method == method)
            return method_expl[k].name;

    return method_expl[k].name;
}


//*************************************************************************************************************

int FwdBemModel::get_int(FiffStream::SPtr &stream, const FiffDirNode::SPtr &node, int what, int *res)
/*
    * Wrapper to get int's
    */
{
    FiffTag::SPtr t_pTag;
    if(node->find_tag(stream, what, t_pTag)) {
        if (t_pTag->getType() != FIFFT_INT) {
            printf("Expected an integer tag : %d (found data type %d instead)\n",what,t_pTag->getType() );
            return FAIL;
        }
        *res = *t_pTag->toInt();
        return OK;
    }
    return FAIL;
}


//*************************************************************************************************************

MneSurfaceOrVolume::MneCSurface *FwdBemModel::fwd_bem_find_surface(int kind)
{
    //    if (!model) {
    //        printf("No model specified for fwd_bem_find_surface");
    //        return NULL;
    //    }
    for (int k = 0; k < this->nsurf; k++)
        if (this->surfs[k]->id == kind)
            return this->surfs[k];
    printf("Desired surface (%d = %s) not found.", kind,fwd_bem_explain_surface(kind));
    return NULL;
}


//*************************************************************************************************************

FwdBemModel *FwdBemModel::fwd_bem_load_surfaces(const QString &name, int *kinds, int nkind)
/*
* Load a set of surfaces
*/
{
    MneSurfaceOrVolume::MneCSurface* *surfs = NULL;
    float      *sigma = NULL;
    float      *sigma1;
    FwdBemModel *m = NULL;
    int         j,k;

    if (nkind <= 0) {
        printf("No surfaces specified to fwd_bem_load_surfaces");
        return NULL;
    }

    surfs = MALLOC_40(nkind,MneSurfaceOrVolume::MneCSurface*);
    sigma = MALLOC_40(nkind,float);
    for (k = 0; k < nkind; k++)
        surfs[k] = NULL;

    for (k = 0; k < nkind; k++) {
        if ((surfs[k] = MneSurfaceOrVolume::MneCSurface::read_bem_surface(name,kinds[k],TRUE,sigma+k)) == NULL)
            goto bad;
        if (sigma[k] < 0.0) {
            printf("No conductivity available for surface %s",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
        if (surfs[k]->coord_frame != FIFFV_COORD_MRI) { /* We make our life much easier with this */
            printf("Surface %s not specified in MRI coordinates.",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
    }
    m = new FwdBemModel;

    m->surf_name = name;
    m->nsurf     = nkind;
    m->surfs     = surfs;
    m->sigma     = sigma;
    m->ntri      = MALLOC_40(nkind,int);
    m->np        = MALLOC_40(nkind,int);
    m->gamma = ALLOC_CMATRIX_40(nkind,nkind);
    m->source_mult = MALLOC_40(nkind,float);
    m->field_mult  = MALLOC_40(nkind,float);
    /*
       * Dirty trick for the zero conductivity outside
       */
    sigma1 = MALLOC_40(nkind+1,float);
    sigma1[0] = 0.0;
    sigma  = sigma1+1;
    for (k = 0; k < m->nsurf; k++)
        sigma[k] = m->sigma[k];
    /*
       * Gamma factors and multipliers
       */
    for (j = 0; j < m->nsurf; j++) {
        m->ntri[j] = m->surfs[j]->ntri;
        m->np[j]   = m->surfs[j]->np;
        m->source_mult[j] = 2.0/(sigma[j]+sigma[j-1]);
        m->field_mult[j] = sigma[j]-sigma[j-1];
        for (k = 0; k < m->nsurf; k++)
            m->gamma[j][k] = (sigma[k]-sigma[k-1])/(sigma[j]+sigma[j-1]);
    }
    FREE_40(sigma1);

    return m;

bad : {
        FREE_40(sigma);
        for (k = 0; k < nkind; k++)
            delete surfs[k];
        FREE_40(surfs);
        return NULL;
    }
}


//*************************************************************************************************************

FwdBemModel *FwdBemModel::fwd_bem_load_homog_surface(const QString &name)
/*
* Load surfaces for the homogeneous model
*/
{
    int kinds[] = { FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 1;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}


//*************************************************************************************************************

FwdBemModel *FwdBemModel::fwd_bem_load_three_layer_surfaces(const QString &name)
/*
* Load surfaces for three-layer model
*/
{
    int kinds[] = { FIFFV_BEM_SURF_ID_HEAD, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 3;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_load_solution(const QString &name, int bem_method, FwdBemModel *m)
/*
    * Load the potential solution matrix and attach it to the model:
    *
    * return values:
    *
    *       TRUE   found a suitable solution
    *       FALSE  did not find a suitable solution
    *       FAIL   error in reading the solution
    *
    */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    float       **sol = NULL;
    FiffDirNode::SPtr bem_node;
    int         method;
    FiffTag::SPtr t_pTag;
    int         nsol;

    if(!stream->open())
        goto not_found;

    /*
       * Find the BEM data
       */
    {
        QList<FiffDirNode::SPtr> nodes = stream->tree()->dir_tree_find(FIFFB_BEM);

        if (nodes.size() == 0) {
            printf ("No BEM data in %s",name.toLatin1().constData());
            goto not_found;
        }
        bem_node = nodes[0];
    }
    /*
        * Approximation method
        */
    if (get_int(stream,bem_node,FIFF_BEM_APPROX,&method) != OK)
        goto not_found;
    if (method == FIFFV_BEM_APPROX_CONST)
        method = FWD_BEM_CONSTANT_COLL;
    else if (method == FIFFV_BEM_APPROX_LINEAR)
        method = FWD_BEM_LINEAR_COLL;
    else {
        printf ("Cannot handle BEM approximation method : %d",method);
        goto bad;
    }
    if (bem_method != FWD_BEM_UNKNOWN && method != bem_method) {
        printf("Approximation method in file : %d desired : %d",method,bem_method);
        goto not_found;
    }
    {
        int         dim,k;

        if (!bem_node->find_tag(stream, FIFF_BEM_POT_SOLUTION, t_pTag))
            goto bad;
        qint32 ndim;
        QVector<qint32> dims;
        t_pTag->getMatrixDimensions(ndim, dims);

        if (ndim != 2) {
            printf("Expected a two-dimensional solution matrix instead of a %d dimensional one",ndim);
            goto bad;
        }
        for (k = 0, dim = 0; k < m->nsurf; k++)
            dim = dim + ((method == FWD_BEM_LINEAR_COLL) ? m->surfs[k]->np : m->surfs[k]->ntri);
        if (dims[0] != dim || dims[1] != dim) {
            printf("Expected a %d x %d solution matrix instead of a %d x %d  one",dim,dim,dims[0],dims[1]);
            goto not_found;
        }

        MatrixXf tmp_sol = t_pTag->toFloatMatrix().transpose();
        sol = ALLOC_CMATRIX_40(tmp_sol.rows(),tmp_sol.cols());
        fromFloatEigenMatrix_40(tmp_sol, sol);
        nsol = dims[1];
    }
    if(m)
        delete m;
    m->sol_name = name;
    m->solution = sol;
    m->nsol     = nsol;
    m->bem_method = method;
    stream->close();

    return TRUE;

bad : {
        stream->close();
        FREE_CMATRIX_40(sol);
        return FAIL;
    }

not_found : {
        stream->close();
        FREE_CMATRIX_40(sol);
        return FALSE;
    }
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_set_head_mri_t(FwdBemModel *m, FiffCoordTransOld *t)
/*
    * Set the coordinate transformation
    */
{
    if (t->from == FIFFV_COORD_HEAD && t->to == FIFFV_COORD_MRI) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = new FiffCoordTransOld( *t );
        return OK;
    }
    else if (t->from == FIFFV_COORD_MRI && t->to == FIFFV_COORD_HEAD) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = t->fiff_invert_transform();
        return OK;
    }
    else {
        printf ("Improper coordinate transform delivered to fwd_bem_set_head_mri_t");
        return FAIL;
    }
}


//*************************************************************************************************************

double FwdBemModel::calc_beta(double *rk, double *rk1)

{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF_40(rk,rk1,rkk1);
    size = VEC_LEN_40(rkk1);

    res = log((VEC_LEN_40(rk)*size + VEC_DOT_40(rk,rkk1))/
              (VEC_LEN_40(rk1)*size + VEC_DOT_40(rk1,rkk1)))/size;
    return (res);
}


//*************************************************************************************************************

void FwdBemModel::lin_pot_coeff(float *from, mneTriangle to, double omega[])	/* The final result */
/*
          * The linear potential matrix element computations
          */
{
    double y1[3],y2[3],y3[3];	/* Corners with origin at from */
    double *y[5];
    double **yy;
    double l1,l2,l3;		/* Lengths of y1, y2, and y3 */
    double solid;			/* The standard solid angle */
    double vec_omega[3];		/* The cross-product integral */
    double cross[3];		/* y1 x y2 */
    double triple;		/* VEC_DOT_40(y1 x y2,y3) */
    double ss;
    double beta[3],bbeta[3];
    int   j,k;
    double z[3];
    double n2,area2;
    double diff[3];
    static const double solid_eps = 4.0*M_PI/1.0E6;
    /*
       * This circularity makes things easy for us...
       */
    y[0] = y3;
    y[1] = y1;
    y[2] = y2;
    y[3] = y3;
    y[4] = y1;
    yy = y + 1;			/* yy can have index -1! */
    /*
       * The standard solid angle computation
       */
    VEC_DIFF_40(from,to->r1,y1);
    VEC_DIFF_40(from,to->r2,y2);
    VEC_DIFF_40(from,to->r3,y3);

    CROSS_PRODUCT_40(y1,y2,cross);
    triple = VEC_DOT_40(cross,y3);

    l1 = VEC_LEN_40(y1);
    l2 = VEC_LEN_40(y2);
    l3 = VEC_LEN_40(y3);
    ss = (l1*l2*l3+VEC_DOT_40(y1,y2)*l3+VEC_DOT_40(y1,y3)*l2+VEC_DOT_40(y2,y3)*l1);
    solid  = 2.0*atan2(triple,ss);
    if (fabs(solid) < solid_eps) {
        for (k = 0; k < 3; k++)
            omega[k] = 0.0;
    }
    else {
        /*
         * Calculate the magic vector vec_omega
         */
        for (j = 0; j < 3; j++)
            beta[j] = calc_beta(yy[j],yy[j+1]);
        bbeta[0] = beta[2] - beta[0];
        bbeta[1] = beta[0] - beta[1];
        bbeta[2] = beta[1] - beta[2];

        for (j = 0; j < 3; j++)
            vec_omega[j] = 0.0;
        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                vec_omega[k] = vec_omega[k] + bbeta[j]*yy[j][k];
        /*
         * Put it all together...
         */
        area2 = 2.0*to->area;
        n2 = 1.0/(area2*area2);
        for (k = 0; k < 3; k++) {
            CROSS_PRODUCT_40(yy[k+1],yy[k-1],z);
            VEC_DIFF_40(yy[k+1],yy[k-1],diff);
            omega[k] = n2*(-area2*VEC_DOT_40(z,to->nn)*solid +
                           triple*VEC_DOT_40(diff,vec_omega));
        }
    }
#ifdef CHECK
    /*
       * Check it out!
       *
       * omega1 + omega2 + omega3 = solid
       */
    rel1 = (solid + omega[X_40]+omega[Y_40]+omega[Z_40])/solid;
    /*
       * The other way of evaluating...
       */
    for (j = 0; j < 3; j++)
        check[j] = 0;
    for (k = 0; k < 3; k++) {
        CROSS_PRODUCT_40(to->nn[to],yy[k],z);
        for (j = 0; j < 3; j++)
            check[j] = check[j] + omega[k]*z[j];
    }
    for (j = 0; j < 3; j++)
        check[j] = -area2*check[j]/triple;
    fprintf (stderr,"(%g,%g,%g) =? (%g,%g,%g)\n",
             check[X_40],check[Y_40],check[Z_40],
             vec_omega[X_40],vec_omega[Y_40],vec_omega[Z_40]);
    for (j = 0; j < 3; j++)
        check[j] = check[j] - vec_omega[j];
    rel2 = sqrt(VEC_DOT_40(check,check)/VEC_DOT_40(vec_omega,vec_omega));
    fprintf (stderr,"err1 = %g, err2 = %g\n",100*rel1,100*rel2);
#endif
    return;
}


//*************************************************************************************************************

void FwdBemModel::correct_auto_elements(MneSurfaceOrVolume::MneCSurface *surf, float **mat)
/*
          * Improve auto-element approximation...
          */
{
    float *row;
    float sum,miss;
    int   nnode = surf->np;
    int   ntri  = surf->ntri;
    int   nmemb;
    int   j,k;
    float pi2 = 2.0*M_PI;
    mneTriangle   tri;

#ifdef SIMPLE
    for (j = 0; j < nnode; j++) {
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        fprintf (stderr,"row %d sum = %g missing = %g\n",j+1,sum/pi2,
                 1.0-sum/pi2);
        row[j] = pi2 - sum;
    }
#else
    for (j = 0; j < nnode; j++) {
        /*
         * How much is missing?
         */
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        miss  = pi2-sum;
        nmemb = surf->nneighbor_tri[j];
        /*
         * The node itself receives one half
         */
        row[j] = miss/2.0;
        /*
         * The rest is divided evenly among the member nodes...
         */
        miss = miss/(4.0*nmemb);
        for (k = 0,tri = surf->tris; k < ntri; k++,tri++) {
            if (tri->vert[0] == j) {
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[1] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[2] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
            }
        }
        /*
         * Just check it it out...
         *
        for (k = 0, sum = 0; k < nnode; k++)
          sum = sum + row[k];
        fprintf (stderr,"row %d sum = %g\n",j+1,sum/pi2);
        */
    }
#endif
    return;
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_lin_pot_coeff(MneSurfaceOrVolume::MneCSurface **surfs, int nsurf)
/*
          * Calculate the coefficients for linear collocation approach
          */
{
    float **mat = NULL;
    float **sub_mat = NULL;
    int   np1,np2,ntri,np_tot,np_max;
    float **nodes;
    mneTriangle   tri;
    double omega[3];
    double *row = NULL;
    int    j,k,p,q,c;
    int    joff,koff;
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;

    for (p = 0, np_tot = np_max = 0; p < nsurf; p++) {
        np_tot += surfs[p]->np;
        if (surfs[p]->np > np_max)
            np_max = surfs[p]->np;
    }

    mat = ALLOC_CMATRIX_40(np_tot,np_tot);
    for (j = 0; j < np_tot; j++)
        for (k = 0; k < np_tot; k++)
            mat[j][k] = 0.0;
    row        = MALLOC_40(np_max,double);
    sub_mat = MALLOC_40(np_max,float *);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + np1) {
        surf1 = surfs[p];
        np1   = surf1->np;
        nodes = surf1->rr;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + np2) {
            surf2 = surfs[q];
            np2   = surf2->np;
            ntri  = surf2->ntri;

            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",
                    fwd_bem_explain_surface(surf1->id),np1,
                    fwd_bem_explain_surface(surf2->id),np2);

            for (j = 0; j < np1; j++) {
                for (k = 0; k < np2; k++)
                    row[k] = 0.0;
                for (k = 0, tri = surf2->tris; k < ntri; k++,tri++) {
                    /*
               * No contribution from a triangle that
               * this vertex belongs to
               */
                    if (p == q && (tri->vert[0] == j || tri->vert[1] == j || tri->vert[2] == j))
                        continue;
                    /*
               * Otherwise do the hard job
               */
                    lin_pot_coeff (nodes[j],tri,omega);
                    for (c = 0; c < 3; c++)
                        row[tri->vert[c]] = row[tri->vert[c]] - omega[c];
                }
                for (k = 0; k < np2; k++)
                    mat[j+joff][k+koff] = row[k];
            }
            if (p == q) {
                for (j = 0; j < np1; j++)
                    sub_mat[j] = mat[j+joff]+koff;
                correct_auto_elements (surf1,sub_mat);
            }
            fprintf(stderr,"[done]\n");
        }
    }
    FREE_40(row);
    FREE_40(sub_mat);
    return(mat);
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_linear_collocation_solution(FwdBemModel *m)
/*
     * Compute the linear collocation potential solution
     */
{
    float **coeff = NULL;
    float ip_mult;
    int k;

    if(m)
        delete m;

    fprintf(stderr,"\nComputing the linear collocation solution...\n");
    fprintf (stderr,"\tMatrix coefficients...\n");
    if ((coeff = fwd_bem_lin_pot_coeff (m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->np;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (coeff,m->gamma,m->nsurf,m->np)) == NULL)
        goto bad;

    /*
       * IP approach?
       */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tMatrix coefficients (homog)...\n");
        if ((coeff = fwd_bem_lin_pot_coeff (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (coeff,m->surfs[m->nsurf-1]->np)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");

        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->np);
        FREE_CMATRIX_40(ip_solution);

    }
    m->bem_method = FWD_BEM_LINEAR_COLL;
    fprintf(stderr,"Solution ready.\n");
    return OK;

bad : {
        if(m)
            delete m;
        FREE_CMATRIX_40(coeff);
        return FAIL;
    }
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_multi_solution(float **solids, float **gamma, int nsurf, int *ntri)       /* Number of triangles or nodes on each surface */
/*
          * Invert I - solids/(2*M_PI)
          * Take deflation into account
          * The matrix is destroyed after inversion
          * This is the general multilayer case
          */
{
    int j,k,p,q;
    float defl;
    float pi2 = 1.0/(2*M_PI);
    float mult;
    int   joff,koff,jup,kup,ntot;

    for (j = 0,ntot = 0; j < nsurf; j++)
        ntot += ntri[j];
    defl = 1.0/ntot;
    /*
       * Modify the matrix
       */
    for (p = 0, joff = 0; p < nsurf; p++) {
        jup = ntri[p] + joff;
        for (q = 0, koff = 0; q < nsurf; q++) {
            kup = ntri[q] + koff;
            mult = (gamma == NULL) ? pi2 : pi2*gamma[p][q];
            for (j = joff; j < jup; j++)
                for (k = koff; k < kup; k++)
                    solids[j][k] = defl - solids[j][k]*mult;
            koff = kup;
        }
        joff = jup;
    }
    for (k = 0; k < ntot; k++)
        solids[k][k] = solids[k][k] + 1.0;

    return (mne_lu_invert_40(solids,ntot));
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_homog_solution(float **solids, int ntri)
/*
          * Invert I - solids/(2*M_PI)
          * Take deflation into account
          * The matrix is destroyed after inversion
          * This is the homogeneous model case
          */
{
    return fwd_bem_multi_solution (solids,NULL,1,&ntri);
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_ip_modify_solution(float **solution, float **ip_solution, float ip_mult, int nsurf, int *ntri)                  /* Number of triangles (nodes) on each surface */
/*
          * Modify the solution according to the IP approach
          */
{
    int s;
    int j,k,joff,koff,ntot,nlast;
    float mult;
    float *row = NULL;
    float **sub = NULL;

    for (s = 0, koff = 0; s < nsurf-1; s++)
        koff = koff + ntri[s];
    nlast = ntri[nsurf-1];
    ntot  = koff + nlast;

    row = MALLOC_40(nlast,float);
    sub = MALLOC_40(ntot,float *);
    mult = (1.0 + ip_mult)/ip_mult;

    fprintf(stderr,"\t\tCombining...");
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_40(ip_solution,nlast);
#endif
    for (s = 0, joff = 0; s < nsurf; s++) {
        fprintf(stderr,"%d3 ",s+1);
        /*
         * Pick the correct submatrix
         */
        for (j = 0; j < ntri[s]; j++)
            sub[j] = solution[j+joff]+koff;
        /*
         * Multiply
         */
#ifdef OLD
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++) {
                res = mne_dot_vectors_skip_skip(sub[j],1,ip_solution[0]+k,nlast,nlast);
                row[k] = sub[j][k] - 2.0*res;
            }
            for (k = 0; k < nlast; k++)
                sub[j][k] = row[k];
        }
#else
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++)
                row[k] = mne_dot_vectors_40(sub[j],ip_solution[k],nlast);
            mne_add_scaled_vector_to_40(row,-2.0,sub[j],nlast);
        }
#endif
        joff = joff+ntri[s];
    }
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_40(ip_solution,nlast);
#endif
    fprintf(stderr,"33 ");
    /*
       * The lower right corner is a special case
       */
    for (j = 0; j < nlast; j++)
        for (k = 0; k < nlast; k++)
            sub[j][k] = sub[j][k] + mult*ip_solution[j][k];
    /*
       * Final scaling
       */
    fprintf(stderr,"done.\n\t\tScaling...");
    mne_scale_vector_40(ip_mult,solution[0],ntot*ntot);
    fprintf(stderr,"done.\n");
    FREE_40(row); FREE_40(sub);
    return;
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_check_solids(float **angles, int ntri1, int ntri2, float desired)
/*
     * Check the angle computations
     */
{
    float *sums = MALLOC_40(ntri1,float);
    float sum;
    int j,k;
    int res = 0;

    for (j = 0; j < ntri1; j++) {
        sum = 0;
        for (k = 0; k < ntri2; k++)
            sum = sum + angles[j][k];
        sums[j] = sum/(2*M_PI);
    }
    for (j = 0; j < ntri1; j++)
        /*
         * Three cases:
         * same surface: sum = 2*pi
         * to outer:     sum = 4*pi
         * to inner:     sum = 0*pi;
         */
        if (fabs(sums[j]-desired) > 1e-4) {
            printf("solid angle matrix: rowsum[%d] = 2PI*%g",
                   j+1,sums[j]);
            res = -1;
            break;
        }
    FREE_40(sums);
    return res;
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_solid_angles(MneSurfaceOrVolume::MneCSurface **surfs, int nsurf)
/*
          * Compute the solid angle matrix
          */
{
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;
    mneTriangle tri;
    int ntri1,ntri2,ntri_tot;
    int j,k,p,q;
    int joff,koff;
    float **solids;
    float result;
    float **sub_solids = NULL;
    float desired;

    for (p = 0,ntri_tot = 0; p < nsurf; p++)
        ntri_tot += surfs[p]->ntri;

    sub_solids = MALLOC_40(ntri_tot,float *);
    solids = ALLOC_CMATRIX_40(ntri_tot,ntri_tot);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + ntri1) {
        surf1 = surfs[p];
        ntri1 = surf1->ntri;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + ntri2) {
            surf2 = surfs[q];
            ntri2 = surf2->ntri;
            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",fwd_bem_explain_surface(surf1->id),ntri1,fwd_bem_explain_surface(surf2->id),ntri2);
            for (j = 0; j < ntri1; j++)
                for (k = 0, tri = surf2->tris; k < ntri2; k++, tri++) {
                    if (p == q && j == k)
                        result = 0.0;
                    else
                        result = MneSurfaceOrVolume::solid_angle (surf1->tris[j].cent,tri);
                    solids[j+joff][k+koff] = result;
                }
            for (j = 0; j < ntri1; j++)
                sub_solids[j] = solids[j+joff]+koff;
            fprintf(stderr,"[done]\n");
            if (p == q)
                desired = 1;
            else if (p < q)
                desired = 0;
            else
                desired = 2;
            if (fwd_bem_check_solids(sub_solids,ntri1,ntri2,desired) == FAIL) {
                FREE_CMATRIX_40(solids);
                FREE_40(sub_solids);
                return NULL;
            }
        }
    }
    FREE_40(sub_solids);
    return (solids);
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_constant_collocation_solution(FwdBemModel *m)
/*
     * Compute the solution for the constant collocation approach
     */
{
    float  **solids = NULL;
    int    k;
    float  ip_mult;

    if(m)
        delete m;

    fprintf(stderr,"\nComputing the constant collocation solution...\n");
    fprintf(stderr,"\tSolid angles...\n");
    if ((solids = fwd_bem_solid_angles(m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->ntri;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (solids,m->gamma,m->nsurf,m->ntri)) == NULL)
        goto bad;
    /*
       * IP approach?
       */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tSolid angles (homog)...\n");
        if ((solids = fwd_bem_solid_angles (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (solids,m->surfs[m->nsurf-1]->ntri)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");
        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->ntri);
        FREE_CMATRIX_40(ip_solution);
    }
    m->bem_method = FWD_BEM_CONSTANT_COLL;
    fprintf (stderr,"Solution ready.\n");

    return OK;

bad : {
        if(m)
            delete m;
        FREE_CMATRIX_40(solids);
        return FAIL;
    }
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_compute_solution(FwdBemModel *m, int bem_method)
/*
* Compute the solution
*/
{
    /*
        * Compute the solution
        */
    if (bem_method == FWD_BEM_LINEAR_COLL)
        return fwd_bem_linear_collocation_solution(m);
    else if (bem_method == FWD_BEM_CONSTANT_COLL)
        return fwd_bem_constant_collocation_solution(m);

    if(m)
        delete m;
    printf ("Unknown BEM method: %d\n",bem_method);
    return FAIL;
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_load_recompute_solution(char *name, int bem_method, int force_recompute, FwdBemModel *m)
/*
* Load or recompute the potential solution matrix
*/
{
    int solres;

    if (!m) {
        printf ("No model specified for fwd_bem_load_recompute_solution");
        return FAIL;
    }

    if (!force_recompute) {
        if(m)
            delete m;
        solres = fwd_bem_load_solution(name,bem_method,m);
        if (solres == TRUE) {
            fprintf(stderr,"\nLoaded %s BEM solution from %s\n",fwd_bem_explain_method(m->bem_method),name);
            return OK;
        }
        else if (solres == FAIL)
            return FAIL;
#ifdef DEBUG
        else
            fprintf(stderr,"Desired BEM  solution not available in %s (%s)\n",name,err_get_error());
#endif
    }
    if (bem_method == FWD_BEM_UNKNOWN)
        bem_method = FWD_BEM_LINEAR_COLL;
    return fwd_bem_compute_solution(m,bem_method);
}


//*************************************************************************************************************

float FwdBemModel::fwd_bem_inf_field(float *rd, float *Q, float *rp, float *dir)     /* Which field component */
/*
     * Infinite-medium magnetic field
     * (without \mu_0/4\pi)
     */
{
    float diff[3],diff2,cross[3];

    VEC_DIFF_40(rd,rp,diff);
    diff2 = VEC_DOT_40(diff,diff);
    CROSS_PRODUCT_40(Q,diff,cross);

    return (VEC_DOT_40(cross,dir)/(diff2*sqrt(diff2)));
}


//*************************************************************************************************************

float FwdBemModel::fwd_bem_inf_pot(float *rd, float *Q, float *rp)	/* Potential point */
/*
     * The infinite medium potential
     */
{
    float diff[3];
    float diff2;
    VEC_DIFF_40(rd,rp,diff);
    diff2 = VEC_DOT_40(diff,diff);
    return (VEC_DOT_40(Q,diff)/(4.0*M_PI*diff2*sqrt(diff2)));
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_specify_els(FwdBemModel* m, FwdCoilSet *els)
/*
     * Set up for computing the solution at a set of electrodes
     */
{
    FwdCoil*     el;
    MneSurfaceOrVolume::MneCSurface*  scalp;
    int         k,p,q,v;
    float       *one_sol,*pick_sol;
    float       r[3],w[3],dist;
    int         best;
    mneTriangle tri;
    float       x,y,z;
    fwdBemSolution* sol;

    extern fwdBemSolution fwd_bem_new_coil_solution();
    extern void fwd_bem_free_coil_solution(void *user);

    if (!m) {
        printf("Model missing in fwd_bem_specify_els");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_els");
        goto bad;
    }
    if (!els || els->ncoil == 0)
        return OK;
    els->fwd_free_coil_set_user_data();
    /*
       * Hard work follows
       */
    els->user_data = sol = fwd_bem_new_coil_solution();
    els->user_data_free = fwd_bem_free_coil_solution;

    sol->ncoil = els->ncoil;
    sol->np    = m->nsol;
    sol->solution  = ALLOC_CMATRIX_40(sol->ncoil,sol->np);
    /*
       * Go through all coils
       */
    for (k = 0; k < els->ncoil; k++) {
        el = els->coils[k];
        one_sol = sol->solution[k];
        for (q = 0; q < m->nsol; q++)
            one_sol[q] = 0.0;
        scalp = m->surfs[0];
        /*
         * Go through all 'integration points'
         */
        for (p = 0; p < el->np; p++) {
            VEC_COPY_40(r,el->rmag[p]);
            if (m->head_mri_t != NULL)
                FiffCoordTransOld::fiff_coord_trans(r,m->head_mri_t,FIFFV_MOVE);
            best = mne_project_to_surface(scalp,NULL,r,FALSE,&dist);
            if (best < 0) {
                printf("One of the electrodes could not be projected onto the scalp surface. How come?");
                goto bad;
            }
            if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
                /*
             * Simply pick the value at the triangle
             */
                pick_sol = m->solution[best];
                for (q = 0; q < m->nsol; q++)
                    one_sol[q] += el->w[p]*pick_sol[q];
            }
            else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
                /*
             * Calculate a linear interpolation between the vertex values
             */
                tri = scalp->tris+best;
                mne_triangle_coords(r,scalp,best,&x,&y,&z);

                w[X_40] = el->w[p]*(1.0 - x - y);
                w[Y_40] = el->w[p]*x;
                w[Z_40] = el->w[p]*y;
                for (v = 0; v < 3; v++) {
                    pick_sol = m->solution[tri->vert[v]];
                    for (q = 0; q < m->nsol; q++)
                        one_sol[q] += w[v]*pick_sol[q];
                }
            }
            else {
                printf("Unknown BEM approximation method : %d\n",m->bem_method);
                goto bad;
            }
        }
    }
    return OK;

bad : {
        els->fwd_free_coil_set_user_data();
        return FAIL;
    }
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_lin_pot_calc(float *rd, float *Q, FwdBemModel *m, FwdCoilSet *els, int all_surfs, float *pot)              /* Put the result here */
/*
          * Compute the potentials due to a current dipole
          * using the linear potential approximation
          */
{
    float **rr;
    int   np;
    int   s,k,p,nsol;
    float mult,mri_rd[3],mri_Q[3];

    float *v0;
    float **solution;

    if (!m->v0)
        m->v0 = MALLOC_40(m->nsol,float);
    v0 = m->v0;

    VEC_COPY_40(mri_rd,rd);
    VEC_COPY_40(mri_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,rr[k]);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->np;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors_40(solution[k],v0,m->nsol);
    return;
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_pot_calc(float *rd, float *Q, FwdBemModel *m, FwdCoilSet *els, int all_surfs, float *pot)
/*
          * Compute the potentials due to a current dipole
          */
{
    mneTriangle tri;
    int         ntri;
    int         s,k,p,nsol;
    float       mult;
    float       *v0;
    float       **solution;
    float       mri_rd[3],mri_Q[3];

    if (!m->v0)
        m->v0 = MALLOC_40(m->nsol,float);
    v0 = m->v0;

    VEC_COPY_40(mri_rd,rd);
    VEC_COPY_40(mri_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,tri->cent);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->ntri;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors_40(solution[k],v0,m->nsol);
    return;
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_pot_els(float *rd, float *Q, FwdCoilSet *els, float *pot, void *client) /* The model */
/*
     * This version calculates the potential on all surfaces
     */
{
    fwdBemModel    m = (fwdBemModel)client;
    fwdBemSolution sol = (fwdBemSolution)els->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_pot_els");
        return FAIL;
    }
    if (!m->solution) {
        printf("No solution available for fwd_bem_pot_els");
        return FAIL;
    }
    if (!sol || sol->ncoil != els->ncoil) {
        printf("No appropriate electrode-specific data available in fwd_bem_pot_coils");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_pot_calc(rd,Q,m,els,FALSE,pot);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_pot_calc(rd,Q,m,els,FALSE,pot);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}


//*************************************************************************************************************

#define ARSINH(x) log((x) + sqrt(1.0+(x)*(x)))

void FwdBemModel::calc_f(double *xx, double *yy, double *f0, double *fx, double *fy)	        /* The weights in the linear approximation */
{
    double det = -xx[Y_40]*yy[X_40] + xx[Z_40]*yy[X_40] +
            xx[X_40]*yy[Y_40] - xx[Z_40]*yy[Y_40] - xx[X_40]*yy[Z_40] + xx[Y_40]*yy[Z_40];
    int k;

    f0[X_40] = -xx[Z_40]*yy[Y_40] + xx[Y_40]*yy[Z_40];
    f0[Y_40] = xx[Z_40]*yy[X_40] - xx[X_40]*yy[Z_40];
    f0[Z_40] = -xx[Y_40]*yy[X_40] + xx[X_40]*yy[Y_40];

    fx[X_40] =  yy[Y_40] - yy[Z_40];
    fx[Y_40] = -yy[X_40] + yy[Z_40];
    fx[Z_40] = yy[X_40] - yy[Y_40];

    fy[X_40] = -xx[Y_40] + xx[Z_40];
    fy[Y_40] = xx[X_40] - xx[Z_40];
    fy[Z_40] = -xx[X_40] + xx[Y_40];

    for (k = 0; k < 3; k++) {
        f0[k] = f0[k]/det;
        fx[k] = fx[k]/det;
        fy[k] = fy[k]/det;
    }
}


//*************************************************************************************************************

void FwdBemModel::calc_magic(double u, double z, double A, double B, double *beta, double *D)
/*
* Calculate Urankar's magic numbers
*/
{
    double B2 = 1.0 + B*B;
    double ABu = A + B*u;
    *D = sqrt(u*u + z*z + ABu*ABu);
    beta[0] = ABu/sqrt(u*u + z*z);
    beta[1] = (A*B + B2*u)/sqrt(A*A + B2*z*z);
    beta[2] = (B*z*z - A*u)/(z*(*D));
}


//*************************************************************************************************************

void FwdBemModel::field_integrals(float *from, mneTriangle to, double *I1p, double *T, double *S1, double *S2, double *f0, double *fx, double *fy)
{
    double y1[3],y2[3],y3[3];
    double xx[4],yy[4];
    double A,B,z,dx;
    double beta[3],I1,Tx,Ty,Txx,Tyy,Sxx,mult;
    double S1x,S1y,S2x,S2y;
    double D1,B2;
    int k;
    /*
       * Preliminaries...
       *
       * 1. Move origin to viewpoint...
       *
       */
    VEC_DIFF_40(from,to->r1,y1);
    VEC_DIFF_40(from,to->r2,y2);
    VEC_DIFF_40(from,to->r3,y3);
    /*
       * 2. Calculate local xy coordinates...
       */
    xx[0] = VEC_DOT_40(y1,to->ex);
    xx[1] = VEC_DOT_40(y2,to->ex);
    xx[2] = VEC_DOT_40(y3,to->ex);
    xx[3] = xx[0];

    yy[0] = VEC_DOT_40(y1,to->ey);
    yy[1] = VEC_DOT_40(y2,to->ey);
    yy[2] = VEC_DOT_40(y3,to->ey);
    yy[3] = yy[0];

    calc_f (xx,yy,f0,fx,fy);
    /*
       * 3. Distance of the plane from origin...
       */
    z = VEC_DOT_40(y1,to->nn);
    /*
       * Put together the line integral...
       * We use the convention where the local y-axis
       * is parallel to the last side and, therefore, dx = 0
       * on that side. We can thus omit the last side from this
       * computation in some cases.
       */
    I1 = 0.0;
    Tx = 0.0;
    Ty = 0.0;
    S1x = 0.0;
    S1y = 0.0;
    S2x = 0.0;
    S2y = 0.0;
    for (k = 0; k < 2; k++) {
        dx = xx[k+1] - xx[k];
        A = (yy[k]*xx[k+1] - yy[k+1]*xx[k])/dx;
        B = (yy[k+1]-yy[k])/dx;
        B2 = (1.0 + B*B);
        /*
         * Upper limit
         */
        calc_magic (xx[k+1],z,A,B,beta,&D1);
        I1 = I1 - xx[k+1]*ARSINH(beta[0]) - (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                - z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx + Txx;
        Ty = Ty + B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x + Sxx;
        S1y = S1y + B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x + Sxx;
        /*
         * Lower limit
         */
        calc_magic (xx[k],z,A,B,beta,&D1);
        I1 = I1 + xx[k]*ARSINH(beta[0]) + (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                + z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx - Txx;
        Ty = Ty - B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x - Sxx;
        S1y = S1y - B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x - Sxx;
    }
    /*
       * Handle last side (dx = 0) in a special way;
       */
    mult = 1.0/sqrt(xx[k]*xx[k]+z*z);
    /*
       * Upper...
       */
    Tyy = ARSINH(mult*yy[k+1]);
    Ty = Ty + Tyy;
    S1y = S1y + xx[k]*Tyy;
    /*
       * Lower...
       */
    Tyy = ARSINH(mult*yy[k]);
    Ty = Ty - Tyy;
    S1y = S1y - xx[k]*Tyy;
    /*
       * Set return values
       */
    *I1p = I1;
    T[X_40] = Tx;
    T[Y_40] = Ty;
    S1[X_40] = S1x;
    S1[Y_40] = S1y;
    S2[X_40] = S2x;
    S2[Y_40] = -S1x;
    return;
}


//*************************************************************************************************************

double FwdBemModel::one_field_coeff(float *dest, float *normal, mneTriangle tri)
/*
* Compute the integral over one triangle.
* This looks magical but it is not.
*/
{
    double *yy[4];
    double y1[3],y2[3],y3[3];
    double beta[3];
    double bbeta[3];
    double coeff[3];
    int   j,k;

    yy[0] = y1;
    yy[1] = y2;
    yy[2] = y3;
    yy[3] = y1;
    VEC_DIFF_40(dest,tri->r1,y1);
    VEC_DIFF_40(dest,tri->r2,y2);
    VEC_DIFF_40(dest,tri->r3,y3);
    for (j = 0; j < 3; j++)
        beta[j] = calc_beta(yy[j],yy[j+1]);
    bbeta[0] = beta[2] - beta[0];
    bbeta[1] = beta[0] - beta[1];
    bbeta[2] = beta[1] - beta[2];

    for (j = 0; j < 3; j++)
        coeff[j] = 0.0;
    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            coeff[k] = coeff[k] + yy[j][k]*bbeta[j];
    return (VEC_DOT_40(coeff,normal));
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_field_coeff(FwdBemModel *m, FwdCoilSet *coils)	/* Gradiometer coil positions */
/*
     * Compute the weighting factors to obtain the magnetic field
     */
{
    MneSurfaceOrVolume::MneCSurface*     surf;
    mneTriangle    tri;
    FwdCoil*        coil;
    FwdCoilSet*     tcoils = NULL;
    int            ntri;
    float          **coeff = NULL;
    int            j,k,p,s,off;
    double         res;
    double         mult;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_CONSTANT_COLL) {
        printf("BEM method should be constant collocation for fwd_bem_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                    * Make a transformed duplicate
                    */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    ntri  = m->nsol;
    coeff = ALLOC_CMATRIX_40(coils->ncoil,ntri);

    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                res = 0.0;
                for (p = 0; p < coil->np; p++)
                    res = res + coil->w[p]*one_field_coeff(coil->rmag[p],coil->cosmag[p],tri);
                coeff[j][k+off] = mult*res;
            }
        }
        off = off + ntri;
    }
    delete tcoils;
    return coeff;
}


//*************************************************************************************************************

double FwdBemModel::calc_gamma(double *rk, double *rk1)
{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF_40(rk,rk1,rkk1);
    size = VEC_LEN_40(rkk1);

    res = log((VEC_LEN_40(rk1)*size + VEC_DOT_40(rk1,rkk1))/
              (VEC_LEN_40(rk)*size + VEC_DOT_40(rk,rkk1)))/size;
    return (res);
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_one_lin_field_coeff_ferg(float *dest, float *dir, mneTriangle tri, double *res)	/* The results */
{
    double c[3];			/* Component of dest vector normal to
                                     * the triangle plane */
    double A[3];			/* Projection of dest onto the triangle */
    double c1[3],c2[3],c3[3];
    double y1[3],y2[3],y3[3];
    double *yy[4],*cc[4];
    double rjk[3][3];
    double cross[3],triple,l1,l2,l3,solid,clen;
    double common,sum,beta,gamma;
    int    k;

    yy[0] = y1;   cc[0] = c1;
    yy[1] = y2;   cc[1] = c2;
    yy[2] = y3;   cc[2] = c3;
    yy[3] = y1;   cc[3] = c1;

    VEC_DIFF_40(tri->r2,tri->r3,rjk[0]);
    VEC_DIFF_40(tri->r3,tri->r1,rjk[1]);
    VEC_DIFF_40(tri->r1,tri->r2,rjk[2]);

    for (k = 0; k < 3; k++) {
        y1[k] = tri->r1[k] - dest[k];
        y2[k] = tri->r2[k] - dest[k];
        y3[k] = tri->r3[k] - dest[k];
    }
    clen  = VEC_DOT_40(y1,tri->nn);
    for (k = 0; k < 3; k++) {
        c[k]  = clen*tri->nn[k];
        A[k]  = dest[k] + c[k];
        c1[k] = tri->r1[k] - A[k];
        c2[k] = tri->r2[k] - A[k];
        c3[k] = tri->r3[k] - A[k];
    }
    /*
       * beta and gamma...
       */
    for (sum = 0.0, k = 0; k < 3; k++) {
        CROSS_PRODUCT_40(cc[k],cc[k+1],cross);
        beta  = VEC_DOT_40(cross,tri->nn);
        gamma = calc_gamma (yy[k],yy[k+1]);
        sum = sum + beta*gamma;
    }
    /*
       * Solid angle...
       */
    CROSS_PRODUCT_40(y1,y2,cross);
    triple = VEC_DOT_40(cross,y3);

    l1 = VEC_LEN_40(y1);
    l2 = VEC_LEN_40(y2);
    l3 = VEC_LEN_40(y3);
    solid = 2.0*atan2(triple,
                      (l1*l2*l3+
                       VEC_DOT_40(y1,y2)*l3+
                       VEC_DOT_40(y1,y3)*l2+
                       VEC_DOT_40(y2,y3)*l1));
    /*
       * Now we are ready to assemble it all together
       */
    common = (sum-clen*solid)/(2.0*tri->area);
    for (k = 0; k < 3; k++)
        res[k] = -VEC_DOT_40(rjk[k],dir)*common;
    return;
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_one_lin_field_coeff_uran(float *dest, float *dir, mneTriangle tri, double *res)	/* The results */
{
    double      I1,T[2],S1[2],S2[2];
    double      f0[3],fx[3],fy[3];
    double      res_x,res_y;
    double      x_fac,y_fac;
    int         k;
    double      len;
    /*
       * Compute the component integrals
       */
    field_integrals (dest,tri,&I1,T,S1,S2,f0,fx,fy);
    /*
       * Compute the coefficient for each node...
       */
    len = VEC_LEN_40(dir);
    dir[X_40] = dir[X_40]/len;
    dir[Y_40] = dir[Y_40]/len;
    dir[Z_40] = dir[Z_40]/len;

    x_fac = -VEC_DOT_40(dir,tri->ex);
    y_fac = -VEC_DOT_40(dir,tri->ey);
    for (k = 0; k < 3; k++) {
        res_x = f0[k]*T[X_40] + fx[k]*S1[X_40] + fy[k]*S2[X_40] + fy[k]*I1;
        res_y = f0[k]*T[Y_40] + fx[k]*S1[Y_40] + fy[k]*S2[Y_40] - fx[k]*I1;
        res[k] = x_fac*res_x + y_fac*res_y;
    }
    return;
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_one_lin_field_coeff_simple(float *dest, float *normal, mneTriangle source, double *res)     /* The result for each triangle node */
/*
          * Simple version...
          */
{
    float diff[3];
    float vec_result[3];
    float dl;
    int   k;
    float *rr[3];


    rr[0] = source->r1;
    rr[1] = source->r2;
    rr[2] = source->r3;

    for (k = 0; k < 3; k++) {
        VEC_DIFF_40(rr[k],dest,diff);
        dl = VEC_DOT_40(diff,diff);
        CROSS_PRODUCT_40(diff,source->nn,vec_result);
        res[k] = source->area*VEC_DOT_40(vec_result,normal)/(3.0*dl*sqrt(dl));
    }
    return;
}


//*************************************************************************************************************

float **FwdBemModel::fwd_bem_lin_field_coeff(FwdBemModel *m, FwdCoilSet *coils, int method)    /* Which integration formula to use */
/*
          * Compute the weighting factors to obtain the magnetic field
          * in the linear potential approximation
          */
{
    MneSurfaceOrVolume::MneCSurface*  surf;
    mneTriangle tri;
    FwdCoil*     coil;
    FwdCoilSet*  tcoils = NULL;
    int         ntri;
    float       **coeff  = NULL;
    int         j,k,p,pp,off,s;
    double      res[3],one[3];
    float       mult;
    linFieldIntFunc func;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_LINEAR_COLL) {
        printf("BEM method should be linear collocation for fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_lin_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                    * Make a transformed duplicate
                    */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    if (method == FWD_BEM_LIN_FIELD_FERGUSON)
        func = fwd_bem_one_lin_field_coeff_ferg;
    else if (method == FWD_BEM_LIN_FIELD_URANKAR)
        func = fwd_bem_one_lin_field_coeff_uran;
    else
        func = fwd_bem_one_lin_field_coeff_simple;

    coeff = ALLOC_CMATRIX_40(coils->ncoil,m->nsol);
    for (k = 0; k < m->nsol; k++)
        for (j = 0; j < coils->ncoil; j++)
            coeff[j][k] = 0.0;
    /*
       * Process each of the surfaces
       */
    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                for (pp = 0; pp < 3; pp++)
                    res[pp] = 0;
                /*
             * Accumulate the coefficients for each triangle node...
             */
                for (p = 0; p < coil->np; p++) {
                    func(coil->rmag[p],coil->cosmag[p],tri,one);
                    for (pp = 0; pp < 3; pp++)
                        res[pp] = res[pp] + coil->w[p]*one[pp];
                }
                /*
             * Add these to the corresponding coefficient matrix
             * elements...
             */
                for (pp = 0; pp < 3; pp++)
                    coeff[j][tri->vert[pp]+off] = coeff[j][tri->vert[pp]+off] + mult*res[pp];
            }
        }
        off = off + surf->np;
    }
    /*
       * Discard the duplicate
       */
    delete tcoils;
    return (coeff);
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_specify_coils(FwdBemModel *m, FwdCoilSet *coils)
/*
     * Set up for computing the solution at a set of coils
      */
{
    float **sol = NULL;
    fwdBemSolution csol;

    if (!m) {
        printf("Model missing in fwd_bem_specify_coils");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_coils");
        goto bad;
    }
    if(coils)
        coils->fwd_free_coil_set_user_data();
    if (!coils || coils->ncoil == 0)
        return OK;
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        sol = fwd_bem_field_coeff(m,coils);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        sol = fwd_bem_lin_field_coeff(m,coils,FWD_BEM_LIN_FIELD_SIMPLE);
    else {
        printf("Unknown BEM method in fwd_bem_specify_coils : %d",m->bem_method);
        goto bad;
    }
    coils->user_data = csol = fwd_bem_new_coil_solution();
    coils->user_data_free   = fwd_bem_free_coil_solution;

    csol->ncoil     = coils->ncoil;
    csol->np        = m->nsol;
    csol->solution  = mne_mat_mat_mult_3(sol,m->solution,coils->ncoil,m->nsol,m->nsol);

    FREE_CMATRIX_40(sol);
    return OK;

bad : {
        FREE_CMATRIX_40(sol);
        return FAIL;

    }
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_lin_field_calc(float *rd, float *Q, FwdCoilSet *coils, FwdBemModel *m, float *B)
/*
     * Calculate the magnetic field in a set of coils
     */
{
    float *v0;
    int   s,k,p,np;
    FwdCoil* coil;
    float  mult;
    float  **rr;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
       * Infinite-medium potentials
       */
    if (!m->v0)
        m->v0 = MALLOC_40(m->nsol,float);
    v0 = m->v0;
    /*
       * The dipole location and orientation must be transformed
       */
    VEC_COPY_40(my_rd,rd);
    VEC_COPY_40(my_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
       * Compute the inifinite-medium potentials at the vertices
       */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,rr[k]);
    }
    /*
       * Primary current contribution
       * (can be calculated in the coil/dipole coordinates)
       */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
       * Volume current contribution
       */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors_40(sol->solution[k],v0,m->nsol);
    /*
       * Scale correctly
       */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}


//*************************************************************************************************************

void FwdBemModel::fwd_bem_field_calc(float *rd, float *Q, FwdCoilSet *coils, FwdBemModel *m, float *B)
/*
     * Calculate the magnetic field in a set of coils
     */
{
    float *v0;
    int   s,k,p,ntri;
    FwdCoil* coil;
    mneTriangle tri;
    float   mult;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
       * Infinite-medium potentials
       */
    if (!m->v0)
        m->v0 = MALLOC_40(m->nsol,float);
    v0 = m->v0;
    /*
       * The dipole location and orientation must be transformed
       */
    VEC_COPY_40(my_rd,rd);
    VEC_COPY_40(my_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
       * Compute the inifinite-medium potentials at the centers of the triangles
       */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,tri->cent);
    }
    /*
       * Primary current contribution
       * (can be calculated in the coil/dipole coordinates)
       */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
       * Volume current contribution
       */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors_40(sol->solution[k],v0,m->nsol);
    /*
       * Scale correctly
       */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}


//*************************************************************************************************************

int FwdBemModel::fwd_bem_field(float *rd, float *Q, FwdCoilSet *coils, float *B, void *client)  /* The model */
/*
     * This version calculates the magnetic field in a set of coils
     * Call fwd_bem_specify_coils first to establish the coil-specific
     * solution matrix
     */
{
    FwdBemModel* m = (FwdBemModel*)client;
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_field");
        return FAIL;
    }
    if (!sol || !sol->solution || sol->ncoil != coils->ncoil) {
        printf("No appropriate coil-specific data available in fwd_bem_field");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_field_calc(rd,Q,coils,m,B);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_field_calc(rd,Q,coils,m,B);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}
