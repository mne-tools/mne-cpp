//=============================================================================================================
/**
 * @file     mne_cov_matrix.cpp
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
 * @brief    Definition of the MneCovMatrix Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_cov_matrix.h"
#include "mne_sss_data.h"
#include "mne_proj_item.h"
#include "mne_proj_op.h"

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#define MALLOC_30(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_30(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_30(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_30(m) mne_free_cmatrix_30((m))
#define FREE_DCMATRIX_30(m) mne_free_dcmatrix_30((m))

void mne_free_cmatrix_30 (float **m)
{
    if (m) {
        FREE_30(*m);
        FREE_30(m);
    }
}

void mne_free_dcmatrix_30 (double **m)

{
    if (m) {
        FREE_30(*m);
        FREE_30(m);
    }
}

#define ALLOC_CMATRIX_30(x,y) mne_cmatrix_30((x),(y))

#define ALLOC_DCMATRIX_30(x,y) mne_dmatrix_30((x),(y))

static void matrix_error_30(int kind, int nr, int nc)

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

float **mne_cmatrix_30(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_30(nr,float *);
    if (!m) matrix_error_30(1,nr,nc);
    whole = MALLOC_30(nr*nc,float);
    if (!whole) matrix_error_30(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

double **mne_dmatrix_30(int nr, int nc)

{
    int i;
    double **m;
    double *whole;

    m = MALLOC_30(nr,double *);
    if (!m) matrix_error_30(1,nr,nc);
    whole = MALLOC_30(nr*nc,double);
    if (!whole) matrix_error_30(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

//============================= mne_decompose.c =============================

int mne_decompose_eigen (double *mat,
                         double *lambda,
                         float  **vectors, /* Eigenvectors fit into floats easily */
                         int    dim)
/*
      * Compute the eigenvalue decomposition of
      * a symmetric matrix using the LAPACK routines
      *
      * 'mat' contains the lower triangle of the matrix
      */
{
    int    np  =   dim*(dim+1)/2;
    double *w    = MALLOC_30(dim,double);
    double *z    = MALLOC_30(dim*dim,double);
    double *work = MALLOC_30(3*dim,double);
    double *dmat = MALLOC_30(np,double);
    float  *vecp = vectors[0];

//    const char   *uplo  = "U";
//    const char   *compz = "V";
    int    info,k;
//    int    one = 1;
    int    maxi;
    double scale;

//    maxi = idamax(&np,mat,&one);
// idamax workaround begin
    maxi = 0;
    for(int i = 0; i < np; ++i)
        if (std::fabs(mat[i]) > std::fabs(mat[maxi]))
            maxi = i;
// idamax workaround end

    scale = 1.0/mat[maxi];//scale = 1.0/mat[maxi-1];

    for (k = 0; k < np; k++)
        dmat[k] = mat[k]*scale;
//    dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);

// dspev workaround begin
    MatrixXd dmat_tmp = MatrixXd::Zero(dim,dim);
    int idx = 0;
    for (int i = 0; i < dim; ++i) {
        for(int j = 0; j <= i; ++j) {
            dmat_tmp(i,j) = dmat[idx];
            dmat_tmp(j,i) = dmat[idx];
            ++idx;
        }
    }
    SelfAdjointEigenSolver<MatrixXd> es;
    es.compute(dmat_tmp);
    for ( int i = 0; i < dim; ++i )
        w[i] = es.eigenvalues()[i];

    idx = 0;
    for ( int j = 0; j < dim; ++j ) {
        for( int i = 0; i < dim; ++i ) {
            z[idx] = es.eigenvectors()(i,j);// Column Major
            ++idx;
        }
    }
// dspev workaround end

    info = 0;

    qDebug() << "!!!DEBUG ToDo: dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);";

    FREE_30(work);
    if (info != 0)
        printf("Eigenvalue decomposition failed (LAPACK info = %d)",info);
    else {
        scale = 1.0/scale;
        for (k = 0; k < dim; k++)
            lambda[k] = scale*w[k];
        for (k = 0; k < dim*dim; k++)
            vecp[k] = z[k];
    }
    FREE_30(w);
    FREE_30(z);
    FREE_30(dmat);
    if (info == 0)
        return 0;
    else
        return -1;
}

double **mne_dmatt_dmat_mult2 (double **m1,double **m2, int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d2 x d1)^T * m2(d2 x d3) */

{
    double **result = ALLOC_DCMATRIX_30(d1,d3);
#ifdef BLAS
    char  *transa = "N";
    char  *transb = "T";
    double zero = 0.0;
    double one  = 1.0;

    dgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d1,&zero,result[0],&d3);

    return result;
#else
    int j,k,p;
    double sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
            for (p = 0; p < d2; p++)
                sum = sum + m1[p][j]*m2[p][k];
            result[j][k] = sum;
        }
    return result;
#endif
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneCovMatrix::MneCovMatrix(int p_kind,
                           int p_ncov,
                           const QStringList& p_names,
                           double *p_cov,
                           double *p_cov_diag,
                           FiffSparseMatrix* p_cov_sparse)
:kind(p_kind)
,ncov(p_ncov)
,nproj(0)
,nzero(0)
,names(p_names)
,cov(p_cov)
,cov_diag(p_cov_diag)
,cov_sparse(p_cov_sparse)
,eigen(NULL)
,lambda(NULL)
,chol(NULL)
,inv_lambda(NULL)
,nfree(1)
,ch_class(NULL)
,proj(NULL)
,sss(NULL)
,nbad(0)
{
}

//=============================================================================================================

MneCovMatrix::~MneCovMatrix()
{
    FREE_30(cov);
    FREE_30(cov_diag);
    if(cov_sparse)
        delete cov_sparse;
    names.clear();
    FREE_CMATRIX_30(eigen);
    FREE_30(lambda);
    FREE_30(inv_lambda);
    FREE_30(chol);
    FREE_30(ch_class);
    if(proj)
        delete proj;
    if (sss)
        delete sss;
    bads.clear();
}

//=============================================================================================================

MneCovMatrix *MneCovMatrix::mne_dup_cov(MneCovMatrix *c)
{
    double       *vals;
    int          nval;
    int          k;
    MneCovMatrix* res;

    if (c->cov_diag)
        nval = c->ncov;
    else
        nval = (c->ncov*(c->ncov+1))/2;

    vals = MALLOC_30(nval,double);
    if (c->cov_diag) {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov_diag[k];
        res = mne_new_cov(c->kind,c->ncov,c->names,NULL,vals);
    }
    else {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov[k];
        res = mne_new_cov(c->kind,c->ncov,c->names,vals,NULL);
    }
    /*
        * Duplicate additional items
        */
    if (c->ch_class) {
        res->ch_class = MALLOC_30(c->ncov,int);
        for (k = 0; k < c->ncov; k++)
            res->ch_class[k] = c->ch_class[k];
    }
    res->bads = c->bads;
    res->nbad = c->nbad;
    res->proj = MneProjOp::mne_dup_proj_op(c->proj);
    res->sss  = new MneSssData(*(c->sss));

    return res;
}

//=============================================================================================================

int MneCovMatrix::mne_is_diag_cov(MneCovMatrix *c)
{
    return c->cov_diag != NULL;
}

//=============================================================================================================

int MneCovMatrix::mne_add_inv_cov(MneCovMatrix *c)
/*
          * Calculate the inverse square roots for whitening
          */
{
    double *src = c->lambda ? c->lambda : c->cov_diag;
    int k;

    if (src == NULL) {
        qCritical("Covariance matrix is not diagonal or not decomposed.");
        return FAIL;
    }
    c->inv_lambda = REALLOC_30(c->inv_lambda,c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (src[k] <= 0.0)
            c->inv_lambda[k] = 0.0;
        else
            c->inv_lambda[k] = 1.0/sqrt(src[k]);
    }
    return OK;
}

//=============================================================================================================

int MneCovMatrix::condition_cov(MneCovMatrix *c, float rank_threshold, int use_rank)
{
    double *scale  = NULL;
    double *cov    = NULL;
    double *lambda = NULL;
    float  **eigen = NULL;
    double **data1 = NULL;
    double **data2 = NULL;
    double magscale,gradscale,eegscale;
    int    nmag,ngrad,neeg,nok;
    int    j,k;
    int    res = FAIL;

    if (c->cov_diag)
        return OK;
    if (!c->ch_class) {
        qCritical("Channels not classified. Rank cannot be determined.");
        return FAIL;
    }
    magscale = gradscale = eegscale = 0.0;
    nmag = ngrad = neeg = 0;
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG) {
            magscale += c->cov[mne_lt_packed_index(k,k)]; nmag++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD) {
            gradscale += c->cov[mne_lt_packed_index(k,k)]; ngrad++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_EEG) {
            eegscale += c->cov[mne_lt_packed_index(k,k)]; neeg++;
        }
#ifdef DEBUG
        fprintf(stdout,"%d ",c->ch_class[k]);
#endif
    }
#ifdef DEBUG
    fprintf(stdout,"\n");
#endif
    if (nmag > 0)
        magscale = magscale > 0.0 ? sqrt(nmag/magscale) : 0.0;
    if (ngrad > 0)
        gradscale = gradscale > 0.0 ? sqrt(ngrad/gradscale) : 0.0;
    if (neeg > 0)
        eegscale = eegscale > 0.0 ? sqrt(neeg/eegscale) : 0.0;
#ifdef DEBUG
    fprintf(stdout,"%d %g\n",nmag,magscale);
    fprintf(stdout,"%d %g\n",ngrad,gradscale);
    fprintf(stdout,"%d %g\n",neeg,eegscale);
#endif
    scale = MALLOC_30(c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG)
            scale[k] = magscale;
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD)
            scale[k] = gradscale;
        else if (c->ch_class[k] == MNE_COV_CH_EEG)
            scale[k] = eegscale;
        else
            scale[k] = 1.0;
    }
    cov    = MALLOC_30(c->ncov*(c->ncov+1)/2.0,double);
    lambda = MALLOC_30(c->ncov,double);
    eigen  = ALLOC_CMATRIX_30(c->ncov,c->ncov);
    for (j = 0; j < c->ncov; j++)
        for (k = 0; k <= j; k++)
            cov[mne_lt_packed_index(j,k)] = c->cov[mne_lt_packed_index(j,k)]*scale[j]*scale[k];
    if (mne_decompose_eigen(cov,lambda,eigen,c->ncov) == 0) {
#ifdef DEBUG
        for (k = 0; k < c->ncov; k++)
            fprintf(stdout,"%g ",lambda[k]/lambda[c->ncov-1]);
        fprintf(stdout,"\n");
#endif
        nok = 0;
        for (k = c->ncov-1; k >= 0; k--) {
            if (lambda[k] >= rank_threshold*lambda[c->ncov-1])
                nok++;
            else
                break;
        }
        printf("\n\tEstimated covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        if (use_rank > 0 && use_rank < nok) {
            nok = use_rank;
            fprintf(stderr,"\tUser-selected covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        }
        /*
         * Put it back together
         */
        for (j = 0; j < c->ncov-nok; j++)
            lambda[j] = 0.0;
        data1 = ALLOC_DCMATRIX_30(c->ncov,c->ncov);
        for (j = 0; j < c->ncov; j++) {
#ifdef DEBUG
            mne_print_vector(stdout,NULL,eigen[j],c->ncov);
#endif
            for (k = 0; k < c->ncov; k++)
                data1[j][k] = sqrt(lambda[j])*eigen[j][k];
        }
        data2 = mne_dmatt_dmat_mult2(data1,data1,c->ncov,c->ncov,c->ncov);
#ifdef DEBUG
        printf(">>>\n");
        for (j = 0; j < c->ncov; j++)
            mne_print_dvector(stdout,NULL,data2[j],c->ncov);
        printf(">>>\n");
#endif
        /*
         * Scale back
         */
        for (k = 0; k < c->ncov; k++)
            if (scale[k] > 0.0)
                scale[k] = 1.0/scale[k];
        for (j = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                if (c->cov[mne_lt_packed_index(j,k)] != 0.0)
                    c->cov[mne_lt_packed_index(j,k)] = scale[j]*scale[k]*data2[j][k];
        res = nok;
    }
    FREE_30(cov);
    FREE_30(lambda);
    FREE_CMATRIX_30(eigen);
    FREE_DCMATRIX_30(data1);
    FREE_DCMATRIX_30(data2);
    return res;
}

//=============================================================================================================

int MneCovMatrix::mne_decompose_eigen_cov_small(MneCovMatrix *c, float p_small, int use_rank)
/*
          * Do the eigenvalue decomposition
          */
{
    int   np,k,p,rank;
    float rank_threshold = 1e-6;

    if (p_small < 0)
        p_small = 1.0;

    if (!c)
        return OK;
    if (c->cov_diag)
        return mne_add_inv_cov(c);
    if (c->lambda && c->eigen) {
        fprintf(stderr,"\n\tEigenvalue decomposition had been precomputed.\n");
        c->nzero = 0;
        for (k = 0; k < c->ncov; k++, c->nzero++)
            if (c->lambda[k] > 0)
                break;
    }
    else {
        FREE_30(c->lambda); c->lambda = NULL;
        FREE_CMATRIX_30(c->eigen); c->eigen = NULL;

        if ((rank = condition_cov(c,rank_threshold,use_rank)) < 0)
            return FAIL;

        np = c->ncov*(c->ncov+1)/2;
        c->lambda = MALLOC_30(c->ncov,double);
        c->eigen  = ALLOC_CMATRIX_30(c->ncov,c->ncov);
        if (mne_decompose_eigen (c->cov,c->lambda,c->eigen,c->ncov) != 0)
            goto bad;
        c->nzero = c->ncov - rank;
        for (k = 0; k < c->nzero; k++)
            c->lambda[k] = 0.0;
        /*
         * Find which eigenvectors correspond to EEG/MEG
         */
        {
            float meglike,eeglike;
            int   nmeg,neeg;

            nmeg = neeg = 0;
            for (k = c->nzero; k < c->ncov; k++) {
                meglike = eeglike = 0.0;
                for (p = 0; p < c->ncov; p++)  {
                    if (c->ch_class[p] == MNE_COV_CH_EEG)
                        eeglike += std::fabs(c->eigen[k][p]);
                    else if (c->ch_class[p] == MNE_COV_CH_MEG_MAG || c->ch_class[p] == MNE_COV_CH_MEG_GRAD)
                        meglike += std::fabs(c->eigen[k][p]);
                }
                if (meglike > eeglike)
                    nmeg++;
                else
                    neeg++;
            }
            printf("\t%d MEG and %d EEG-like channels remain in the whitened data\n",nmeg,neeg);
        }
    }
    return mne_add_inv_cov(c);

bad : {
        FREE_30(c->lambda); c->lambda = NULL;
        FREE_CMATRIX_30(c->eigen); c->eigen = NULL;
        return FAIL;
    }
}

//=============================================================================================================

int MneCovMatrix::mne_decompose_eigen_cov(MneCovMatrix *c)

{
    return mne_decompose_eigen_cov_small(c,-1.0,-1);
}

//=============================================================================================================

int MneCovMatrix::mne_lt_packed_index(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}
