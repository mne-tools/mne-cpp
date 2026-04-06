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
 * @brief    Definition of the MNERawInfo Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_info.h"

#include <fiff/fiff_tag.h>

#include <Eigen/Core>

#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERawInfo::MNERawInfo()
: nchan(0)
, coord_frame(0)
, sfreq(0.0f)
, lowpass(0.0f)
, highpass(0.0f)
, buf_size(0)
, maxshield_data(0)
, ndir(0)
{
}

//=============================================================================================================

MNERawInfo::~MNERawInfo()
{
}

//=============================================================================================================

FiffDirNode::SPtr MNERawInfo::find_meas(const FiffDirNode::SPtr &node)
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == nullptr)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    return tmp_node;
}

//=============================================================================================================

FiffDirNode::SPtr MNERawInfo::find_meas_info(const FiffDirNode::SPtr &node)
{
    int k;
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == nullptr)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    for (k = 0; k < tmp_node->nchild(); k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}

//=============================================================================================================

FiffDirNode::SPtr MNERawInfo::find_raw(const FiffDirNode::SPtr &node)
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

FiffDirNode::SPtr MNERawInfo::find_maxshield(const FiffDirNode::SPtr &node)

{
    FiffDirNode::SPtr raw;
    QList<FiffDirNode::SPtr> temp;
    temp = node->dir_tree_find(FIFFB_SMSH_RAW_DATA);
    if (temp.size() > 0)
        raw = temp[0];
    return (raw);
}

//=============================================================================================================

int MNERawInfo::get_meas_info(FiffStream::SPtr &stream,
                              FiffDirNode::SPtr &node,
                              std::unique_ptr<FiffId>& id,
                              int *nchan,
                              float *sfreq,
                              float *highpass,
                              float *lowpass,
                              QList<FiffChInfo>& chp,
                              FiffCoordTrans& trans,
                              FiffTime* *start_time)
{
    FiffTag::UPtr t_pTag;
    QList<FiffChInfo> ch;
    FiffChInfo this_ch;
    FiffCoordTrans t;
    int j,k;
    int to_find = 4;
    QList<FiffDirNode::SPtr> hpi;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;

     trans      = FiffCoordTrans();
     id.reset();
     *start_time = nullptr;
    /*
        * Find desired parents
        */
    if (!(meas = find_meas(node))) {
        printf ("Meas. block not found!");
        goto bad;
    }

    if (!(meas_info = find_meas_info(node))) {
        printf ("Meas. info not found!");
        goto bad;
    }
    /*
       * Is there a block id is in the FIFFB_MEAS node?
       */
    if (!meas->id.isEmpty()) {
        id = std::make_unique<FiffId>();
        id->version = meas->id.version;
        id->machid[0] = meas->id.machid[0];
        id->machid[1] = meas->id.machid[1];
        id->time = meas->id.time;
    }
    /*
       * Others from FIFFB_MEAS_INFO
       */
     *lowpass  = -1;
     *highpass = -1;
    for (k = 0; k < meas_info->nent(); k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {

        case FIFF_NCHAN :
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
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *sfreq = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_LOWPASS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *lowpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_HIGHPASS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *highpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_CH_INFO :
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
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            {
                FiffTime* pTime = (FiffTime*)t_pTag->data();
                *start_time = new FiffTime(pTime->secs, pTime->usecs);
            }
            break;

        case FIFF_COORD_TRANS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            t = FiffCoordTrans::readFromTag( t_pTag );
            /*
                * Require this particular transform!
                */
            if (t.from == FIFFV_COORD_DEVICE && t.to == FIFFV_COORD_HEAD) {
                trans = t;
                break;
            }
        }
    }
    /*
        * Search for the coordinate transformation from
        * HPI_RESULT block if it was not previously found
        */
    hpi = meas_info->dir_tree_find(FIFFB_HPI_RESULT);

    if (hpi.size() > 0 && trans.isEmpty())
        for (k = 0; k < hpi[0]->nent(); k++)
            if (hpi[0]->dir[k]->kind ==  FIFF_COORD_TRANS) {
                if (!stream->read_tag(t_pTag,hpi[0]->dir[k]->pos))
                    goto bad;
                t = FiffCoordTrans::readFromTag( t_pTag );
                if (t.from == FIFFV_COORD_DEVICE && t.to == FIFFV_COORD_HEAD) {
                    trans = t;
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
    chp = ch;
    return (0);

bad : {
        delete *start_time;
        *start_time = nullptr;
        return (-1);
    }
}

//=============================================================================================================

int MNERawInfo::load(const QString& name, int allow_maxshield, std::unique_ptr<MNERawInfo>& infop)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int            res      = FIFF_FAIL;
    QList<FiffChInfo> chs;	/* Channel info */
    FiffCoordTrans trans;       /* The coordinate transformation */
    std::unique_ptr<FiffId> id; /* Measurement id */
    QList<FiffDirEntry::SPtr>   rawDir;	/* Directory of raw data tags */
    std::unique_ptr<MNERawInfo> info;
    int            nchan    = 0;		/* Number of channels */
    float          sfreq    = 0.0;	/* Sampling frequency */
    float          highpass;		/* Highpass filter frequency */
    float          lowpass;		/* Lowpass filter frequency */
    FiffDirNode::SPtr    raw;
    FiffTime*      start_time = nullptr;
    int            k;
    int            maxshield_data = false;
    /*
       * Open file
       */
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
            maxshield_data = true;
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
                       id,
                       &nchan,
                       &sfreq,
                       &highpass,
                       &lowpass,
                       chs,
                       trans,
                       &start_time) < 0)
        goto out;
    /*
        * Get the raw directory
        */
    rawDir = raw->dir;
    /*
       * Ready to put everything together
       */
    info = std::make_unique<MNERawInfo>();
    info->filename       = name;
    info->nchan          = nchan;
    info->chInfo         = chs;
    info->coord_frame    = FIFFV_COORD_DEVICE;
    info->trans          = std::make_unique<FiffCoordTrans>(trans);
    info->sfreq          = sfreq;
    info->lowpass        = lowpass;
    info->highpass       = highpass;
    info->maxshield_data = maxshield_data;
    if (id) {
        info->id = std::make_unique<FiffId>(*id);
    }
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
    delete start_time;
    start_time = nullptr;
    info->buf_size   = 0;
    for (k = 0; k < raw->nent(); k++) {
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
    infop = std::move(info);
    res = FIFF_OK;

out : {
        stream->close();
        return (res);
    }
}
