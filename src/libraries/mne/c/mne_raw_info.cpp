//=============================================================================================================
/**
 * @file     mne_raw_info.cpp
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
 * @brief    Definition of the MneRawInfo Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_info.h"

#include <fiff/fiff_tag.h>

#include <Eigen/Core>

#include <QFile>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define FREE_33(x) if ((char *)(x) != NULL) free((char *)(x))

#define MALLOC_33(x,t) (t *)malloc((x)*sizeof(t))

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneRawInfo::MneRawInfo()
{
}

//=============================================================================================================

MneRawInfo::~MneRawInfo()
{
    this->filename.clear();
    FREE_33(this->trans);
//    FREE_33(this->rawDir);
    FREE_33(this->id);
}

//=============================================================================================================

FiffDirNode::SPtr MneRawInfo::find_meas(const FiffDirNode::SPtr &node)
/*
          * Find corresponding meas node
          */
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;//(NULL);
        tmp_node = tmp_node->parent;
    }
    return (tmp_node);
}

//=============================================================================================================

FiffDirNode::SPtr MneRawInfo::find_meas_info(const FiffDirNode::SPtr &node)
/*
          * Find corresponding meas info node
          */
{
    int k;
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    for (k = 0; k < tmp_node->nchild(); k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}

//=============================================================================================================

FiffDirNode::SPtr MneRawInfo::find_raw(const FiffDirNode::SPtr &node)
/*
          * Find the raw data
          */
{
    FiffDirNode::SPtr raw;
    QList<FiffDirNode::SPtr> temp;
    temp = node->dir_tree_find(FIFFB_RAW_DATA);
    if (temp.size() == 0) {
        temp = node->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if (temp.size() > 0)
            raw = temp[0];
    }
    else
        raw = temp[0];
    return raw;
}

//=============================================================================================================

FiffDirNode::SPtr MneRawInfo::find_maxshield(const FiffDirNode::SPtr &node)

{
    FiffDirNode::SPtr raw;
    QList<FiffDirNode::SPtr> temp;
    temp = node->dir_tree_find(FIFFB_SMSH_RAW_DATA);
    if (temp.size() > 0)
        raw = temp[0];
    return (raw);
}

//=============================================================================================================

int MneRawInfo::get_meas_info(FiffStream::SPtr &stream,
                              FiffDirNode::SPtr &node,
                              fiffId *id,
                              int *nchan,
                              float *sfreq,
                              float *highpass,
                              float *lowpass,
                              QList<FiffChInfo>& chp,
                              FiffCoordTransOld **trans,
                              fiffTime *start_time)  /* Measurement date (starting time) */
/*
          * Find channel information from
          * nearest FIFFB_MEAS_INFO parent of
          * node.
          */
{
    FiffTag::SPtr t_pTag;
    //    fiffTagRec tag;
    //    fiffDirEntry this_ent;
    QList<FiffChInfo> ch;
    FiffChInfo this_ch;
    FiffCoordTransOld* t = nullptr;
    int j,k;
    int to_find = 4;
    QList<FiffDirNode::SPtr> hpi;
    FiffDirNode::SPtr meas;
    fiff_int_t kind, pos;

    //    tag.data    = NULL;
     *trans      = NULL;
     *id         = NULL;
     *start_time = NULL;
    /*
        * Find desired parents
        */
    //    meas = node->dir_tree_find(FIFFB_MEAS);
    if (!(meas = find_meas(node))) {
        //    if (meas.size() == 0) {
        printf ("Meas. block not found!");
        goto bad;
    }

    //    meas_info = node->dir_tree_find(FIFFB_MEAS_INFO);
    if (!(node = find_meas_info(node))) {
        //    if (meas_info.count() == 0) {
        printf ("Meas. info not found!");
        goto bad;
    }
    /*
       * Is there a block id is in the FIFFB_MEAS node?
       */
    //    if (meas->id != NULL) {
    if (!meas->id.isEmpty()) {
        *id = MALLOC_33(1,fiffIdRec);
        //        memcpy (*id,meas[0]->id,sizeof(fiffIdRec));
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
       * Others from FIFFB_MEAS_INFO
       */
     *lowpass  = -1;
     *highpass = -1;
    for (k = 0; k < node->nent(); k++) {
        kind = node->dir[k]->kind;
        pos  = node->dir[k]->pos;
        switch (kind) {

        case FIFF_NCHAN :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            *nchan = *(int *)(tag.data);
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();

            for (j = 0; j < *nchan; j++) {
                ch.append(FiffChInfo());
                ch[j].scanNo = -1;
            }
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_SFREQ :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            *sfreq = *(float *)(tag.data);
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *sfreq = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_LOWPASS :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            *lowpass = *(float *)(tag.data);
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *lowpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_HIGHPASS :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            *highpass = *(float *)(tag.data);
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *highpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_CH_INFO :		/* Information about one channel */
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            this_ch = (fiffChInfo)(tag.data);
            if (!stream->read_tag(t_pTag,pos))
                goto bad;

            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                ch[this_ch.scanNo-1] = this_ch;
            to_find--;
            break;

        case FIFF_MEAS_DATE :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            if (*start_time)
                FREE_33(*start_time);
            //            *start_time = (fiffTime)tag.data;
            *start_time = (fiffTime)t_pTag->data();
            //            tag.data = NULL;
            break;

        case FIFF_COORD_TRANS :
            //            if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
            //                goto bad;
            //            t = (fiffCoordTrans)tag.data;
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            if(t)
                delete t;
            t = FiffCoordTransOld::read_helper( t_pTag );
            /*
                * Require this particular transform!
                */
            if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                *trans = t;
                //                tag.data = NULL;
                break;
            }
        }
    }
    /*
        * Search for the coordinate transformation from
        * HPI_RESULT block if it was not previously found
        */
    //    hpi = fiff_dir_tree_find(node,FIFFB_HPI_RESULT);
    //    node = hpi[0];

    hpi = node->dir_tree_find(FIFFB_HPI_RESULT);
    node = hpi[0];

    //    FREE_33(hpi);
    if (hpi.size() > 0 && *trans == NULL)
        for (k = 0; k < hpi[0]->nent(); k++)
            if (hpi[0]->dir[k]->kind ==  FIFF_COORD_TRANS) {
                //                if (fiff_read_this_tag (file->fd,this_ent->pos,&tag) == -1)
                //                    goto bad;
                //                t = (fiffCoordTrans)tag.data;
                if (!stream->read_tag(t_pTag,hpi[0]->dir[k]->pos))
                    goto bad;
                t = FiffCoordTransOld::read_helper( t_pTag );
                /*
                    * Require this particular transform!
                    */
                if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                    *trans = t;
                    //                    tag.data = NULL;
                    break;
                }
            }
    if (to_find < 3) {
        if (*lowpass < 0) {
            *lowpass = *sfreq/2.0;
            to_find--;
        }
        if (*highpass < 0) {
            *highpass = 0.0;
            to_find--;
        }
    }
    if (to_find != 0) {
        printf ("Not all essential tags were found!");
        goto bad;
    }
    //    FREE_33(tag.data);
    chp = ch;
    return (0);

bad : {
        //        FREE_33(tag.data);
        return (-1);
    }
}

//=============================================================================================================

int MneRawInfo::mne_load_raw_info(const QString& name, int allow_maxshield, MneRawInfo **infop)
/*
          * Load raw data information from a fiff file
          */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    //    fiffFile       in       = NULL;

    int            res      = FIFF_FAIL;
    QList<FiffChInfo> chs;	/* Channel info */
    FiffCoordTransOld* trans    = NULL;	/* The coordinate transformation */
    fiffId         id       = NULL;	/* Measurement id */
    QList<FiffDirEntry::SPtr>   rawDir;	/* Directory of raw data tags */
    MneRawInfo*    info     = NULL;
    int            nchan    = 0;		/* Number of channels */
    float          sfreq    = 0.0;	/* Sampling frequency */
    float          highpass;		/* Highpass filter frequency */
    float          lowpass;		/* Lowpass filter frequency */
    FiffDirNode::SPtr    raw;
    //    FiffDirEntry   one;
    fiffTime       start_time = NULL;
    int            k;
    int            maxshield_data = FALSE;
    /*
       * Open file
       */
    //    if ((in = fiff_open(name)) == NULL)
    //        goto out;
    if(!stream->open())
        goto out;
    raw = find_raw(stream->dirtree());
    if (raw->isEmpty()) {
        if (allow_maxshield) {
            raw = find_maxshield(stream->dirtree());
            if (raw->isEmpty()) {
                printf("No raw data in this file.");
                goto out;
            }
            maxshield_data = TRUE;
        }
        else {
            printf("No raw data in this file.");
            goto out;
        }
    }
    /*
       * Get the essential measurement information
       */
    if (get_meas_info (stream,
                       raw,
                       &id,
                       &nchan,
                       &sfreq,
                       &highpass,
                       &lowpass,
                       chs,
                       &trans,
                       &start_time) < 0)
        goto out;
    /*
        * Get the raw directory
        */
    //    rawDir = MALLOC_33(raw->nent,fiffDirEntryRec);
    //    memcpy(rawDir,raw->dir,raw->nent*sizeof(fiffDirEntryRec));
    rawDir = raw->dir;
    /*
       * Ready to put everything together
       */
    info = new MneRawInfo();
    info->filename       = name;
    info->nchan          = nchan;
    info->chInfo         = chs;
    info->coord_frame    = FIFFV_COORD_DEVICE;
    info->trans          = trans;
    info->sfreq          = sfreq;
    info->lowpass        = lowpass;
    info->highpass       = highpass;
    //    info->rawDir         = NULL;
    info->maxshield_data = maxshield_data;
    if (id) {
        info->id           = MALLOC_33(1,fiffIdRec);
        *info->id          = *id;
    }
    else
        info->id           = NULL;
    /*
       * Getting starting time from measurement ID is not too accurate...
       */
    if (start_time)
        info->start_time = *start_time;
    else {
        if (id)
            info->start_time = id->time;
        else {
            info->start_time.secs = 0;
            info->start_time.usecs = 0;
        }
    }
    info->buf_size   = 0;
    //    for (k = 0, one = raw->dir; k < raw->nent; k++, one++) {
    for (k = 0; k < raw->nent(); k++) {
        //        raw->dir[k]->kind
        //                raw->dir[k]->type
        //                raw->dir[k].size
        if (raw->dir[k]->kind == FIFF_DATA_BUFFER) {
            if (raw->dir[k]->type == FIFFT_DAU_PACK16 || raw->dir[k]->type == FIFFT_SHORT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_short_t));
            else if (raw->dir[k]->type == FIFFT_FLOAT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_float_t));
            else if (raw->dir[k]->type == FIFFT_INT)
                info->buf_size = raw->dir[k]->size/(nchan*sizeof(fiff_int_t));
            else {
                printf("We are not prepared to handle raw data type: %d",raw->dir[k]->type);
                goto out;
            }
            break;
        }
    }
    if (info->buf_size <= 0) {
        printf("No raw data buffers available.");
        goto out;
    }
    info->rawDir     = rawDir;
    info->ndir       = raw->nent();
     *infop = info;
    res = FIFF_OK;

out : {
        if (res != FIFF_OK) {
            FREE_33(trans);
            //            FREE_33(rawDir);
            FREE_33(info);
        }
        FREE_33(id);
        //        fiff_close(in);
        stream->close();
        return (res);
    }
}
