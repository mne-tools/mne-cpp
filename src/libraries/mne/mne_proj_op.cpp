//=============================================================================================================
/**
 * @file     mne_proj_op.cpp
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
 * @brief    Definition of the MNEProjOp Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_tag.h>

#include "mne_proj_op.h"
#include "mne_proj_item.h"
#include "mne_cov_matrix.h"
#include "mne_named_vector.h"
#include "mne_named_matrix.h"

#include <QFile>
#include <QTextStream>

#include <Eigen/Core>
#include <Eigen/SVD>

#include <vector>

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

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEProjOp::MNEProjOp()
: nitems (0)
, nch (0)
, nvec (0)
{
}

//=============================================================================================================

MNEProjOp::~MNEProjOp()
{
}

//=============================================================================================================

void MNEProjOp::free_proj()
{
    proj_data.resize(0, 0);

    names.clear();
    nch  = 0;
    nvec = 0;

    return;
}

//=============================================================================================================

MNEProjOp *MNEProjOp::combine(MNEProjOp *from)
/*
     * Copy items from 'from' operator to this operator
     */
{
    if (from) {
        for (int k = 0; k < from->nitems; k++) {
            const auto& it = from->items[k];
            add_item(it.vecs.get(),it.kind,it.desc);
            items[nitems-1].active_file = it.active_file;
        }
    }
    return this;
}

//=============================================================================================================

void MNEProjOp::add_item_active(const MNENamedMatrix *vecs, int kind, const QString& desc, int is_active)
/*
 * Add a new item to an existing projection operator
 */
{
    items.append(MNEProjItem());
    auto& new_item = items.back();

    new_item.active      = is_active;
    new_item.vecs        = std::make_unique<MNENamedMatrix>(*vecs);

    if (kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
        new_item.has_meg = FALSE;
        new_item.has_eeg = TRUE;
    }
    else {
        for (int k = 0; k < vecs->ncol; k++) {
            if (vecs->collist[k].contains("EEG"))//strstr(vecs->collist[k],"EEG") == vecs->collist[k])
                new_item.has_eeg = TRUE;
            if (vecs->collist[k].contains("MEG"))//strstr(vecs->collist[k],"MEG") == vecs->collist[k])
                new_item.has_meg = TRUE;
        }
        if (!new_item.has_meg && !new_item.has_eeg) {
            new_item.has_meg = TRUE;
            new_item.has_eeg = FALSE;
        }
        else if (new_item.has_meg && new_item.has_eeg) {
            new_item.has_meg = TRUE;
            new_item.has_eeg = FALSE;
        }
    }
    if (!desc.isEmpty())
        new_item.desc = desc;
    new_item.kind = kind;
    new_item.nvec = new_item.vecs->nrow;

    nitems++;

    free_proj();  /* These data are not valid any more */
    return;
}

//=============================================================================================================

void MNEProjOp::add_item(const MNENamedMatrix *vecs, int kind, const QString& desc)
{
    add_item_active(vecs, kind, desc, TRUE);
}

//=============================================================================================================

MNEProjOp *MNEProjOp::dup() const
/*
 * Provide a duplicate (item data only)
 */
{
    MNEProjOp* res = new MNEProjOp();

    for (int k = 0; k < nitems; k++) {
        const auto& it = items[k];
        res->add_item_active(it.vecs.get(),it.kind,it.desc,it.active);
        res->items[k].active_file = it.active_file;
    }
    return res;
}

//=============================================================================================================

MNEProjOp *MNEProjOp::create_average_eeg_ref(const QList<FiffChInfo>& chs, int nch)
/*
     * Make the projection operator for average electrode reference
     */
{
    int eegcount = 0;
    int k;
    QStringList     names;
    MNEProjOp*      op;

    for (k = 0; k < nch; k++)
        if (chs.at(k).kind == FIFFV_EEG_CH)
            eegcount++;
    if (eegcount == 0) {
        qCritical("No EEG channels specified for average reference.");
        return nullptr;
    }

    for (k = 0; k < nch; k++)
        if (chs.at(k).kind == FIFFV_EEG_CH)
            names.append(chs.at(k).ch_name);

    Eigen::MatrixXf vec_data = Eigen::MatrixXf::Constant(1, eegcount, 1.0f/sqrt((double)eegcount));

    QStringList emptyList;
    auto vecs = MNENamedMatrix::build(1,eegcount,emptyList,names,vec_data);

    op = new MNEProjOp();
    op->add_item(vecs.get(),FIFFV_MNE_PROJ_ITEM_EEG_AVREF,"Average EEG reference");

    return op;
}

//=============================================================================================================

int MNEProjOp::affect(const QStringList& list, int nlist)
{
    int k;
    int naff;

    for (k = 0, naff = 0; k < nitems; k++)
        if (items[k].active && items[k].affect(list,nlist))
            naff += items[k].nvec;

    return naff;
}

//=============================================================================================================

int MNEProjOp::affect_chs(const QList<FiffChInfo>& chs, int nch)
{
    if (nch == 0)
        return FALSE;
    QStringList list;
    list.reserve(nch);
    for (int k = 0; k < nch; k++)
        list.append(chs.at(k).ch_name);
    return affect(list, nch);
}

//=============================================================================================================

int MNEProjOp::project_vector(float *vec, int nvec, int do_complement)
/*
     * Apply projection operator to a vector (floats)
     * Assume that all dimension checking etc. has been done before
     */
{
    if (nitems <= 0 || this->nvec <= 0)
        return OK;

    if (nch != nvec) {
        printf("Data vector size does not match projection operator");
        return FAIL;
    }

    Eigen::Map<Eigen::VectorXf> v(vec, nch);
    Eigen::VectorXf proj = Eigen::VectorXf::Zero(nch);

    for (int p = 0; p < this->nvec; p++) {
        auto row = proj_data.row(p);
        float w = row.dot(v);
        proj += w * row.transpose();
    }

    if (do_complement)
        v -= proj;
    else
        v = proj;

    return OK;
}

//=============================================================================================================

MNEProjOp *MNEProjOp::read_from_node(FiffStream::SPtr &stream, const FiffDirNode::SPtr &start)
/*
     * Load all the linear projection data
     */
{
    MNEProjOp*   op     = nullptr;
    QList<FiffDirNode::SPtr> proj;
    FiffDirNode::SPtr start_node;
    QList<FiffDirNode::SPtr> items;
    FiffDirNode::SPtr node;
    int         k;
    QString     item_desc,desc_tag;
    int         global_nchan,item_nchan;
    QStringList item_names;
    int         item_kind;
    int         item_nvec;
    int         item_active;
    FiffTag::UPtr t_pTag;

    if (!stream) {
        qCritical("File not open read_from_node");
        goto bad;
    }

    if (!start || start->isEmpty())
        start_node = stream->dirtree();
    else
        start_node = start;

    op = new MNEProjOp();
    proj = start_node->dir_tree_find(FIFFB_PROJ);
    if (proj.size() == 0 || proj[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
        * Only the first projection block is recognized
        */
    items = proj[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    if (items.size() == 0 || items[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
        * Get a common number of channels
        */
    node = proj[0];
    if(!node->find_tag(stream, FIFF_NCHAN, t_pTag))
        global_nchan = 0;
    else {
        global_nchan = *t_pTag->toInt();
        //        TAG_FREE(tag);
    }
    /*
       * Proceess each item
       */
    for (k = 0; k < items.size(); k++) {
        node = items[k];
        /*
        * Complicated procedure for getting the description
        */
        item_desc.clear();

        if (node->find_tag(stream, FIFF_NAME, t_pTag)) {
            item_desc += t_pTag->toString();
        }

        /*
            * Take the first line of description if it exists
            */
        if (node->find_tag(stream, FIFF_DESCRIPTION, t_pTag)) {
            desc_tag = t_pTag->toString();
            int pos;
            if((pos = desc_tag.indexOf("\n")) >= 0)
                desc_tag.truncate(pos);
            if (!item_desc.isEmpty())
                item_desc += " ";
            item_desc += desc_tag;
        }
        /*
        * Possibility to override number of channels here
        */
        if (!node->find_tag(stream, FIFF_NCHAN, t_pTag)) {
            item_nchan = global_nchan;
        }
        else {
            item_nchan = *t_pTag->toInt();
        }
        if (item_nchan <= 0) {
            qCritical("Number of channels incorrectly specified for one of the projection items.");
            goto bad;
        }
        /*
            * Take care of the channel names
            */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag))
            goto bad;

        item_names = FiffStream::split_name_list(t_pTag->toString());

        if (item_names.size() != item_nchan) {
            printf("Channel name list incorrectly specified for proj item # %d",k+1);
            item_names.clear();
            goto bad;
        }
        /*
            * Kind of item
            */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_KIND, t_pTag))
            goto bad;
        item_kind = *t_pTag->toInt();
        /*
            * How many vectors
            */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_NVEC, t_pTag))
            goto bad;
        item_nvec = *t_pTag->toInt();
        /*
            * The projection data
            */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_VECTORS, t_pTag))
            goto bad;

        MatrixXf item_vectors = t_pTag->toFloatMatrix().transpose();

        /*
            * Is this item active?
            */
        if (node->find_tag(stream, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag)) {
            item_active = *t_pTag->toInt();
        }
        else
            item_active = FALSE;
        /*
        * Ready to add
        */
        QStringList emptyList;
        auto item = MNENamedMatrix::build(item_nvec,item_nchan,emptyList,item_names,item_vectors);
        op->add_item_active(item.get(),item_kind,item_desc,item_active);
        op->items[op->nitems-1].active_file = item_active;
    }

out :
    return op;

bad : {
        if(op)
            delete op;
        return nullptr;
    }
}

//=============================================================================================================

MNEProjOp *MNEProjOp::read(const QString &name)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if(!stream->open())
        return nullptr;

    MNEProjOp*  res = nullptr;

    FiffDirNode::SPtr t_default;
    res = read_from_node(stream,t_default);

    stream->close();

    return res;
}

//=============================================================================================================

void MNEProjOp::report_data(QTextStream &out, const QString &tag, bool list_data, const QStringList &exclude)
/*
     * Output info about the projection operator
     */
{
    int j,p,q;
    MNENamedMatrix* vecs;
    bool found;

    if (nitems <= 0) {
        out << "Empty operator\n";
        return;
    }

    for (int k = 0; k < nitems; k++) {
        const auto& it = items[k];
        if (list_data && !tag.isEmpty())
            out << tag << "\n";
        if (!tag.isEmpty())
            out << tag;
        out << "# " << (k+1) << " : " << it.desc << " : " << it.nvec << " vecs : " << it.vecs->ncol << " chs "
            << (it.has_meg ? "MEG" : "EEG") << " "
            << (it.active ? "active" : "idle") << "\n";
        if (list_data && !tag.isEmpty())
            out << tag << "\n";
        if (list_data) {
            vecs = items[k].vecs.get();

            for (q = 0; q < vecs->ncol; q++) {
                out << qSetFieldWidth(10) << Qt::left << vecs->collist[q] << qSetFieldWidth(0);
                out << (q < vecs->ncol-1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow; p++)
                for (q = 0; q < vecs->ncol; q++) {
                    found = exclude.contains(vecs->collist[q]);
                    out << qSetFieldWidth(10) << qSetRealNumberPrecision(5) << Qt::forcepoint
                        << (found ? 0.0 : vecs->data(p, q)) << qSetFieldWidth(0) << " ";
                    out << (q < vecs->ncol-1 ? " " : "\n");
                }
            if (list_data && !tag.isEmpty())
                out << tag << "\n";
        }
    }
    return;
}

//=============================================================================================================

void MNEProjOp::report(QTextStream &out, const QString &tag)
{
    report_data(out, tag, false, QStringList());
}

//=============================================================================================================

int MNEProjOp::assign_channels(const QStringList& list, int nlist)
{
    free_proj();  /* Compiled data is no longer valid */

    if (nlist == 0)
        return OK;

    names = list;
    nch   = nlist;

    return OK;
}

//=============================================================================================================

namespace {

void clear_channel_group(Eigen::Ref<Eigen::VectorXf> data, const QStringList& ch_names, int nnames, const QString& prefix)
{
    for (int k = 0; k < nnames; k++)
        if (ch_names[k].contains(prefix))
            data[k] = 0.0;
}

constexpr float USE_LIMIT   = 1e-5f;
constexpr float SMALL_VALUE = 1e-4f;

} // anonymous namespace

int MNEProjOp::make_proj_bad(const QStringList& bad)
{
    using RowMatrixXf = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    int   k,p,q,r,nvec_total;
    RowMatrixXf vv_meg_mat;
    Eigen::VectorXf sing_meg_vec;
    RowMatrixXf vv_eeg_mat;
    Eigen::VectorXf sing_eeg_vec;
    int   nvec_meg;
    int   nvec_eeg;
    MNENamedVector vec;
    float size;
    int   nzero;

    proj_data.resize(0, 0);
    nvec      = 0;

    if (nch <= 0)
        return OK;
    if (nitems <= 0)
        return OK;

    nvec_total = affect(names,nch);
    if (nvec_total == 0)
        return OK;

    RowMatrixXf mat_meg_mat = RowMatrixXf::Zero(nvec_total, nch);
    RowMatrixXf mat_eeg_mat = RowMatrixXf::Zero(nvec_total, nch);

    for (k = 0, nvec_meg = nvec_eeg = 0; k < nitems; k++) {
        if (items[k].active && items[k].affect(names,nch)) {
            vec.nvec  = items[k].vecs->ncol;
            vec.names = items[k].vecs->collist;
            if (items[k].has_meg) {
                for (p = 0; p < items[k].nvec; p++, nvec_meg++) {
                    vec.data = items[k].vecs->data.row(p);
                    Eigen::Map<Eigen::VectorXf> res_meg(mat_meg_mat.row(nvec_meg).data(), nch);
                    if (vec.pick(names,nch,false,res_meg) == FAIL)
                        return FAIL;
                }
            }
            else if (items[k].has_eeg) {
                for (p = 0; p < items[k].nvec; p++, nvec_eeg++) {
                    vec.data = items[k].vecs->data.row(p);
                    Eigen::Map<Eigen::VectorXf> res_eeg(mat_eeg_mat.row(nvec_eeg).data(), nch);
                    if (vec.pick(names,nch,false,res_eeg) == FAIL)
                        return FAIL;
                }
            }
        }
    }
    /*
     * Replace bad channel entries with zeroes
     */
    for (q = 0; q < bad.size(); q++)
        for (r = 0; r < nch; r++)
            if (names[r] == bad[q]) {
                for (p = 0; p < nvec_meg; p++)
                    mat_meg_mat(p,r) = 0.0;
                for (p = 0; p < nvec_eeg; p++)
                    mat_eeg_mat(p,r) = 0.0;
            }
    /*
     * Scale the rows so that detection of linear dependence becomes easy
     */
    for (p = 0, nzero = 0; p < nvec_meg; p++) {
        size = mat_meg_mat.row(p).norm();
        if (size > 0) {
            mat_meg_mat.row(p) /= size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_meg) {
        mat_meg_mat.resize(0, 0); nvec_meg = 0;
    }
    for (p = 0, nzero = 0; p < nvec_eeg; p++) {
        size = mat_eeg_mat.row(p).norm();
        if (size > 0) {
            mat_eeg_mat.row(p) /= size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_eeg) {
        mat_eeg_mat.resize(0, 0); nvec_eeg = 0;
    }
    if (nvec_meg + nvec_eeg == 0) {
        qWarning("No projection remains after excluding bad channels. Omitting projection.");
        return OK;
    }
    /*
     * Proceed to SVD
     */
    if (nvec_meg > 0) {
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(mat_meg_mat.topRows(nvec_meg), Eigen::ComputeFullV);
        sing_meg_vec = svd.singularValues();
        vv_meg_mat = svd.matrixV().transpose().topRows(nvec_meg);
    }
    if (nvec_eeg > 0) {
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(mat_eeg_mat.topRows(nvec_eeg), Eigen::ComputeFullV);
        sing_eeg_vec = svd.singularValues();
        vv_eeg_mat = svd.matrixV().transpose().topRows(nvec_eeg);
    }
    /*
     * Check for linearly dependent vectors
     */
    for (p = 0, nvec = 0; p < nvec_meg; p++, nvec++)
        if (sing_meg_vec[p]/sing_meg_vec[0] < USE_LIMIT)
            break;
    for (p = 0; p < nvec_eeg; p++, nvec++)
        if (sing_eeg_vec[p]/sing_eeg_vec[0] < USE_LIMIT)
            break;
    proj_data = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>::Zero(nvec,nch);
    for (p = 0, nvec = 0; p < nvec_meg; p++, nvec++) {
        if (sing_meg_vec[p]/sing_meg_vec[0] < USE_LIMIT)
            break;
        for (k = 0; k < nch; k++) {
            if (std::fabs(vv_meg_mat(p,k)) < SMALL_VALUE)
                proj_data(nvec,k) = 0.0;
            else
                proj_data(nvec,k) = vv_meg_mat(p,k);
            clear_channel_group(Eigen::Map<Eigen::VectorXf>(proj_data.row(nvec).data(), nch),names,nch,"EEG");
        }
    }
    for (p = 0; p < nvec_eeg; p++, nvec++) {
        if (sing_eeg_vec[p]/sing_eeg_vec[0] < USE_LIMIT)
            break;
        for (k = 0; k < nch; k++) {
            if (std::fabs(vv_eeg_mat(p,k)) < SMALL_VALUE)
                proj_data(nvec,k) = 0.0;
            else
                proj_data(nvec,k) = vv_eeg_mat(p,k);
            clear_channel_group(Eigen::Map<Eigen::VectorXf>(proj_data.row(nvec).data(), nch),names,nch,"MEG");
        }
    }
    /*
     * Make sure that the stimulus channels are not modified
     */
    for (k = 0; k < nch; k++)
        if (names[k].contains("STI")) {
            for (p = 0; p < nvec; p++)
                proj_data(p,k) = 0.0;
        }

    return OK;
}

//=============================================================================================================

int MNEProjOp::make_proj()
{
    return make_proj_bad(QStringList());
}

//=============================================================================================================

int MNEProjOp::project_dvector(Eigen::Ref<Eigen::VectorXd> vec, int vec_nch, int do_complement)
{
    if (nvec <= 0)
        return OK;

    if (nch != vec_nch) {
        qCritical("Data vector size does not match projection operator");
        return FAIL;
    }

    Eigen::VectorXd proj = Eigen::VectorXd::Zero(vec_nch);

    for (int p = 0; p < nvec; p++) {
        double w = vec.dot(proj_data.row(p).cast<double>());
        proj += w * proj_data.row(p).cast<double>().transpose();
    }

    if (do_complement)
        vec -= proj;
    else
        vec = proj;

    return OK;
}

//=============================================================================================================

bool MNEProjOp::makeProjection(const QList<QString>& projnames,
                               const QList<FiffChInfo>& chs,
                               int nch,
                               std::unique_ptr<MNEProjOp>& result)
{
    result.reset();
    int neeg = 0;

    for (int k = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;

    if (projnames.size() == 0 && neeg == 0)
        return true;

    std::unique_ptr<MNEProjOp> all;

    for (int k = 0; k < projnames.size(); k++) {
        std::unique_ptr<MNEProjOp> one(MNEProjOp::read(projnames[k]));
        if (!one) {
            qCritical("Failed to read projection from %s.", projnames[k].toUtf8().data());
            return false;
        }
        if (one->nitems == 0) {
            qInfo("No linear projection information in %s.", projnames[k].toUtf8().data());
        }
        else {
            qInfo("Loaded projection from %s:", projnames[k].toUtf8().data());
            { QTextStream errStream(stderr); one->report(errStream, QStringLiteral("\t")); }
            if (!all)
                all = std::make_unique<MNEProjOp>();
            all->combine(one.get());
        }
    }

    if (neeg > 0) {
        bool found = false;
        if (all) {
            for (int k = 0; k < all->nitems; k++)
                if (all->items[k].kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
                    found = true;
                    break;
                }
        }
        if (!found) {
            std::unique_ptr<MNEProjOp> one(MNEProjOp::create_average_eeg_ref(chs, nch));
            if (one) {
                qInfo("Average EEG reference projection added:");
                { QTextStream errStream(stderr); one->report(errStream, QStringLiteral("\t")); }
                if (!all)
                    all = std::make_unique<MNEProjOp>();
                all->combine(one.get());
            }
        }
    }
    if (all && all->affect_chs(chs, nch) == 0) {
        qInfo("Projection will not have any effect on selected channels. Projection omitted.");
        all.reset();
    }
    result = std::move(all);
    return true;
}

//=============================================================================================================

int MNEProjOp::apply_cov(MNECovMatrix* c)
{
    using RowMatrixXd = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    int j,k,p;
    int do_complement = true;

    if (nitems == 0)
        return OK;

    if (nch != c->ncov || names != c->names) {
        qCritical("Incompatible data in apply_cov");
        return FAIL;
    }

    RowMatrixXd dcovMat = RowMatrixXd::Zero(c->ncov,c->ncov);

    if (c->cov_diag.size() > 0) {
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k < c->ncov; k++)
                dcovMat(j,k) = (j == k) ? c->cov_diag[j] : 0;
    }
    else {
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                dcovMat(j,k) = c->cov[p++];
        for (j = 0; j < c->ncov; j++)
            for (k = j+1; k < c->ncov; k++)
                dcovMat(j,k) = dcovMat(k,j);
    }

    for (k = 0; k < c->ncov; k++) {
        Eigen::Map<Eigen::VectorXd> row_k(dcovMat.row(k).data(), c->ncov);
        if (project_dvector(row_k,c->ncov,do_complement) != OK)
            return FAIL;
    }

    dcovMat.transposeInPlace();

    for (k = 0; k < c->ncov; k++) {
        Eigen::Map<Eigen::VectorXd> row_k(dcovMat.row(k).data(), c->ncov);
        if (project_dvector(row_k,c->ncov,do_complement) != OK)
            return FAIL;
    }

    if (c->cov_diag.size() > 0) {
        for (j = 0; j < c->ncov; j++) {
            c->cov_diag[j] = dcovMat(j,j);
        }
        c->cov.resize(0);
    }
    else {
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                c->cov[p++] = dcovMat(j,k);
    }

    c->nproj = affect(c->names,c->ncov);
    return OK;
}
