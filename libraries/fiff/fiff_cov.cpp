//=============================================================================================================
/**
 * @file     fiff_cov.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffCov Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_cov.h"
#include "fiff_stream.h"
#include "fiff_info_base.h"
#include "fiff_dir_node.h"

#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCov::FiffCov()
: kind(-1)
, diag(false)
, dim(-1)
, nfree(-1)
{
    qRegisterMetaType<QSharedPointer<FIFFLIB::FiffCov> >("QSharedPointer<FIFFLIB::FiffCov>");
    qRegisterMetaType<FIFFLIB::FiffCov>("FIFFLIB::FiffCov");
}

//=============================================================================================================

FiffCov::FiffCov(QIODevice &p_IODevice)
: kind(-1)
, diag(false)
, dim(-1)
, nfree(-1)
{
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if(!t_pStream->open())
    {
        printf("\tNot able to open IODevice.\n");//ToDo Throw here
        return;
    }

    if(!t_pStream->read_cov(t_pStream->dirtree(), FIFFV_MNE_NOISE_COV, *this))
        printf("\tFiff covariance not found.\n");//ToDo Throw here

    qRegisterMetaType<QSharedPointer<FIFFLIB::FiffCov> >("QSharedPointer<FIFFLIB::FiffCov>");
    qRegisterMetaType<FIFFLIB::FiffCov>("FIFFLIB::FiffCov");
}

//=============================================================================================================

FiffCov::FiffCov(const FiffCov &p_FiffCov)
: QSharedData(p_FiffCov)
, kind(p_FiffCov.kind)
, diag(p_FiffCov.diag)
, dim(p_FiffCov.dim)
, names(p_FiffCov.names)
, data(p_FiffCov.data)
, projs(p_FiffCov.projs)
, bads(p_FiffCov.bads)
, nfree(p_FiffCov.nfree)
, eig(p_FiffCov.eig)
, eigvec(p_FiffCov.eigvec)
{
    qRegisterMetaType<QSharedPointer<FIFFLIB::FiffCov> >("QSharedPointer<FIFFLIB::FiffCov>");
    qRegisterMetaType<FIFFLIB::FiffCov>("FIFFLIB::FiffCov");
}

//=============================================================================================================

FiffCov::~FiffCov()
{
}

//=============================================================================================================

void FiffCov::clear()
{
    kind = -1;
    diag = false;
    dim = -1;
    names.clear();
    data = MatrixXd();
    projs.clear();
    bads.clear();
    nfree = -1;
    eig = VectorXd();
    eigvec = MatrixXd();
}

//=============================================================================================================

FiffCov FiffCov::pick_channels(const QStringList &p_include, const QStringList &p_exclude)
{
    RowVectorXi sel = FiffInfoBase::pick_channels(this->names, p_include, p_exclude);
    FiffCov res;//No deep copy here - since almost everything else is adapted anyway

    res.kind = this->kind;
    res.diag = this->diag;
    res.dim = sel.size();

    for(qint32 k = 0; k < sel.size(); ++k)
        res.names << this->names[sel(k)];

    res.data.resize(res.dim, res.dim);
    for(qint32 i = 0; i < res.dim; ++i)
        for(qint32 j = 0; j < res.dim; ++j)
            res.data(i, j) = this->data(sel(i), sel(j));
    res.projs = this->projs;

    for(qint32 k = 0; k < this->bads.size(); ++k)
        if(res.names.contains(this->bads[k]))
            res.bads << this->bads[k];
    res.nfree = this->nfree;

    return res;
}

//=============================================================================================================

FiffCov FiffCov::prepare_noise_cov(const FiffInfo &p_Info, const QStringList &p_ChNames) const
{
    FiffCov p_NoiseCov(*this);

    VectorXi C_ch_idx = VectorXi::Zero(p_NoiseCov.names.size());
    qint32 count = 0;
    for(qint32 i = 0; i < p_ChNames.size(); ++i)
    {
        qint32 idx = p_NoiseCov.names.indexOf(p_ChNames[i]);
        if(idx > -1)
        {
            C_ch_idx[count] = idx;
            ++count;
        }
    }
    C_ch_idx.conservativeResize(count);

    MatrixXd C(count, count);

    if(!p_NoiseCov.diag)
        for(qint32 i = 0; i < count; ++i)
            for(qint32 j = 0; j < count; ++j)
                C(i,j) = p_NoiseCov.data(C_ch_idx(i), C_ch_idx(j));
    else
    {
        qWarning("Warning in FiffCov::prepare_noise_cov: This has to be debugged - not done before!");
        C = MatrixXd::Zero(count, count);
        for(qint32 i = 0; i < count; ++i)
            C.diagonal()[i] = p_NoiseCov.data(C_ch_idx(i),0);
    }

    MatrixXd proj;
    qint32 ncomp = p_Info.make_projector(proj, p_ChNames);

    //Create the projection operator
    if (ncomp > 0 && proj.rows() == count)
    {
        printf("Created an SSP operator (subspace dimension = %d)\n", ncomp);
        C = proj * (C * proj.transpose());
    } else {
        qWarning("Warning in FiffCov::prepare_noise_cov: No projections applied since no projectors specified or projector dimensions do not match!");
    }

    RowVectorXi pick_meg = p_Info.pick_types(true, false, false, defaultQStringList, p_Info.bads);
    RowVectorXi pick_eeg = p_Info.pick_types(false, true, false, defaultQStringList, p_Info.bads);

    QStringList meg_names, eeg_names;

    for(qint32 i = 0; i < pick_meg.size(); ++i)
        meg_names << p_Info.chs[pick_meg[i]].ch_name;
    VectorXi C_meg_idx = VectorXi::Zero(p_NoiseCov.names.size());
    count = 0;
    for(qint32 k = 0; k < C.rows(); ++k)
    {
        if(meg_names.indexOf(p_ChNames[k]) > -1)
        {
            C_meg_idx[count] = k;
            ++count;
        }
    }
    if(count > 0)
        C_meg_idx.conservativeResize(count);
    else
        C_meg_idx = VectorXi();

    //
    for(qint32 i = 0; i < pick_eeg.size(); ++i)
        eeg_names << p_Info.chs[pick_eeg(0,i)].ch_name;
    VectorXi C_eeg_idx = VectorXi::Zero(p_NoiseCov.names.size());
    count = 0;
    for(qint32 k = 0; k < C.rows(); ++k)
    {
        if(eeg_names.indexOf(p_ChNames[k]) > -1)
        {
            C_eeg_idx[count] = k;
            ++count;
        }
    }

    if(count > 0)
        C_eeg_idx.conservativeResize(count);
    else
        C_eeg_idx = VectorXi();

    bool has_meg = C_meg_idx.size() > 0;
    bool has_eeg = C_eeg_idx.size() > 0;

    MatrixXd C_meg, C_eeg;
    VectorXd C_meg_eig, C_eeg_eig;
    MatrixXd C_meg_eigvec, C_eeg_eigvec;
    if (has_meg)
    {
        count = C_meg_idx.rows();
        C_meg = MatrixXd(count,count);
        for(qint32 i = 0; i < count; ++i)
            for(qint32 j = 0; j < count; ++j)
                C_meg(i,j) = C(C_meg_idx(i), C_meg_idx(j));
        MNEMath::get_whitener(C_meg, false, QString("MEG"), C_meg_eig, C_meg_eigvec);
    }

    if (has_eeg)
    {
        count = C_eeg_idx.rows();
        C_eeg = MatrixXd(count,count);
        for(qint32 i = 0; i < count; ++i)
            for(qint32 j = 0; j < count; ++j)
                C_eeg(i,j) = C(C_eeg_idx(i), C_eeg_idx(j));
        MNEMath::get_whitener(C_eeg, false, QString("EEG"), C_eeg_eig, C_eeg_eigvec);
    }

    qint32 n_chan = p_ChNames.size();
    p_NoiseCov.eigvec = MatrixXd::Zero(n_chan, n_chan);
    p_NoiseCov.eig = VectorXd::Zero(n_chan);

    if(has_meg)
    {
        for(qint32 i = 0; i < C_meg_idx.rows(); ++i)
            for(qint32 j = 0; j < C_meg_idx.rows(); ++j)
                p_NoiseCov.eigvec(C_meg_idx[i], C_meg_idx[j]) = C_meg_eigvec(i, j);
        for(qint32 i = 0; i < C_meg_idx.rows(); ++i)
            p_NoiseCov.eig(C_meg_idx[i]) = C_meg_eig[i];
    }
    if(has_eeg)
    {
        for(qint32 i = 0; i < C_eeg_idx.rows(); ++i)
            for(qint32 j = 0; j < C_eeg_idx.rows(); ++j)
                p_NoiseCov.eigvec(C_eeg_idx[i], C_eeg_idx[j]) = C_eeg_eigvec(i, j);
        for(qint32 i = 0; i < C_eeg_idx.rows(); ++i)
            p_NoiseCov.eig(C_eeg_idx[i]) = C_eeg_eig[i];
    }

    if (C_meg_idx.size() + C_eeg_idx.size() != n_chan)
    {
        printf("Error in FiffCov::prepare_noise_cov: channel sizes do no match!\n");//ToDo Throw here
        return FiffCov();
    }

    p_NoiseCov.data = C;
    p_NoiseCov.dim = p_ChNames.size();
    p_NoiseCov.diag = false;
    p_NoiseCov.names = p_ChNames;

    return p_NoiseCov;
}

//=============================================================================================================

FiffCov FiffCov::regularize(const FiffInfo& p_info, double p_fRegMag, double p_fRegGrad, double p_fRegEeg, bool p_bProj, QStringList p_exclude) const
{
    FiffCov cov(*this);

    if(p_exclude.size() == 0)
    {
        p_exclude = p_info.bads;
        for(qint32 i = 0; i < cov.bads.size(); ++i)
            if(!p_exclude.contains(cov.bads[i]))
                p_exclude << cov.bads[i];
    }

    //Allways exclude all STI channels from covariance computation
    int iNoStimCh = 0;

    for(int i=0; i<p_info.chs.size(); i++) {
        if(p_info.chs[i].kind == FIFFV_STIM_CH) {
            p_exclude << p_info.chs[i].ch_name;
            iNoStimCh++;
        }
    }

    RowVectorXi sel_eeg = p_info.pick_types(false, true, false, defaultQStringList, p_exclude);
    RowVectorXi sel_mag = p_info.pick_types(QString("mag"), false, false, defaultQStringList, p_exclude);
    RowVectorXi sel_grad = p_info.pick_types(QString("grad"), false, false, defaultQStringList, p_exclude);

    QStringList info_ch_names = p_info.ch_names;
    QStringList ch_names_eeg, ch_names_mag, ch_names_grad;
    for(qint32 i = 0; i < sel_eeg.size(); ++i)
        ch_names_eeg << info_ch_names[sel_eeg(i)];
    for(qint32 i = 0; i < sel_mag.size(); ++i)
        ch_names_mag << info_ch_names[sel_mag(i)];
    for(qint32 i = 0; i < sel_grad.size(); ++i)
        ch_names_grad << info_ch_names[sel_grad(i)];

    // This actually removes bad channels from the cov, which is not backward
    // compatible, so let's leave all channels in
    FiffCov cov_good = cov.pick_channels(info_ch_names, p_exclude);
    QStringList ch_names = cov_good.names;

    std::vector<qint32> idx_eeg, idx_mag, idx_grad;
    for(qint32 i = 0; i < ch_names.size(); ++i)
    {
        if(ch_names_eeg.contains(ch_names[i]))
            idx_eeg.push_back(i);
        else if(ch_names_mag.contains(ch_names[i]))
            idx_mag.push_back(i);
        else if(ch_names_grad.contains(ch_names[i]))
            idx_grad.push_back(i);
    }

    MatrixXd C(cov_good.data);

    //Subtract number of found stim channels because they are still in C but not the idx_eeg, idx_mag or idx_grad
    if((unsigned) C.rows() - iNoStimCh != idx_eeg.size() + idx_mag.size() + idx_grad.size()) {
        printf("Error in FiffCov::regularize: Channel dimensions do not fit.\n");//ToDo Throw
    }

    QList<FiffProj> t_listProjs;
    if(p_bProj)
    {
        t_listProjs = p_info.projs + cov_good.projs;
        FiffProj::activate_projs(t_listProjs);
    }

    //Build regularization MAP
    QMap<QString, QPair<double, std::vector<qint32> > > regData;
    regData.insert("EEG", QPair<double, std::vector<qint32> >(p_fRegEeg, idx_eeg));
    regData.insert("MAG", QPair<double, std::vector<qint32> >(p_fRegMag, idx_mag));
    regData.insert("GRAD", QPair<double, std::vector<qint32> >(p_fRegGrad, idx_grad));

    //
    //Regularize
    //
    QMap<QString, QPair<double, std::vector<qint32> > >::Iterator it;
    for(it = regData.begin(); it != regData.end(); ++it)
    {
        QString desc(it.key());
        double reg = it.value().first;
        std::vector<qint32> idx = it.value().second;

        if(idx.size() == 0 || reg == 0.0)
            printf("\tNothing to regularize within %s data.\n", desc.toUtf8().constData());
        else
        {
            printf("\tRegularize %s: %f\n", desc.toUtf8().constData(), reg);
            MatrixXd this_C(idx.size(), idx.size());
            for(quint32 i = 0; i < idx.size(); ++i)
                for(quint32 j = 0; j < idx.size(); ++j)
                    this_C(i,j) = cov_good.data(idx[i], idx[j]);

            MatrixXd U;
            qint32 ncomp;
            if(p_bProj)
            {
                QStringList this_ch_names;
                for(quint32 k = 0; k < idx.size(); ++k)
                    this_ch_names << ch_names[idx[k]];

                MatrixXd P;
                ncomp = FiffProj::make_projector(t_listProjs, this_ch_names, P); //ToDo: Synchronize with mne-python and debug

                JacobiSVD<MatrixXd> svd(P, ComputeFullU);
                //Sort singular values and singular vectors
                VectorXd t_s = svd.singularValues();
                MatrixXd t_U = svd.matrixU();
                MNEMath::sort<double>(t_s, t_U);

                U = t_U.block(0,0, t_U.rows(), t_U.cols()-ncomp);

                if (ncomp > 0)
                {
                    printf("\tCreated an SSP operator for %s (dimension = %d).\n", desc.toUtf8().constData(), ncomp);
                    this_C = U.transpose() * (this_C * U);
                }
            }

            double sigma = this_C.diagonal().mean();
            this_C.diagonal() = this_C.diagonal().array() + reg * sigma;  // modify diag inplace
            if(p_bProj && ncomp > 0)
                this_C = U * (this_C * U.transpose());

            for(qint32 i = 0; i < this_C.rows(); ++i)
                for(qint32 j = 0; j < this_C.cols(); ++j)
                    C(idx[i],idx[j]) = this_C(i,j);
        }
    }

    // Put data back in correct locations
    RowVectorXi idx = FiffInfo::pick_channels(cov.names, info_ch_names, p_exclude);
    for(qint32 i = 0; i < idx.size(); ++i)
        for(qint32 j = 0; j < idx.size(); ++j)
            cov.data(idx[i], idx[j]) = C(i, j);

    return cov;
}

//=============================================================================================================

FiffCov& FiffCov::operator= (const FiffCov &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        kind = rhs.kind;
        diag = rhs.diag;
        dim = rhs.dim;
        names = rhs.names;
        data = rhs.data;
        projs = rhs.projs;
        bads = rhs.bads;
        nfree = rhs.nfree;
        eig = rhs.eig;
        eigvec = rhs.eigvec;
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}
