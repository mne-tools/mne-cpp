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
 * @brief    Definition of the MNECovMatrix Class.
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
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <QFile>

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

MNECovMatrix::MNECovMatrix(int p_kind,
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

MNECovMatrix::~MNECovMatrix()
{
}

//=============================================================================================================

std::unique_ptr<MNECovMatrix> MNECovMatrix::read(const QString& name, int kind)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffTag::UPtr t_pTag;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr covnode;

    QStringList     names;
    int             nnames     = 0;
    Eigen::VectorXd cov;
    Eigen::VectorXd cov_diag;
    FiffSparseMatrix* cov_sparse = nullptr;
    Eigen::VectorXd lambda;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> eigen;
    MatrixXf        tmp_eigen;
    QStringList     bads;
    int             nbad       = 0;
    int             ncov       = 0;
    int             nfree      = 1;
    std::unique_ptr<MNECovMatrix> res;

    int            k,p,nn;
    float          *f;
    double         *d;
    std::unique_ptr<MNEProjOp> op;
    std::unique_ptr<MNESssData> sss;

    if (!stream->open())
        return nullptr;

    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_COV);

    if (nodes.size() == 0) {
        qWarning("No covariance matrix available in %s", name.toUtf8().data());
        stream->close();
        return nullptr;
    }
    /*
     * Locate the desired matrix
     */
    for (k = 0; k < nodes.size(); ++k) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_KIND, t_pTag))
            continue;

        if (*t_pTag->toInt() == kind) {
            covnode = nodes[k];
            break;
        }
    }
    if (covnode->isEmpty()) {
        qWarning("Desired covariance matrix not found from %s", name.toUtf8().data());
        stream->close();
        return nullptr;
    }
    /*
     * Read the data
     */
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIM, t_pTag)) {
        stream->close();
        return nullptr;
    }
    ncov = *t_pTag->toInt();

    if (nodes[k]->find_tag(stream, FIFF_MNE_COV_NFREE, t_pTag)) {
        nfree = *t_pTag->toInt();
    }
    if (covnode->find_tag(stream, FIFF_MNE_ROW_NAMES, t_pTag)) {
        names = FiffStream::split_name_list(t_pTag->toString());
        nnames = names.size();
        if (nnames != ncov) {
            qCritical("Incorrect number of channel names for a covariance matrix");
            stream->close();
            return nullptr;
        }
    }
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV, t_pTag)) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIAG, t_pTag)) {
            stream->close();
            return nullptr;
        } else {
            if (t_pTag->getType() == FIFFT_DOUBLE) {
                cov_diag.resize(ncov);
                d = t_pTag->toDouble();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = d[p];
                /*
                 * Check for all-zero data
                 */
                if (cov_diag.sum() == 0.0) {
                    qCritical("Sum of covariance matrix elements is zero!");
                    stream->close();
                    return nullptr;
                }
            } else if (t_pTag->getType() == FIFFT_FLOAT) {
                cov_diag.resize(ncov);
                f = t_pTag->toFloat();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = f[p];
            } else {
                qWarning("Illegal data type for covariance matrix");
                stream->close();
                return nullptr;
            }
        }
    } else {
        nn = ncov * (ncov + 1) / 2;
        if (t_pTag->getType() == FIFFT_DOUBLE) {
            cov.resize(nn);
            d = t_pTag->toDouble();
            for (p = 0; p < nn; p++)
                cov[p] = d[p];
            if (cov.sum() == 0.0) {
                qCritical("Sum of covariance matrix elements is zero!");
                stream->close();
                return nullptr;
            }
        } else if (t_pTag->getType() == FIFFT_FLOAT) {
            cov.resize(nn);
            f = t_pTag->toFloat();
            for (p = 0; p < nn; p++)
                cov[p] = f[p];
        } else {
            auto cov_sparse_uptr = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag);
            if (!cov_sparse_uptr) {
                stream->close();
                return nullptr;
            }
            cov_sparse = cov_sparse_uptr.release();
        }

        if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVALUES, t_pTag)) {
            const double *lambda_data = static_cast<const double *>(t_pTag->toDouble());
            lambda = Eigen::Map<const Eigen::VectorXd>(lambda_data, ncov);
            if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVECTORS, t_pTag)) {
                stream->close();
                delete cov_sparse;
                return nullptr;
            }

            tmp_eigen = t_pTag->toFloatMatrix().transpose();
            eigen.resize(tmp_eigen.rows(), tmp_eigen.cols());
            for (int r = 0; r < tmp_eigen.rows(); ++r)
                for (int c = 0; c < tmp_eigen.cols(); ++c)
                    eigen(r, c) = tmp_eigen(r, c);
        }
        /*
         * Read the optional projection operator
         */
        op.reset(MNEProjOp::read_from_node(stream, nodes[k]));
        if (!op) {
            stream->close();
            delete cov_sparse;
            return nullptr;
        }
        /*
         * Read the optional SSS data
         */
        sss.reset(MNESssData::read_from_node(stream, nodes[k]));
        if (!sss) {
            stream->close();
            delete cov_sparse;
            return nullptr;
        }
        /*
         * Read the optional bad channel list
         */
        bads = stream->read_bad_channels(nodes[k]);
        nbad = bads.size();
    }
    if (cov_sparse)
        res = create_sparse(kind, ncov, names, cov_sparse);
    else if (cov.size() > 0)
        res = create_dense(kind, ncov, names, cov);
    else if (cov_diag.size() > 0)
        res = create_diag(kind, ncov, names, cov_diag);
    else {
        qCritical("MNECovMatrix::read : covariance matrix data is not defined.");
        stream->close();
        return nullptr;
    }
    cov_sparse = nullptr;
    res->eigen  = std::move(eigen);
    res->lambda = std::move(lambda);
    res->nfree  = nfree;
    res->bads   = bads;
    res->nbad   = nbad;
    /*
     * Count the non-zero eigenvalues
     */
    if (res->lambda.size() > 0) {
        res->nzero = 0;
        for (k = 0; k < res->ncov; k++, res->nzero++)
            if (res->lambda[k] > 0)
                break;
    }

    if (op && op->nitems > 0) {
        res->proj = std::move(op);
    }
    if (sss && sss->comp_info.size() > 0 && sss->job != FIFFV_SSS_JOB_NOTHING) {
        res->sss = std::move(sss);
    }

    stream->close();
    return res;
}

//=============================================================================================================

std::unique_ptr<MNECovMatrix> MNECovMatrix::dup() const
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
        res->sss = std::make_unique<MNESssData>(*sss);

    return res;
}

//=============================================================================================================

int MNECovMatrix::is_diag() const
{
    return cov_diag.size() > 0;
}

//=============================================================================================================

int MNECovMatrix::add_inv()
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

int MNECovMatrix::condition(float rank_threshold, int use_rank)
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
            mne_print_vector(stdout,nullptr,local_eigen.row(j).data(),ncov);
#endif
            for (k = 0; k < ncov; k++)
                data1(j,k) = sqrt(lambda_local[j])*local_eigen(j,k);
        }
        MatrixXd data2 = data1.transpose() * data1;
#ifdef DEBUG
        printf(">>>\n");
        for (j = 0; j < ncov; j++)
            mne_print_dvector(stdout,nullptr,data2.row(j).data(),ncov);
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

int MNECovMatrix::decompose_eigen_small(float p_small, int use_rank)
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

int MNECovMatrix::decompose_eigen()

{
    return decompose_eigen_small(-1.0,-1);
}

//=============================================================================================================

int MNECovMatrix::lt_packed_index(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}

//=============================================================================================================

int MNECovMatrix::classify_channels(const QList<FiffChInfo>& chs, int nchan)
{
    int k,p;
    FiffChInfo ch;

    if (chs.isEmpty()) {
        qCritical("Channel information not available in classify_channels");
        return FAIL;
    }
    ch_class.resize(ncov);
    for (k = 0; k < ncov; k++) {
        ch_class[k] = MNE_COV_CH_UNKNOWN;
        for (p = 0; p < nchan; p++) {
            if (QString::compare(chs[p].ch_name,names[k]) == 0) {
                ch = chs[p];
                if (ch.kind == FIFFV_MEG_CH) {
                    if (ch.unit == FIFF_UNIT_T)
                        ch_class[k] = MNE_COV_CH_MEG_MAG;
                    else
                        ch_class[k] = MNE_COV_CH_MEG_GRAD;
                }
                else if (ch.kind == FIFFV_EEG_CH)
                    ch_class[k] = MNE_COV_CH_EEG;
                break;
            }
        }
    }
    return OK;
}

//=============================================================================================================

int MNECovMatrix::whiten_vector(Eigen::Ref<Eigen::VectorXf> data, Eigen::Ref<Eigen::VectorXf> whitened_data, int nchan) const
{
    if (ncov != nchan) {
        qWarning("Incompatible covariance matrix. Cannot whiten the data.");
        return FAIL;
    }
    const double *inv = inv_lambda.data();
    if (is_diag()) {
        for (int k = 0; k < nchan; k++)
            whitened_data[k] = data[k]*inv[k];
    }
    else {
        Eigen::VectorXf tmp(nchan);
        for (int k = nzero; k < nchan; k++)
            tmp[k] = eigen.row(k).dot(data.cast<float>());
        for (int k = 0; k < nzero; k++)
            whitened_data[k] = 0.0;
        for (int k = nzero; k < nchan; k++)
            whitened_data[k] = tmp[k]*inv[k];
    }
    return OK;
}

//=============================================================================================================

void MNECovMatrix::regularize(const Eigen::Vector3f& regs)
{
    int    j;
    float  sums[3],nn[3];
    int    nkind = 3;

    if (cov.size() == 0 || ch_class.size() == 0)
        return;

    for (j = 0; j < nkind; j++) {
        sums[j] = 0.0;
        nn[j]   = 0;
    }
    for (j = 0; j < ncov; j++) {
        if (ch_class[j] >= 0) {
            sums[ch_class[j]] += cov[lt_packed_index(j,j)];
            nn[ch_class[j]]++;
        }
    }
    qInfo("Average noise-covariance matrix diagonals:");
    for (j = 0; j < nkind; j++) {
        if (nn[j] > 0) {
            sums[j] = sums[j]/nn[j];
            if (j == MNE_COV_CH_MEG_MAG)
                qInfo("\tMagnetometers       : %-7.2f fT    reg = %-6.2f",1e15*sqrt(sums[j]),regs[j]);
            else if (j == MNE_COV_CH_MEG_GRAD)
                qInfo("\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f",1e13*sqrt(sums[j]),regs[j]);
            else
                qInfo("\tEEG                 : %-7.2f uV    reg = %-6.2f",1e6*sqrt(sums[j]),regs[j]);
            sums[j] = regs[j]*sums[j];
        }
    }
    for (j = 0; j < ncov; j++)
        if (ch_class[j] >= 0)
            cov[lt_packed_index(j,j)] += sums[ch_class[j]];

    qInfo("Noise-covariance regularized as requested.");
}

//=============================================================================================================

void MNECovMatrix::revert_to_diag()
{
    int k,p;
    if (cov.size() == 0)
        return;

    cov_diag.resize(ncov);

    for (k = p = 0; k < ncov; k++) {
        cov_diag[k] = cov[p];
        p = p + k + 2;
    }
    cov.resize(0);

    lambda.resize(0);
    eigen.resize(0,0);
}

//=============================================================================================================

std::unique_ptr<MNECovMatrix> MNECovMatrix::pick_chs_omit(const QStringList& new_names,
                                                           int new_ncov,
                                                           int omit_meg_eeg,
                                                           const QList<FiffChInfo>& chs) const
{
    int j,k;
    Eigen::VectorXd cov_local;
    Eigen::VectorXd cov_diag_local;
    QStringList picked_names;
    int   from,to;
    std::unique_ptr<MNECovMatrix> res;

    if (new_ncov == 0) {
        qCritical("No channels specified for picking in pick_chs_omit");
        return nullptr;
    }
    if (names.isEmpty()) {
        qCritical("No names in covariance matrix. Cannot do picking.");
        return nullptr;
    }
    Eigen::VectorXi pickVec = Eigen::VectorXi::Constant(new_ncov, -1);
    for (j = 0; j < new_ncov; j++)
        for (k = 0; k < ncov; k++)
            if (QString::compare(names[k],new_names[j]) == 0) {
                pickVec[j] = k;
                break;
            }
    for (j = 0; j < new_ncov; j++) {
        if (pickVec[j] < 0) {
            qWarning("All desired channels not found in the covariance matrix (at least missing %s).", new_names[j].toUtf8().constData());
            return nullptr;
        }
    }
    Eigen::VectorXi isMegVec;
    if (omit_meg_eeg) {
        isMegVec.resize(new_ncov);
        if (!chs.isEmpty()) {
            for (j = 0; j < new_ncov; j++)
                if (chs[j].kind == FIFFV_MEG_CH)
                    isMegVec[j] = true;
                else
                    isMegVec[j] = false;
        }
        else {
            for (j = 0; j < new_ncov; j++)
                if (new_names[j].startsWith("MEG"))
                    isMegVec[j] = true;
                else
                    isMegVec[j] = false;
        }
    }
    if (cov_diag.size() > 0) {
        cov_diag_local.resize(new_ncov);
        for (j = 0; j < new_ncov; j++) {
            cov_diag_local[j] = cov_diag[pickVec[j]];
            picked_names.append(names[pickVec[j]]);
        }
    }
    else {
        cov_local.resize(new_ncov*(new_ncov+1)/2);
        for (j = 0; j < new_ncov; j++) {
            picked_names.append(names[pickVec[j]]);
            for (k = 0; k <= j; k++) {
                from = lt_packed_index(pickVec[j],pickVec[k]);
                to   = lt_packed_index(j,k);
                if (to < 0 || to > new_ncov*(new_ncov+1)/2-1) {
                    qCritical("Wrong destination index in pick_chs_omit : %d %d %d",j,k,to);
                    return nullptr;
                }
                if (from < 0 || from > ncov*(ncov+1)/2-1) {
                    qCritical("Wrong source index in pick_chs_omit : %d %d %d",pickVec[j],pickVec[k],from);
                    return nullptr;
                }
                cov_local[to] = cov[from];
                if (omit_meg_eeg)
                    if (isMegVec[j] != isMegVec[k])
                        cov_local[to] = 0.0;
            }
        }
    }

    res = MNECovMatrix::create(kind,new_ncov,picked_names,cov_local,cov_diag_local);

    res->bads = bads;
    res->nbad = nbad;
    res->proj.reset(proj ? proj->dup() : nullptr);
    res->sss.reset(sss ? new MNESssData(*sss) : nullptr);

    if (ch_class.size() > 0) {
        res->ch_class.resize(res->ncov);
        for (k = 0; k < res->ncov; k++)
            res->ch_class[k] = ch_class[pickVec[k]];
    }
    return res;
}
