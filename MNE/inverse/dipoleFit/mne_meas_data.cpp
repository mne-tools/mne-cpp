//=============================================================================================================
/**
* @file     mne_meas_data.cpp
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
* @brief    Implementation of the MNE Meas Data (MneMeasData) Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data.h"
#include "mne_meas_data_set.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;



#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define FREE_9(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_9(m) mne_free_cmatrix_9((m))


void mne_free_cmatrix_9 (float **m)
{
    if (m) {
        FREE_9(*m);
        FREE_9(m);
    }
}


void mne_free_name_list_9(char **list, int nlist)
/*
* Free a name list array
*/
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        fprintf(stderr,"%d %s\n",k,list[k]);
#endif
        FREE_9(list[k]);
    }
    FREE_9(list);
    return;
}



void mne_free_proj_op_proj_9(mneProjOp op)

{
    if (op == NULL)
        return;

    mne_free_name_list_9(op->names,op->nch);
    FREE_CMATRIX_9(op->proj_data);

    op->names  = NULL;
    op->nch  = 0;
    op->nvec = 0;
    op->proj_data = NULL;

    return;
}

void mne_free_proj_op_9(mneProjOp op)

{
    int k;

    if (op == NULL)
        return;

    for (k = 0; k < op->nitems; k++)
        if(op->items[k])
            delete op->items[k];
//    FREE_9(op->items);

    mne_free_proj_op_proj_9(op);

    FREE_9(op);
    return;
}



void mne_free_ctf_comp_data_9(mneCTFcompData comp)

{
    if (!comp)
        return;

    if(comp->data)
        delete comp->data;
    if(comp->presel)
        delete comp->presel;
    if(comp->postsel)
        delete comp->postsel;
    FREE_9(comp->presel_data);
    FREE_9(comp->postsel_data);
    FREE_9(comp->comp_data);
    FREE_9(comp);
    return;
}


void mne_free_ctf_comp_data_set_9(mneCTFcompDataSet set)

{
    int k;

    if (!set)
        return;

    for (k = 0; k < set->ncomp; k++)
        mne_free_ctf_comp_data_9(set->comps[k]);
    FREE_9(set->comps);
    FREE_9(set->chs);
    mne_free_ctf_comp_data_9(set->current);
    FREE_9(set);
    return;
}


typedef struct {
    int   size;		        /* Size of this buffer in floats */
    float *data;			/* The allocated buffer */
    float ***datap;		/* The matrix which uses this */
} *ringBufBuf_9,ringBufBufRec_9;

typedef struct {
    ringBufBuf_9 *bufs;
    int        nbuf;
    int        next;
} *ringBuf_9,ringBufRec_9;


void mne_free_ring_buffer_9(void *thisp)

{
    int k;
    ringBuf_9 this_buf = (ringBuf_9)thisp;

    if (!this_buf)
        return;

    for (k = 0; k < this_buf->nbuf; k++)
        FREE_9(this_buf->bufs[k]->data);
    FREE_9(this_buf->bufs);
    FREE_9(this_buf);
    return;
}


static void free_bufs_9(mneRawBufDef bufs, int nbuf)

{
    int k;
    for (k = 0; k < nbuf; k++) {
        FREE_9(bufs[k].ch_filtered);
        /*
     * Clear the pointers only, not the data which are in the ringbuffer
     */
        FREE_9(bufs[k].vals);
    }
    FREE_9(bufs);
}



//============================= mne_events.c =============================


void mne_free_event_9(mneEvent e)
{
    if (!e)
        return;
    FREE_9(e->comment);
    FREE_9(e);
    return;
}


void mne_free_event_list_9(mneEventList list)

{
    int k;
    if (!list)
        return;
    for (k = 0; k < list->nevent; k++)
        mne_free_event_9(list->events[k]);
    FREE_9(list->events);
    FREE_9(list);
    return;
}






void mne_free_sparse_named_matrix_9(mneSparseNamedMatrix mat)
/*
      * Free the matrix and all the data from within
      */
{
    if (!mat)
        return;
    mne_free_name_list_9(mat->rowlist,mat->nrow);
    mne_free_name_list_9(mat->collist,mat->ncol);
    if(mat->data)
        delete mat->data;
    FREE_9(mat);
    return;
}


//============================= mne_raw_routines.c =============================

void mne_free_raw_info_9(mneRawInfo info)

{
    if (!info)
        return;
    FREE_9(info->filename);
    FREE_9(info->chInfo);
    FREE_9(info->trans);
//    FREE(info->rawDir);
    FREE_9(info->id);
    FREE_9(info);
    return;
}


void mne_raw_free_data_9(mneRawData d)

{
    if (!d)
        return;
//    fiff_close(d->file);
    d->stream->close();
    FREE_9(d->filename);
    mne_free_name_list_9(d->ch_names,d->info->nchan);

    free_bufs_9(d->bufs,d->nbuf);
    mne_free_ring_buffer_9(d->ring);

    free_bufs_9(d->filt_bufs,d->nfilt_buf);
    mne_free_ring_buffer_9(d->filt_ring);

    mne_free_proj_op_9(d->proj);
    mne_free_name_list_9(d->badlist,d->nbad);
    FREE_9(d->first_sample_val);
    FREE_9(d->bad);
    FREE_9(d->offsets);
    mne_free_ctf_comp_data_set_9(d->comp);
    if(d->sss)
        delete d->sss;

    if (d->filter_data_free)
        d->filter_data_free(d->filter_data);
    if (d->user_free)
        d->user_free(d->user);
    FREE_9(d->dig_trigger);
    mne_free_event_list_9(d->event_list);

    mne_free_raw_info_9(d->info);

    delete d->deriv;
    delete d->deriv_matched;
    FREE_9(d->deriv_offsets);

    FREE_9(d);
    return;
}





void mne_ch_selection_free_9(mneChSelection s)

{
    if (!s)
        return;
    FREE_9(s->name);
    FREE_9(s->pick);
    FREE_9(s->pick_deriv);
    FREE_9(s->ch_kind);
    mne_free_name_list_9(s->chspick,s->nchan);
    mne_free_name_list_9(s->chspick_nospace,s->nchan);
    mne_free_name_list_9(s->chdef,s->ndef);
    FREE_9(s);
    return;
}





//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMeasData::MneMeasData()
:filename  (NULL)
,meas_id   (NULL)
,current   (NULL)
,ch_major  (FALSE)
,nset      (0)
,nchan     (0)
,op        (NULL)
,fwd       (NULL)
,meg_head_t(NULL)
,mri_head_t(NULL)
,chs       (NULL)
,proj      (NULL)
,comp      (NULL)
,raw       (NULL)
,chsel     (NULL)
,bad       (NULL)
,nbad      (0)
,badlist   (NULL)
{
    meas_date.secs = 0;
    meas_date.usecs = 0;
}


//*************************************************************************************************************

MneMeasData::~MneMeasData()
{
    int k;

    FREE_9(filename);
    FREE_9(meas_id);
    FREE_9(chs);
    if(meg_head_t)
        delete meg_head_t;
    if(mri_head_t)
        delete mri_head_t;
    mne_free_proj_op_9(proj);
    mne_free_ctf_comp_data_set_9(comp);
    FREE_9(bad);
    mne_free_name_list_9(badlist,nbad);

    for (k = 0; k < nset; k++)
        delete sets[k];

    mne_raw_free_data_9(raw);
    mne_ch_selection_free_9(chsel);

    return;
}


//*************************************************************************************************************

void MneMeasData::adjust_baselines(float bmin, float bmax)
{
    int b1,b2;
    float sfreq,tmin,tmax;
    float **data;
    float ave;
    int s,c;

    if (!this->current)
        return;

    sfreq = 1.0/ this->current->tstep;
    tmin  =  this->current->tmin;
    tmax  =  this->current->tmin + ( this->current->np-1)/sfreq;

    if (bmin < tmin)
        b1 = 0;
    else if (bmin > tmax)
        b1 =  this->current->np;
    else {
        for (b1 = 0; b1/sfreq + tmin < bmin; b1++)
            ;
        if (b1 < 0)
            b1 = 0;
        else if (b1 >  this->current->np)
            b1 =  this->current->np;
    }
    if (bmax < tmin)
        b2 = 0;
    else if (bmax > tmax)
        b2 =  this->current->np;
    else {
        for (b2 =  this->current->np; b2/sfreq + tmin > bmax; b2--)
            ;
        if (b2 < 0)
            b2 = 0;
        else if (b2 >  this->current->np)
            b2 =  this->current->np;
    }
    data =  this->current->data;
    if (b2 > b1) {
        for (c = 0; c <  this->nchan; c++) {
            for (s = b1, ave = 0.0; s < b2; s++)
                ave += data[s][c];
            ave = ave/(b2-b1);
             this->current->baselines[c] += ave;
            for (s = 0; s <  this->current->np; s++)
                data[s][c] = data[s][c] - ave;
        }
        qDebug() << "TODO: Check comments content";
        fprintf(stderr,"\t%s : using baseline %7.1f ... %7.1f ms\n",
                 this->current->comment ?  this->current->comment : "unknown",
                1000*(tmin+b1/sfreq),
                1000*(tmin+b2/sfreq));
    }
    return;
}
