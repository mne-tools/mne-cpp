//=============================================================================================================
/**
* @file     mne_raw_data.cpp
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
* @brief    Implementation of the MneRawData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_data.h"


#include <Eigen/Core>


#define FREE_36(x) if ((char *)(x) != NULL) free((char *)(x))



typedef struct {
    int   size;		        /* Size of this buffer in floats */
    float *data;			/* The allocated buffer */
    float ***datap;		/* The matrix which uses this */
} *ringBufBuf_36,ringBufBufRec_36;

typedef struct {
    ringBufBuf_36 *bufs;
    int        nbuf;
    int        next;
} *ringBuf_36,ringBufRec_36;



void mne_free_ring_buffer_36(void *thisp)

{
    int k;
    ringBuf_36 this_buf = (ringBuf_36)thisp;

    if (!this_buf)
        return;

    for (k = 0; k < this_buf->nbuf; k++)
        FREE_36(this_buf->bufs[k]->data);
    FREE_36(this_buf->bufs);
    FREE_36(this_buf);
    return;
}


void mne_free_event_36(mneEvent e)
{
    if (!e)
        return;
    FREE_36(e->comment);
    FREE_36(e);
    return;
}

void mne_free_event_list_36(mneEventList list)

{
    int k;
    if (!list)
        return;
    for (k = 0; k < list->nevent; k++)
        mne_free_event_36(list->events[k]);
    FREE_36(list->events);
    FREE_36(list);
    return;
}



void mne_free_name_list_36(char **list, int nlist)
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
        FREE_36(list[k]);
    }
    FREE_36(list);
    return;
}




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

MneRawData::MneRawData()
:filename(NULL)
,info(NULL)
,bufs(NULL)
,nbuf(0)
,proj(NULL)
,ch_names(NULL)
,bad(NULL)
,badlist(NULL)
,nbad(0)
,first_samp(0)
,omit_samp(0)
,omit_samp_old(0)
,event_list(NULL)
,max_event(0)
,dig_trigger(NULL)
,dig_trigger_mask(0)
,ring(NULL)
,filt_ring(NULL)
,filt_bufs(NULL)
,nfilt_buf(0)
,first_sample_val(NULL)
,filter(NULL)
,filter_data(NULL)
,filter_data_free(NULL)
,offsets(NULL)
,deriv(NULL)
,deriv_matched(NULL)
,deriv_offsets(NULL)
,user(NULL)
,user_free(NULL)
,comp(NULL)
,comp_file(MNE_CTFV_NOGRAD)
,comp_now(MNE_CTFV_NOGRAD)
,sss(NULL)
{

}


//*************************************************************************************************************

MneRawData::~MneRawData()
{
//    fiff_close(this->file);
    this->stream->close();
    FREE_36(this->filename);
    mne_free_name_list_36(this->ch_names,this->info->nchan);

    MneRawBufDef::free_bufs(this->bufs,this->nbuf);
    mne_free_ring_buffer_36(this->ring);

    MneRawBufDef::free_bufs(this->filt_bufs,this->nfilt_buf);
    mne_free_ring_buffer_36(this->filt_ring);

    if(this->proj)
        delete this->proj;
    mne_free_name_list_36(this->badlist,this->nbad);
    FREE_36(this->first_sample_val);
    FREE_36(this->bad);
    FREE_36(this->offsets);
    if(this->comp)
        delete this->comp;
    if(this->sss)
        delete this->sss;

    if (this->filter_data_free)
        this->filter_data_free(this->filter_data);
    if (this->user_free)
        this->user_free(this->user);
    FREE_36(this->dig_trigger);
    mne_free_event_list_36(this->event_list);

    if(this->info)
        delete this->info;

    delete this->deriv;
    delete this->deriv_matched;
    FREE_36(this->deriv_offsets);
}
