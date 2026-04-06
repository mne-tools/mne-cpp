//=============================================================================================================
/**
 * @file     mne_ctf_comp_data_set.cpp
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
 * @brief    Definition of the MNECTFCompDataSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data_set.h"
#include "mne_ctf_comp_data.h"

#include "mne_types.h"

#include <fiff/fiff_coord_trans.h>

#include <fiff/fiff_types.h>

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

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



//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_32(const QString& name,
                                  QList<FIFFLIB::FiffChInfo>& megp,	 /* MEG channels */
                                  int *nmegp,
                                  QList<FIFFLIB::FiffChInfo>& meg_compp,
                                  int *nmeg_compp,
                                  QList<FIFFLIB::FiffChInfo>& eegp,	 /* EEG channels */
                                  int *neegp,
                                  FiffCoordTrans *meg_head_t,
                                  fiffId *idp)	 /* The measurement ID */
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FIFFLIB::FiffChInfo> chs;
    int        nchan = 0;
    QList<FIFFLIB::FiffChInfo> meg;
    int        nmeg  = 0;
    QList<FIFFLIB::FiffChInfo> meg_comp;
    int        nmeg_comp = 0;
    QList<FIFFLIB::FiffChInfo> eeg;
    int        neeg  = 0;
    std::unique_ptr<FiffId> id;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::UPtr t_pTag;
    FIFFLIB::FiffChInfo   this_ch;
    FiffCoordTrans t;
    fiff_int_t kind, pos;
    int j,k,to_find;

    if(!stream->open())
        goto bad;

    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            qCritical ("Could not find the channel information.");
            goto bad;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent(); k++) {
        kind = info->dir[k]->kind;
        pos  = info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();

            for (j = 0; j < nchan; j++) {
                chs.append(FiffChInfo());
                chs[j].scanNo = -1;
            }
            to_find = nchan;
            break;

        case FIFF_PARENT_BLOCK_ID :
            if(!stream->read_tag(t_pTag, pos))
                goto bad;
            id = std::make_unique<FiffId>(*(FiffId*)t_pTag->data());
            break;

        case FIFF_COORD_TRANS :
            if(!stream->read_tag(t_pTag, pos))
                goto bad;
//            t = t_pTag->toCoordTrans();
            t = FiffCoordTrans::readFromTag( t_pTag );
            if (t.from != FIFFV_COORD_DEVICE || t.to   != FIFFV_COORD_HEAD)
                t = FiffCoordTrans();
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if(!stream->read_tag(t_pTag, pos))
                goto bad;

            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > nchan) {
                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch.scanNo,nchan);
                goto bad;
            }
            else
                chs[this_ch.scanNo-1] = this_ch;
            to_find--;
            break;
        }
    }
    if (to_find != 0) {
        qCritical("Some of the channel information was missing.");
        goto bad;
    }
    if (t.isEmpty() && meg_head_t != nullptr) {
        /*
     * Try again in a more general fashion
     */
        t = FiffCoordTrans::readMeasTransform(name);
        if (t.isEmpty()) {
            qCritical("MEG -> head coordinate transformation not found.");
            goto bad;
        }
    }
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            meg.append(chs[k]);
            nmeg++;
        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
            meg_comp.append(chs[k]);
            nmeg_comp++;
        } else if (chs[k].kind == FIFFV_EEG_CH) {
            eeg.append(chs[k]);
            neeg++;
        }
    }
//    fiff_close(in);
    stream->close();

    megp  = meg;
    if(nmegp) {
        *nmegp = nmeg;
    }

    meg_compp = meg_comp;
    if(nmeg_compp) {
        *nmeg_compp = nmeg_comp;
    }

    eegp = eeg;
    if(neegp) {
        *neegp = neeg;
    }

    if (idp == nullptr) {
        /* id is auto-deleted by unique_ptr */
    }
    else
        *idp   = id.release();
    if (meg_head_t == nullptr) {
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
        stream->close();
        return FIFF_FAIL;
    }
}

#define MNE_CTFV_COMP_UNKNOWN -1
#define MNE_CTFV_COMP_NONE    0
#define MNE_CTFV_COMP_G1BR    0x47314252
#define MNE_CTFV_COMP_G2BR    0x47324252
#define MNE_CTFV_COMP_G3BR    0x47334252
#define MNE_CTFV_COMP_G2OI    0x47324f49
#define MNE_CTFV_COMP_G3OI    0x47334f49

static struct {
    int grad_comp;
    int ctf_comp;
} compMap[] = { { MNE_CTFV_NOGRAD,       MNE_CTFV_COMP_NONE },
{ MNE_CTFV_GRAD1,        MNE_CTFV_COMP_G1BR },
{ MNE_CTFV_GRAD2,        MNE_CTFV_COMP_G2BR },
{ MNE_CTFV_GRAD3,        MNE_CTFV_COMP_G3BR },
{ MNE_4DV_COMP1,         MNE_4DV_COMP1 },             /* One-to-one mapping for 4D data */
{ MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN }};

int mne_unmap_ctf_comp_kind(int ctf_comp)

{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (ctf_comp == compMap[k].ctf_comp)
            return compMap[k].grad_comp;
    return ctf_comp;
}

std::unique_ptr<FiffSparseMatrix> mne_convert_to_sparse(const Eigen::MatrixXf& dense,   /* The dense matrix to be converted */
                                      int   stor_type,      /* Either FIFFTS_MC_CCS or FIFFTS_MC_RCS */
                                      float small)          /* How small elements should be ignored? */
/*
 * Create the compressed row or column storage sparse matrix representation
 * including a vector containing the nonzero matrix element values,
 * the row or column pointer vector and the appropriate index vector(s).
 */
{
    int j,k;
    int nz;
    int ptr;
    int nrow = static_cast<int>(dense.rows());
    int ncol = static_cast<int>(dense.cols());

    if (small < 0) {		/* Automatic scaling */
        float maxval = dense.cwiseAbs().maxCoeff();
        if (maxval > 0)
            small = maxval*std::fabs(small);
        else
            small = std::fabs(small);
    }
    for (j = 0, nz = 0; j < nrow; j++)
        for (k = 0; k < ncol; k++) {
            if (std::fabs(dense(j,k)) > small)
                nz++;
        }

    if (nz <= 0) {
        qWarning("No nonzero elements found.");
        return nullptr;
    }
    if (stor_type != FIFFTS_MC_CCS && stor_type != FIFFTS_MC_RCS) {
        qWarning("Unknown sparse matrix storage type: %d",stor_type);
        return nullptr;
    }
    auto sparse = std::make_unique<FiffSparseMatrix>();
    sparse->coding = stor_type;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data.resize(nz);
    sparse->inds.resize(nz);

    if (stor_type == FIFFTS_MC_RCS) {
        sparse->ptrs.resize(nrow + 1);
        for (j = 0, nz = 0; j < nrow; j++) {
            ptr = -1;
            for (k = 0; k < ncol; k++)
                if (std::fabs(dense(j,k)) > small) {
                    sparse->data[nz] = dense(j,k);
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = k;
                }
            sparse->ptrs[j] = ptr;
        }
        sparse->ptrs[nrow] = nz;
        for (j = nrow - 1; j >= 0; j--) /* Take care of the empty rows */
            if (sparse->ptrs[j] < 0)
                sparse->ptrs[j] = sparse->ptrs[j+1];
    }
    else if (stor_type == FIFFTS_MC_CCS) {
        sparse->ptrs.resize(ncol + 1);
        for (k = 0, nz = 0; k < ncol; k++) {
            ptr = -1;
            for (j = 0; j < nrow; j++)
                if (std::fabs(dense(j,k)) > small) {
                    sparse->data[nz] = dense(j,k);
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = j;
                }
            sparse->ptrs[k] = ptr;
        }
        sparse->ptrs[ncol] = nz;
        for (k = ncol-1; k >= 0; k--) /* Take care of the empty columns */
            if (sparse->ptrs[k] < 0)
                sparse->ptrs[k] = sparse->ptrs[k+1];
    }
    return sparse;
}

int  mne_sparse_vec_mult2_32(FiffSparseMatrix* mat,     /* The sparse matrix */
                          float           *vector, /* Vector to be multiplied */
                          float           *res)    /* Result of the multiplication */
/*
      * Multiply a vector by a sparse matrix.
      */
{
    int i,j;

    if (mat->coding == FIFFTS_MC_RCS) {
        for (i = 0; i < mat->m; i++) {
            res[i] = 0.0;
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[i] += mat->data[j]*vector[mat->inds[j]];
        }
        return 0;
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (i = 0; i < mat->m; i++)
            res[i] = 0.0;
        for (i = 0; i < mat->n; i++)
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[mat->inds[j]] += mat->data[j]*vector[i];
        return 0;
    }
    else {
        qWarning("mne_sparse_vec_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNECTFCompDataSet::MNECTFCompDataSet()
:ncomp(0)
,nch(0)
,undo(nullptr)
,current(nullptr)
{
}

//=============================================================================================================

MNECTFCompDataSet::MNECTFCompDataSet(const MNECTFCompDataSet &set)
:ncomp(0)
,nch(set.nch)
,undo(nullptr)
,current(nullptr)
{
    if (set.ncomp > 0) {
        for (int k = 0; k < set.ncomp; k++)
            if(set.comps[k])
                this->comps.append(std::make_unique<MNECTFCompData>(*set.comps[k]));
        this->ncomp = this->comps.size();
    }

    this->chs = set.chs;

    if(set.undo)
        this->undo = std::make_unique<MNECTFCompData>(*set.undo);

    if(set.current)
        this->current = std::make_unique<MNECTFCompData>(*set.current);
}

//=============================================================================================================

MNECTFCompDataSet::~MNECTFCompDataSet()
{
}

//=============================================================================================================

std::unique_ptr<MNECTFCompDataSet> MNECTFCompDataSet::read(const QString &name)
/*
     * Read all CTF compensation data from a given file
     */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    std::unique_ptr<MNECTFCompDataSet> set;
    QList<FiffDirNode::SPtr> nodes;
    QList<FiffDirNode::SPtr> comps;
    int ncomp;
    int kind,k;
    FiffTag::UPtr t_pTag;
    QList<FiffChInfo> chs;
    int nch = 0;
    int calibrated;
    /*
        * Read the channel information
        */
    {
        QList<FiffChInfo> comp_chs, temp;
        int ncompch = 0;

        if (mne_read_meg_comp_eeg_ch_info_32(name,
                                             chs,
                                             &nch,
                                             comp_chs,
                                             &ncompch,
                                             temp,
                                             nullptr,
                                             nullptr,
                                             nullptr) == FAIL)
            goto bad;
        if (ncompch > 0) {
            for (k = 0; k < ncompch; k++)
                chs.append(comp_chs[k]);
            nch = nch + ncompch;
        }
    }
    /*
        * Read the rest of the stuff
        */
    if(!stream->open())
        goto bad;
    set = std::make_unique<MNECTFCompDataSet>();
    /*
        * Locate the compensation data sets
        */
    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_CTF_COMP);
    if (nodes.size() == 0)
        goto good;      /* Nothing more to do */
    comps = nodes[0]->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);
    if (comps.size() == 0)
        goto good;
    ncomp = comps.size();
    /*
        * Set the channel info
        */
    set->chs = chs;
    set->nch = nch;
    /*
        * Read each data set
        */
    for (k = 0; k < ncomp; k++) {
        auto mat = MNENamedMatrix::read(stream,comps[k],FIFF_MNE_CTF_COMP_DATA);
        if (!mat)
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_KIND, t_pTag);
        if (t_pTag) {
            kind = *t_pTag->toInt();
        }
        else
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_CALIBRATED, t_pTag);
        if (t_pTag) {
            calibrated = *t_pTag->toInt();
        }
        else
            calibrated = FALSE;
        /*
            * Add these data to the set
            */
        auto one = std::make_unique<MNECTFCompData>();
        one->data = std::move(mat);
        one->kind                = kind;
        one->mne_kind            = mne_unmap_ctf_comp_kind(one->kind);
        one->calibrated          = calibrated;

        if (one->calibrate(set->chs,set->nch,TRUE) == FAIL) {
            printf("Warning: Compensation data for '%s' omitted\n", explain_comp(one->kind).toUtf8().constData());
        }
        else {
            set->comps.append(std::move(one));
            set->ncomp++;
        }
    }
#ifdef DEBUG
    printf("%d CTF compensation data sets read from %s\n",set->ncomp,name);
#endif
    goto good;

bad : {
        stream->close();
        return nullptr;
    }

good : {
        stream->close();
        return set;
    }
}

//=============================================================================================================

int MNECTFCompDataSet::make_comp(const QList<FiffChInfo>& chs,
                                 int nch,
                                 QList<FiffChInfo> compchs,
                                 int ncomp)      /* How many of these */
/*
     * Make compensation data to apply to a set of channels to yield (or uncompensated) compensated data
     */
{
    Eigen::VectorXi comps;
    int need_comp;
    int first_comp;
    MNECTFCompData* this_comp;
    Eigen::VectorXi comp_sel;
    QStringList names;
    QString name;
    int  j,k,p;

    std::unique_ptr<FiffSparseMatrix> presel;
    std::unique_ptr<FiffSparseMatrix> postsel;
    std::unique_ptr<MNENamedMatrix>   data;

    QStringList emptyList;

    if (compchs.isEmpty()) {
        compchs = chs;
        ncomp   = nch;
    }
    printf("Setting up compensation data...\n");
    if (nch == 0)
        return OK;
    current.reset();
    comps.resize(nch);
    for (k = 0, need_comp = 0, first_comp = MNE_CTFV_COMP_NONE; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comps[k] = chs[k].chpos.coil_type >> 16;
            if (comps[k] != MNE_CTFV_COMP_NONE) {
                if (first_comp == MNE_CTFV_COMP_NONE)
                    first_comp = comps[k];
                else {
                    if (comps[k] != first_comp) {
                        printf("We do not support nonuniform compensation yet.");
                        return FAIL;
                    }
                }
                need_comp++;
            }
        }
        else
            comps[k] = MNE_CTFV_COMP_NONE;
    }
    if (need_comp == 0) {
        printf("\tNo compensation set. Nothing more to do.\n");
        return OK;
    }
    printf("\t%d out of %d channels have the compensation set.\n",need_comp,nch);
    /*
        * Find the desired compensation data matrix
        */
    for (k = 0, this_comp = nullptr; k < this->ncomp; k++) {
        if (this->comps[k]->mne_kind == first_comp) {
            this_comp = this->comps[k].get();
            break;
        }
    }
    if (!this_comp) {
        printf("Did not find the desired compensation data : %s",
               explain_comp(map_comp_kind(first_comp)).toUtf8().constData());
        return FAIL;
    }
    printf("\tDesired compensation data (%s) found.\n",explain_comp(map_comp_kind(first_comp)).toUtf8().constData());
    /*
        * Find the compensation channels
        */
    comp_sel.resize(this_comp->data->ncol);
    for (k = 0; k < this_comp->data->ncol; k++) {
        comp_sel[k] = -1;
        name = this_comp->data->collist[k];
        for (p = 0; p < ncomp; p++)
            if (QString::compare(name,compchs[p].ch_name) == 0) {
                comp_sel[k] = p;
                break;
            }
        if (comp_sel[k] < 0) {
            printf("Compensation channel %s not found",name.toUtf8().constData());
            return FAIL;
        }
    }
    printf("\tAll compensation channels found.\n");
    /*
        * Create the preselector
        */
    {
        Eigen::MatrixXf sel = Eigen::MatrixXf::Zero(this_comp->data->ncol, ncomp);
        for (j = 0; j < this_comp->data->ncol; j++)
            sel(j, comp_sel[j]) = 1.0f;
        presel = mne_convert_to_sparse(sel, FIFFTS_MC_RCS, 1e-30f);
        if (!presel)
            return FAIL;
        printf("\tPreselector created.\n");
    }
    /*
     * Pick the desired channels
     */
    for (k = 0; k < nch; k++) {
        if (comps[k] != MNE_CTFV_COMP_NONE)
            names.append(chs[k].ch_name);
    }

    {
        auto d = this_comp->data->pick(names, need_comp, emptyList, 0);
        if (!d)
            return FAIL;
        data = std::move(d);
    }
    printf("\tCompensation data matrix created.\n");
    /*
        * Create the postselector
        */
    {
        Eigen::MatrixXf sel = Eigen::MatrixXf::Zero(nch, data->nrow);
        for (j = 0, p = 0; j < nch; j++) {
            if (comps[j] != MNE_CTFV_COMP_NONE)
                sel(j, p++) = 1.0f;
        }
        postsel = mne_convert_to_sparse(sel, FIFFTS_MC_RCS, 1e-30f);
        if (!postsel)
            return FAIL;
        printf("\tPostselector created.\n");
    }
    current           = std::make_unique<MNECTFCompData>();
    current->kind     = this_comp->kind;
    current->mne_kind = this_comp->mne_kind;
    current->data     = std::move(data);
    current->presel   = std::move(presel);
    current->postsel  = std::move(postsel);

    printf("\tCompensation set up.\n");
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::set_comp(QList<FIFFLIB::FiffChInfo>& chs,
                                int nch,
                                int comp)
/*
     * Set the compensation bits to the desired value
     */
{
    int k;
    int nset;
    for (k = 0, nset = 0; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            chs[k].chpos.coil_type = (chs[k].chpos.coil_type & 0xFFFF) | (comp << 16);
            nset++;
        }
    }
    printf("A new compensation value (%s) was assigned to %d MEG channels.\n",
            explain_comp(map_comp_kind(comp)).toUtf8().constData(),nset);
    return nset;
}

//=============================================================================================================

int MNECTFCompDataSet::apply(int do_it, Eigen::Ref<Eigen::VectorXf> data)
{
    return apply(do_it, data, data);
}

//=============================================================================================================

int MNECTFCompDataSet::apply(int do_it, Eigen::Ref<Eigen::VectorXf> data, Eigen::Ref<const Eigen::VectorXf> compdata)
/*
     * Apply compensation or revert to uncompensated data
     */
{
    MNECTFCompData* this_comp;
    int   ndata = static_cast<int>(data.size());
    int   ncompdata = static_cast<int>(compdata.size());

    if (!current)
        return OK;
    this_comp = current.get();
    /*
       * Dimension checks
       */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
        * Preselection is optional
        */
    const float *presel;
    if (this_comp->presel) {
        if (this_comp->presel_data.size() == 0)
            this_comp->presel_data.resize(this_comp->presel->m);
        if (mne_sparse_vec_mult2_32(this_comp->presel.get(),const_cast<float*>(compdata.data()),this_comp->presel_data.data()) != OK)
            return FAIL;
        presel = this_comp->presel_data.data();
    }
    else
        presel = compdata.data();
    /*
        * This always happens
        */
    if (this_comp->comp_data.size() == 0)
        this_comp->comp_data.resize(this_comp->data->nrow);
    {
        Eigen::Map<const Eigen::VectorXf> preselVec(presel, this_comp->data->ncol);
        Eigen::Map<Eigen::VectorXf> compVec(this_comp->comp_data.data(), this_comp->data->nrow);
        compVec = this_comp->data->data * preselVec;
    }
    /*
        * Optional postselection
        */
    const float *comp;
    if (!this_comp->postsel)
        comp = this_comp->comp_data.data();
    else {
        if (this_comp->postsel_data.size() == 0)
            this_comp->postsel_data.resize(this_comp->postsel->m);
        if (mne_sparse_vec_mult2_32(this_comp->postsel.get(),this_comp->comp_data.data(),this_comp->postsel_data.data()) != OK)
            return FAIL;
        comp = this_comp->postsel_data.data();
    }
    /*
        * Compensate or revert compensation?
        */
    Eigen::Map<const Eigen::VectorXf> compVec(comp, ndata);
    if (do_it)
        data -= compVec;
    else
        data += compVec;
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::apply_transpose(int do_it, Eigen::MatrixXf& data)
/*
     * Apply compensation or revert to uncompensated data
     */
{
    using RowMatrixXf = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    MNECTFCompData* this_comp;
    int   ndata = static_cast<int>(data.rows());
    int   ns    = static_cast<int>(data.cols());
    int   ncompdata  = ndata;

    if (!current)
        return OK;
    this_comp = current.get();
    /*
        * Dimension checks
        */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
        * Preselection is optional — sparse matrix * data
        */
    Eigen::MatrixXf preselMat;
    if (this_comp->presel) {
        preselMat.resize(this_comp->presel->m, ns);
        FiffSparseMatrix* sp = this_comp->presel.get();
        if (sp->coding == FIFFTS_MC_RCS) {
            for (int i = 0; i < sp->m; i++)
                for (int c = 0; c < ns; c++) {
                    float val = 0.0f;
                    for (int j = sp->ptrs[i]; j < sp->ptrs[i+1]; j++)
                        val += sp->data[j] * data(sp->inds[j], c);
                    preselMat(i, c) = val;
                }
        } else if (sp->coding == FIFFTS_MC_CCS) {
            preselMat.setZero();
            for (int c = 0; c < ns; c++)
                for (int i = 0; i < sp->n; i++)
                    for (int j = sp->ptrs[i]; j < sp->ptrs[i+1]; j++)
                        preselMat(sp->inds[j], c) += sp->data[j] * data(i, c);
        } else {
            printf("Unknown sparse matrix storage type: %d", sp->coding);
            return FAIL;
        }
    }
    else {
        preselMat = data;
    }
    /*
        * Compensation: comp = data_matrix * preselMat
        * data_matrix is float** (contiguous row-major via mne_cmatrix)
        */
    Eigen::MatrixXf comp = this_comp->data->data * preselMat;
    /*
        * Optional postselection — sparse matrix * comp
        */
    if (this_comp->postsel) {
        Eigen::MatrixXf postselMat(this_comp->postsel->m, ns);
        FiffSparseMatrix* sp = this_comp->postsel.get();
        if (sp->coding == FIFFTS_MC_RCS) {
            for (int i = 0; i < sp->m; i++)
                for (int c = 0; c < ns; c++) {
                    float val = 0.0f;
                    for (int j = sp->ptrs[i]; j < sp->ptrs[i+1]; j++)
                        val += sp->data[j] * comp(sp->inds[j], c);
                    postselMat(i, c) = val;
                }
        } else if (sp->coding == FIFFTS_MC_CCS) {
            postselMat.setZero();
            for (int c = 0; c < ns; c++)
                for (int i = 0; i < sp->n; i++)
                    for (int j = sp->ptrs[i]; j < sp->ptrs[i+1]; j++)
                        postselMat(sp->inds[j], c) += sp->data[j] * comp(i, c);
        } else {
            printf("Unknown sparse matrix storage type: %d", sp->coding);
            return FAIL;
        }
        comp = std::move(postselMat);
    }
    /*
        * Compensate or revert compensation?
        */
    if (do_it)
        data -= comp;
    else
        data += comp;
    return OK;
}

//=============================================================================================================

int MNECTFCompDataSet::get_comp(const QList<FIFFLIB::FiffChInfo> &chs, int nch)
{
    int res = MNE_CTFV_NOGRAD;
    int first_comp,comp;
    int k;

    for (k = 0, first_comp = -1; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comp = chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (first_comp != comp) {
                printf("Non uniform compensation not supported.");
                return FAIL;
            }
        }
    }
    if (first_comp >= 0)
        res = first_comp;
    return res;
}

//=============================================================================================================

int MNECTFCompDataSet::map_comp_kind(int grad)
/*
     * Simple mapping
     */
{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (grad == compMap[k].grad_comp)
            return compMap[k].ctf_comp;
    return grad;
}

//=============================================================================================================

QString MNECTFCompDataSet::explain_comp(int kind)
{
    static const struct {
        int kind;
        const char *expl;
    } explain[] = { { MNE_CTFV_COMP_NONE,    "uncompensated" },
    { MNE_CTFV_COMP_G1BR,    "first order gradiometer" },
    { MNE_CTFV_COMP_G2BR,    "second order gradiometer" },
    { MNE_CTFV_COMP_G3BR,    "third order gradiometer" },
    { MNE_4DV_COMP1,         "4D comp 1" },
    { MNE_CTFV_COMP_UNKNOWN, "unknown" } };
    int k;

    for (k = 0; explain[k].kind != MNE_CTFV_COMP_UNKNOWN; k++)
        if (explain[k].kind == kind)
            return QString::fromLatin1(explain[k].expl);
    return QString::fromLatin1(explain[k].expl);
}

//=============================================================================================================

int MNECTFCompDataSet::set_compensation(int compensate_to,
                                        QList<FiffChInfo>& chs,
                                        int nchan,
                                        QList<FiffChInfo> comp_chs,
                                        int ncomp_chan)     /* How many */
/*
     * Make data which has the third-order gradient compensation applied
     */
{
    int k;
    int have_comp_chs;
    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    if (comp_chs.isEmpty()) {
        comp_chs = chs;
        ncomp_chan = nchan;
    }
    undo.reset();
    current.reset();
    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
            have_comp_chs++;
    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
        printf("No compensation channels in these data.");
        return FAIL;
    }
    /*
        * Update the 'current' field to reflect the compensation possibly present in the data now
        */
    if (make_comp(chs,nchan,comp_chs,ncomp_chan) == FAIL)
        goto bad;
    /*
        * Are we there already?
        */
    if (current && current->mne_kind == compensate_to) {
        printf("No further compensation necessary (comp = %s)\n",explain_comp(current->kind).toUtf8().constData());
        current.reset();
        return OK;
    }
    undo    = std::move(current);
    if (compensate_to == MNE_CTFV_NOGRAD) {
        printf("No compensation was requested.\n");
        set_comp(chs,nchan,compensate_to);
        return OK;
    }
    if (set_comp(chs,nchan,compensate_to) > 0) {
        if (undo)
            comp_was = undo->mne_kind;
        else
            comp_was = MNE_CTFV_NOGRAD;
        if (make_comp(chs,nchan,comp_chs,ncomp_chan) == FAIL)
            goto bad;
        printf("Compensation set up as requested (%s -> %s).\n",
                explain_comp(map_comp_kind(comp_was)).toUtf8().constData(),
                explain_comp(current->kind).toUtf8().constData());
    }
    return OK;

bad : {
        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
            set_comp(chs,nchan,comp_was);
        return FAIL;
    }
}
