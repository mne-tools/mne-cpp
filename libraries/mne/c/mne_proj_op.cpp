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

#include <QFile>

#include <Eigen/Core>

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

#define MALLOC_23(x,t) (t *)malloc((x)*sizeof(t))

#define REALLOC_23(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_23(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_23(m) mne_free_cmatrix_23((m))

void mne_free_cmatrix_23 (float **m)
{
    if (m) {
        FREE_23(*m);
        FREE_23(m);
    }
}

#define ALLOC_CMATRIX_23(x,y) mne_cmatrix_23((x),(y))

static void matrix_error_23(int kind, int nr, int nc)

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

float **mne_cmatrix_23(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_23(nr,float *);
    if (!m) matrix_error_23(1,nr,nc);
    whole = MALLOC_23(nr*nc,float);
    if (!whole) matrix_error_23(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

float mne_dot_vectors_23 (float *v1,
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

//============================= mne_named_matrix.c =============================

void mne_string_to_name_list_23(const QString& s, QStringList& listp,int &nlistp)
/*
      * Convert a colon-separated list into a string array
      */
{
    QStringList list;

    if (!s.isEmpty() && s.size() > 0) {
        list = FIFFLIB::FiffStream::split_name_list(s);
        //list = s.split(":");
    }
    listp  = list;
    nlistp = list.size();
    return;
}

void fromFloatEigenMatrix_23(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_23(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_23(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

QString mne_name_list_to_string_23(const QStringList& list)
/*
 * Convert a string array to a colon-separated string
 */
{
    int nlist = list.size();
    QString res;
    if (nlist == 0 || list.isEmpty())
        return res;
//    res[0] = '\0';
    for (int k = 0; k < nlist-1; k++) {
        res += list[k];
        res += ":";
    }
    res += list[nlist-1];
    return res;
}

QString mne_channel_names_to_string_23(const QList<FIFFLIB::FiffChInfo>& chs, int nch)
/*
 * Make a colon-separated string out of channel names
 */
{
    QStringList names;
    QString res;
    if (nch <= 0)
        return res;
    for (int k = 0; k < nch; k++)
        names.append(chs.at(k).ch_name);
    res = mne_name_list_to_string_23(names);
    return res;
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneProjOp::MneProjOp()
: nitems (0)
, names (NULL)
, nch (0)
, nvec (0)
, proj_data (NULL)
{
}

//=============================================================================================================

MneProjOp::~MneProjOp()
{
    // mne_free_proj_op
    for (int k = 0; k < nitems; k++)
        if(items[k])
            delete items[k];

    // mne_free_proj_op_proj
}

//=============================================================================================================

void MneProjOp::mne_free_proj_op_proj(MneProjOp *op)
{
    if (op == NULL)
        return;

    FREE_CMATRIX_23(op->proj_data);

    op->names.clear();
    op->nch  = 0;
    op->nvec = 0;
    op->proj_data = NULL;

    return;
}

//=============================================================================================================

MneProjOp *MneProjOp::mne_proj_op_combine(MneProjOp *to, MneProjOp *from)
/*
     * Copy items from 'from' operator to 'to' operator
     */
{
    int k;
    MneProjItem* it;

    if (to == NULL)
        to = new MneProjOp();
    if (from) {
        for (k = 0; k < from->nitems; k++) {
            it = from->items[k];
            mne_proj_op_add_item(to,it->vecs,it->kind,it->desc);
            to->items[to->nitems-1]->active_file = it->active_file;
        }
    }
    return to;
}

//=============================================================================================================

void MneProjOp::mne_proj_op_add_item_act(MneProjOp *op, MneNamedMatrix *vecs, int kind, const QString& desc, int is_active)
/*
 * Add a new item to an existing projection operator
 */
{
    MneProjItem* new_item;
    int         k;

    //    op->items = REALLOC(op->items,op->nitems+1,mneProjItem);
    //    op->items[op->nitems] = new_item = new MneProjItem();

    new_item = new MneProjItem();
    op->items.append(new_item);

    new_item->active      = is_active;
    new_item->vecs        = new MneNamedMatrix(*vecs);

    if (kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
        new_item->has_meg = FALSE;
        new_item->has_eeg = TRUE;
    }
    else {
        for (k = 0; k < vecs->ncol; k++) {
            if (vecs->collist[k].contains("EEG"))//strstr(vecs->collist[k],"EEG") == vecs->collist[k])
                new_item->has_eeg = TRUE;
            if (vecs->collist[k].contains("MEG"))//strstr(vecs->collist[k],"MEG") == vecs->collist[k])
                new_item->has_meg = TRUE;
        }
        if (!new_item->has_meg && !new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
        else if (new_item->has_meg && new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
    }
    if (!desc.isEmpty())
        new_item->desc = desc;
    new_item->kind = kind;
    new_item->nvec = new_item->vecs->nrow;

    op->nitems++;

    MneProjOp::mne_free_proj_op_proj(op);  /* These data are not valid any more */
    return;
}

//=============================================================================================================

void MneProjOp::mne_proj_op_add_item(MneProjOp *op, MneNamedMatrix *vecs, int kind, const QString& desc)
{
    mne_proj_op_add_item_act(op, vecs, kind, desc, TRUE);
}

//=============================================================================================================

MneProjOp *MneProjOp::mne_dup_proj_op(MneProjOp *op)
/*
 * Provide a duplicate (item data only)
 */
{
    MneProjOp* dup = new MneProjOp();
    MneProjItem* it;
    int k;

    if (!op)
        return NULL;

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        mne_proj_op_add_item_act(dup,it->vecs,it->kind,it->desc,it->active);
        dup->items[k]->active_file = it->active_file;
    }
    return dup;
}

//=============================================================================================================

MneProjOp *MneProjOp::mne_proj_op_average_eeg_ref(const QList<FiffChInfo>& chs, int nch)
/*
     * Make the projection operator for average electrode reference
     */
{
    int eegcount = 0;
    int k;
    float       **vec_data;
    QStringList     names;
    MneNamedMatrix* vecs;
    MneProjOp*      op;

    for (k = 0; k < nch; k++)
        if (chs.at(k).kind == FIFFV_EEG_CH)
            eegcount++;
    if (eegcount == 0) {
        qCritical("No EEG channels specified for average reference.");
        return NULL;
    }

    vec_data = ALLOC_CMATRIX_23(1,eegcount);

    for (k = 0; k < nch; k++)
        if (chs.at(k).kind == FIFFV_EEG_CH)
            names.append(chs.at(k).ch_name);

    for (k = 0; k < eegcount; k++)
        vec_data[0][k] = 1.0/sqrt((double)eegcount);

    QStringList emptyList;
    vecs = MneNamedMatrix::build_named_matrix(1,eegcount,emptyList,names,vec_data);

    op = new MneProjOp();
    mne_proj_op_add_item(op,vecs,FIFFV_MNE_PROJ_ITEM_EEG_AVREF,"Average EEG reference");

    return op;
}

//=============================================================================================================

int MneProjOp::mne_proj_op_affect(MneProjOp *op, const QStringList& list, int nlist)
{
    int k;
    int naff;

    if (!op)
        return 0;

    for (k = 0, naff = 0; k < op->nitems; k++)
        if (op->items[k]->active && MneProjItem::mne_proj_item_affect(op->items[k],list,nlist))
            naff += op->items[k]->nvec;

    return naff;
}

//=============================================================================================================

int MneProjOp::mne_proj_op_affect_chs(MneProjOp *op, const QList<FiffChInfo>& chs, int nch)
{
    QString ch_string;
    int  res;
    QStringList list;
    int  nlist;

    if (nch == 0)
        return FALSE;
    ch_string = mne_channel_names_to_string_23(chs,nch);
    mne_string_to_name_list_23(ch_string,list,nlist);
    res = mne_proj_op_affect(op,list,nlist);
    list.clear();
    return res;
}

//=============================================================================================================

int MneProjOp::mne_proj_op_proj_vector(MneProjOp *op, float *vec, int nvec, int do_complement)
/*
     * Apply projection operator to a vector (floats)
     * Assume that all dimension checking etc. has been done before
     */
{
    static float *res = NULL;
    int    res_size   = 0;
    float *pvec;
    float  w;
    int k,p;

    if (!op || op->nitems <= 0 || op->nvec <= 0)
        return OK;

    if (op->nch != nvec) {
        printf("Data vector size does not match projection operator");
        return FAIL;
    }

    if (op->nch > res_size) {
        res = REALLOC_23(res,op->nch,float);
        res_size = op->nch;
    }

    for (k = 0; k < op->nch; k++)
        res[k] = 0.0;

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = mne_dot_vectors_23(pvec,vec,op->nch);
        for (k = 0; k < op->nch; k++)
            res[k] = res[k] + w*pvec[k];
    }
    if (do_complement) {
        for (k = 0; k < op->nch; k++)
            vec[k] = vec[k] - res[k];
    }
    else {
        for (k = 0; k < op->nch; k++)
            vec[k] = res[k];
    }
    return OK;
}

//=============================================================================================================

MneProjOp *MneProjOp::mne_read_proj_op_from_node(FiffStream::SPtr &stream, const FiffDirNode::SPtr &start)
/*
     * Load all the linear projection data
     */
{
    MneProjOp*   op     = NULL;
    QList<FiffDirNode::SPtr> proj;
    FiffDirNode::SPtr start_node;
    QList<FiffDirNode::SPtr> items;
    FiffDirNode::SPtr node;
    int         k;
    QString     item_desc,desc_tag;
    int         global_nchan,item_nchan,nlist;
    QStringList item_names;
    int         item_kind;
    float       **item_vectors = NULL;
    int         item_nvec;
    int         item_active;
    MneNamedMatrix* item;
    FiffTag::SPtr t_pTag;

    if (!stream) {
        qCritical("File not open mne_read_proj_op_from_node");
        goto bad;
    }

    if (!start || start->isEmpty())
        start_node = stream->dirtree();
    else
        start_node = start;

    op = new MneProjOp();
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

        MatrixXf tmp_item_vectors = t_pTag->toFloatMatrix().transpose();
        item_vectors = ALLOC_CMATRIX_23(tmp_item_vectors.rows(),tmp_item_vectors.cols());
        fromFloatEigenMatrix_23(tmp_item_vectors, item_vectors);

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
        item = MneNamedMatrix::build_named_matrix(item_nvec,item_nchan,emptyList,item_names,item_vectors);
        mne_proj_op_add_item_act(op,item,item_kind,item_desc,item_active);
        delete item;
        op->items[op->nitems-1]->active_file = item_active;
    }

out :
    return op;

bad : {
        if(op)
            delete op;
        return NULL;
    }
}

//=============================================================================================================

MneProjOp *MneProjOp::mne_read_proj_op(const QString &name)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if(!stream->open())
        return NULL;

    MneProjOp*  res = NULL;

    FiffDirNode::SPtr t_default;
    res = mne_read_proj_op_from_node(stream,t_default);

    stream->close();

    return res;
}

//=============================================================================================================

void MneProjOp::mne_proj_op_report_data(FILE *out, const char *tag, MneProjOp *op, int list_data, char **exclude, int nexclude)
/*
     * Output info about the projection operator
     */
{
    int j,k,p,q;
    MneProjItem* it;
    MneNamedMatrix* vecs;
    int found;

    if (out == NULL)
        return;
    if (op == NULL)
        return;
    if (op->nitems <= 0) {
        fprintf(out,"Empty operator\n");
        return;
    }

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (tag)
            fprintf(out,"%s",tag);
        fprintf(out,"# %d : %s : %d vecs : %d chs %s %s\n",
                k+1,it->desc.toUtf8().constData(),it->nvec,it->vecs->ncol,
                it->has_meg ? "MEG" : "EEG",
                it->active ? "active" : "idle");
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (list_data) {
            vecs = op->items[k]->vecs;

            for (q = 0; q < vecs->ncol; q++) {
                fprintf(out,"%-10s",vecs->collist[q].toUtf8().constData());
                fprintf(out,q < vecs->ncol-1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow; p++)
                for (q = 0; q < vecs->ncol; q++) {
                    for (j = 0, found  = 0; j < nexclude; j++) {
                        if (QString::compare(exclude[j],vecs->collist[q]) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    fprintf(out,"%10.5g ",found ? 0.0 : vecs->data[p][q]);
                    fprintf(out,q < vecs->ncol-1 ? " " : "\n");
                }
            if (list_data && tag)
                fprintf(out,"%s\n",tag);
        }
    }
    return;
}

//=============================================================================================================

void MneProjOp::mne_proj_op_report(FILE *out, const char *tag, MneProjOp *op)
{
    mne_proj_op_report_data(out,tag,op, FALSE, NULL, 0);
}
