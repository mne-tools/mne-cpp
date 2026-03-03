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
#include <fiff/fiff_sparse_matrix.h>

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

//============================= mne_decompose.c =============================

int mne_decompose_eigen (const VectorXd& mat,
                         VectorXd& lambda,
                         Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& vectors,
                         int    dim)
/*
      * Compute the eigenvalue decomposition of
      * a symmetric matrix using the LAPACK routines
      *
      * 'mat' contains the lower triangle of the matrix
      */
{
    int    np  =   dim*(dim+1)/2;
    int    maxi;
    double scale;

// idamax workaround begin
    maxi = 0;
    for(int i = 0; i < np; ++i)
        if (std::fabs(mat[i]) > std::fabs(mat[maxi]))
            maxi = i;
// idamax workaround end

    scale = 1.0/mat[maxi];

// dspev workaround begin
    MatrixXd dmat_full = MatrixXd::Zero(dim,dim);
    int idx = 0;
    for (int i = 0; i < dim; ++i) {
        for(int j = 0; j <= i; ++j) {
            double val = mat[idx]*scale;
            dmat_full(i,j) = val;
            dmat_full(j,i) = val;
            ++idx;
        }
    }
    SelfAdjointEigenSolver<MatrixXd> es;
    es.compute(dmat_full);
// dspev workaround end

    scale = 1.0/scale;
    lambda = es.eigenvalues() * scale;
    vectors = es.eigenvectors().transpose().cast<float>();

    return 0;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneCovMatrix::MneCovMatrix(int p_kind,
                           int p_ncov,
                           const QStringList& p_names,
                           const VectorXd& p_cov,
                           const VectorXd& p_cov_diag,
                           FiffSparseMatrix* p_cov_sparse)
:kind(p_kind)
,ncov(p_ncov)
,nfree(1)
,nproj(0)
,nzero(0)
,names(p_names)
,cov(p_cov)
,cov_diag(p_cov_diag)
,cov_sparse(p_cov_sparse)
,proj(nullptr)
,sss(nullptr)
,nbad(0)
{
}

//=============================================================================================================

MneCovMatrix::~MneCovMatrix()
{
}

//=============================================================================================================

std::unique_ptr<MneCovMatrix> MneCovMatrix::dup() const
{
    auto res = cov_diag.size() > 0
        ? create(kind,ncov,names,VectorXd(),VectorXd(cov_diag))
        : create(kind,ncov,names,VectorXd(cov),VectorXd());
    /*
        * Duplicate additional items
        */
    if (ch_class.size() > 0) {
        res->ch_class = ch_class;
    }
    res->bads = bads;
    res->nbad = nbad;
    res->proj.reset(proj ? proj->dup() : nullptr);
    if (sss)
        res->sss = std::make_unique<MneSssData>(*sss);

    return res;
}

//=============================================================================================================

int MneCovMatrix::is_diag() const
{
    return cov_diag.size() > 0;
}

//=============================================================================================================

int MneCovMatrix::add_inv()
/*
          * Calculate the inverse square roots for whitening
          */
{
    const VectorXd& src = lambda.size() > 0 ? lambda : cov_diag;
    int k;

    if (src.size() == 0) {
        qCritical("Covariance matrix is not diagonal or not decomposed.");
        return FAIL;
    }
    inv_lambda.resize(ncov);
    for (k = 0; k < ncov; k++) {
        if (src[k] <= 0.0)
            inv_lambda[k] = 0.0;
        else
            inv_lambda[k] = 1.0/sqrt(src[k]);
    }
    return OK;
}

//=============================================================================================================

int MneCovMatrix::condition(float rank_threshold, int use_rank)
{
    VectorXd scale_vec;
    VectorXd cov_local;
    VectorXd lambda_local;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> local_eigen;
    MatrixXd data1;
    double magscale,gradscale,eegscale;
    int    nmag,ngrad,neeg,nok;
    int    j,k;
    int    res = FAIL;

    if (cov_diag.size() > 0)
        return OK;
    if (ch_class.size() == 0) {
        qCritical("Channels not classified. Rank cannot be determined.");
        return FAIL;
    }
    magscale = gradscale = eegscale = 0.0;
    nmag = ngrad = neeg = 0;
    for (k = 0; k < ncov; k++) {
        if (ch_class[k] == MNE_COV_CH_MEG_MAG) {
            magscale += this->cov[lt_packed_index(k,k)]; nmag++;
        }
        else if (ch_class[k] == MNE_COV_CH_MEG_GRAD) {
            gradscale += this->cov[lt_packed_index(k,k)]; ngrad++;
        }
        else if (ch_class[k] == MNE_COV_CH_EEG) {
            eegscale += this->cov[lt_packed_index(k,k)]; neeg++;
        }
#ifdef DEBUG
        fprintf(stdout,"%d ",ch_class[k]);
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
    scale_vec.resize(ncov);
    for (k = 0; k < ncov; k++) {
        if (ch_class[k] == MNE_COV_CH_MEG_MAG)
            scale_vec[k] = magscale;
        else if (ch_class[k] == MNE_COV_CH_MEG_GRAD)
            scale_vec[k] = gradscale;
        else if (ch_class[k] == MNE_COV_CH_EEG)
            scale_vec[k] = eegscale;
        else
            scale_vec[k] = 1.0;
    }
    cov_local.resize(ncov*(ncov+1)/2);
    lambda_local.resize(ncov);
    local_eigen.resize(ncov,ncov);
    for (j = 0; j < ncov; j++)
        for (k = 0; k <= j; k++)
            cov_local[lt_packed_index(j,k)] = this->cov[lt_packed_index(j,k)]*scale_vec[j]*scale_vec[k];
    if (mne_decompose_eigen(cov_local,lambda_local,local_eigen,ncov) == 0) {
#ifdef DEBUG
        for (k = 0; k < ncov; k++)
            fprintf(stdout,"%g ",lambda_local[k]/lambda_local[ncov-1]);
        fprintf(stdout,"\n");
#endif
        nok = 0;
        for (k = ncov-1; k >= 0; k--) {
            if (lambda_local[k] >= rank_threshold*lambda_local[ncov-1])
                nok++;
            else
                break;
        }
        printf("\n\tEstimated covariance matrix rank = %d (%g)\n",nok,lambda_local[ncov-nok]/lambda_local[ncov-1]);
        if (use_rank > 0 && use_rank < nok) {
            nok = use_rank;
            printf("\tUser-selected covariance matrix rank = %d (%g)\n",nok,lambda_local[ncov-nok]/lambda_local[ncov-1]);
        }
        /*
         * Put it back together
         */
        for (j = 0; j < ncov-nok; j++)
            lambda_local[j] = 0.0;
        data1.resize(ncov,ncov);
        for (j = 0; j < ncov; j++) {
#ifdef DEBUG
            mne_print_vector(stdout,NULL,local_eigen.row(j).data(),ncov);
#endif
            for (k = 0; k < ncov; k++)
                data1(j,k) = sqrt(lambda_local[j])*local_eigen(j,k);
        }
        MatrixXd data2 = data1.transpose() * data1;
#ifdef DEBUG
        printf(">>>\n");
        for (j = 0; j < ncov; j++)
            mne_print_dvector(stdout,NULL,data2.row(j).data(),ncov);
        printf(">>>\n");
#endif
        /*
         * Scale back
         */
        for (k = 0; k < ncov; k++)
            if (scale_vec[k] > 0.0)
                scale_vec[k] = 1.0/scale_vec[k];
        for (j = 0; j < ncov; j++)
            for (k = 0; k <= j; k++)
                if (this->cov[lt_packed_index(j,k)] != 0.0)
                    this->cov[lt_packed_index(j,k)] = scale_vec[j]*scale_vec[k]*data2(j,k);
        res = nok;
    }
    return res;
}

//=============================================================================================================

int MneCovMatrix::decompose_eigen_small(float p_small, int use_rank)
/*
          * Do the eigenvalue decomposition
          */
{
    int   k,p,rank;
    float rank_threshold = 1e-6;

    if (p_small < 0)
        p_small = 1.0;

    if (cov_diag.size() > 0)
        return add_inv();
    if (lambda.size() > 0 && eigen.size() > 0) {
        printf("\n\tEigenvalue decomposition had been precomputed.\n");
        nzero = 0;
        for (k = 0; k < ncov; k++, nzero++)
            if (lambda[k] > 0)
                break;
    }
    else {
        lambda.resize(0);
        eigen.resize(0,0);

        if ((rank = condition(rank_threshold,use_rank)) < 0)
            return FAIL;

        lambda.resize(ncov);
        eigen.resize(ncov,ncov);
        if (mne_decompose_eigen (cov,lambda,eigen,ncov) != 0)
            goto bad;
        nzero = ncov - rank;
        for (k = 0; k < nzero; k++)
            lambda[k] = 0.0;
        /*
         * Find which eigenvectors correspond to EEG/MEG
         */
        {
            float meglike,eeglike;
            int   nmeg,neeg;

            nmeg = neeg = 0;
            for (k = nzero; k < ncov; k++) {
                meglike = eeglike = 0.0;
                for (p = 0; p < ncov; p++)  {
                    if (ch_class[p] == MNE_COV_CH_EEG)
                        eeglike += std::fabs(eigen(k,p));
                    else if (ch_class[p] == MNE_COV_CH_MEG_MAG || ch_class[p] == MNE_COV_CH_MEG_GRAD)
                        meglike += std::fabs(eigen(k,p));
                }
                if (meglike > eeglike)
                    nmeg++;
                else
                    neeg++;
            }
            printf("\t%d MEG and %d EEG-like channels remain in the whitened data\n",nmeg,neeg);
        }
    }
    return add_inv();

bad : {
        lambda.resize(0);
        eigen.resize(0,0);
        return FAIL;
    }
}

//=============================================================================================================

int MneCovMatrix::decompose_eigen()

{
    return decompose_eigen_small(-1.0,-1);
}

//=============================================================================================================

int MneCovMatrix::lt_packed_index(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}
