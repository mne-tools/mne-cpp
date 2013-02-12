//=============================================================================================================
/**
* @file     fiff_cov.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the FiffCov Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_cov.h"
#include "fiff_stream.h"

#include <mneMath/mnemath.h>


#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNEMATHLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCov::FiffCov()
: kind(-1)
, diag(false)
, dim(-1)
, nfree(-1)
{

}


//*************************************************************************************************************

FiffCov::FiffCov(QIODevice &p_IODevice)
: kind(-1)
, diag(false)
, dim(-1)
, nfree(-1)
{
    FiffStream* t_pStream = new FiffStream(&p_IODevice);
    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;

    if(!t_pStream->open(t_Tree, t_Dir))
    {
        if(t_pStream)
            delete t_pStream;
        printf("\tNot able to open IODevice.\n");//ToDo Throw here
        return;
    }

    if(!t_pStream->read_cov(t_Tree, FIFFV_MNE_NOISE_COV, *this))
        printf("\tFiff covariance not found.\n");//ToDo Throw here

    if(t_pStream)
        delete t_pStream;
}


//*************************************************************************************************************

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

}


//*************************************************************************************************************

FiffCov::~FiffCov()
{
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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
    for(qint32 i = 0; i < count; ++i)
        for(qint32 j = 0; j < count; ++j)
            C(i,j) = p_NoiseCov.data(C_ch_idx(i), C_ch_idx(j));

    MatrixXd proj;
    qint32 ncomp = p_Info.make_projector_info(proj);

    //Create the projection operator
    if (ncomp > 0)
    {
        printf("Created an SSP operator (subspace dimension = %d)\n", ncomp);

        C = proj * (C * proj.transpose());
    }

    MatrixXi pick_meg = p_Info.pick_types(true, false, false, defaultQStringList, p_Info.bads);
    MatrixXi pick_eeg = p_Info.pick_types(false, true, false, defaultQStringList, p_Info.bads);

    QStringList meg_names, eeg_names;

    for(qint32 i = 0; i < pick_meg.size(); ++i)
        meg_names << p_Info.chs[pick_meg(0,i)].ch_name;
    VectorXi C_meg_idx = VectorXi::Zero(p_NoiseCov.names.size());
    count = 0;
    for(qint32 k = 0; k < C.rows(); ++k)
    {
        if(meg_names.indexOf(p_ChNames[k]) > -1)
        {
            C_meg_idx(count) = k;
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
            C_eeg_idx(count) = k;
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
                p_NoiseCov.eigvec(C_meg_idx(i), C_meg_idx(j)) = C_meg_eigvec(i, j);
        for(qint32 i = 0; i < C_meg_idx.rows(); ++i)
            p_NoiseCov.eig(C_meg_idx(i)) = C_meg_eig(i);
    }
    if(has_eeg)
    {
        for(qint32 i = 0; i < C_eeg_idx.rows(); ++i)
            for(qint32 j = 0; j < C_eeg_idx.rows(); ++j)
                p_NoiseCov.eigvec(C_eeg_idx(i), C_eeg_idx(j)) = C_eeg_eigvec(i, j);
        for(qint32 i = 0; i < C_eeg_idx.rows(); ++i)
            p_NoiseCov.eig(C_eeg_idx(i)) = C_eeg_eig(i);
    }

    if (C_meg_idx.size() + C_eeg_idx.size() == n_chan)
        return FiffCov();

    p_NoiseCov.dim = p_ChNames.size();
    p_NoiseCov.diag = false;
    p_NoiseCov.names = p_ChNames;

    return p_NoiseCov;
}


//*************************************************************************************************************

FiffCov FiffCov::regularize(const FiffInfo& p_info, float p_fMag, float p_fGrad, float p_fEeg, bool p_bProj, QStringList p_exclude) const
{
    FiffCov cov(*this);


    if(p_exclude.size() == 0)
    {
        p_exclude = p_info.bads;
        for(qint32 i = 0; i < cov.bads.size(); ++i)
            if(!p_exclude.contains(cov.bads[i]))
                p_exclude << cov.bads[i];
    }

    RowVectorXi sel_eeg = p_info.pick_types(false, true, false, defaultQStringList, p_exclude);
    RowVectorXi sel_mag = p_info.pick_types(QString("mag"), false, false, defaultQStringList, p_exclude);
    RowVectorXi sel_grad = p_info.pick_types(QString("grad"), false, false, defaultQStringList, p_exclude);


    QStringList info_ch_names = p_info.ch_names;
    QStringList ch_names_eeg;
//    for(qint32 i = 0; i < sel_eeg.size(); ++i)

//    = [info_ch_names[i] for i in sel_eeg]
//    QStringList ch_names_mag; = [info_ch_names[i] for i in sel_mag]
//    QStringList ch_names_grad; = [info_ch_names[i] for i in sel_grad]



    qDebug() << "ToDo Regularize...";


    return cov;
}


//*************************************************************************************************************

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
