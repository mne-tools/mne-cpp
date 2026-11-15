//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_raw_info.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNERawInfo.
 *
 * Reads the measurement header (@c FIFFB_MEAS_INFO) and the
 * @c FIFFB_RAW_DATA index from a FIFF file, populating the channel and
 * buffer-descriptor lists consumed by @ref MNERawData.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_info.h"
#include <memory>
#include <base/log.h>

#include <vector>

#include <fiff/fiff_tag.h>

#include <Eigen/Core>

#include <algorithm>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

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
{
}

//=============================================================================================================

MNERawInfo::~MNERawInfo()
{
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> MNERawInfo::find_meas(const std::shared_ptr<FiffDirNode>& node)
{
    std::shared_ptr<FiffDirNode> block = node;
    while (block && block->type != FIFFB_MEAS) {
        block = block->parent.lock();
    }
    return block;
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> MNERawInfo::find_meas_info(const std::shared_ptr<FiffDirNode>& node)
{
    const std::shared_ptr<FiffDirNode> meas = find_meas(node);
    if (!meas) {
        return {};
    }
    const auto info = std::ranges::find(meas->children, FIFFB_MEAS_INFO, &FiffDirNode::type);
    return info != meas->children.end() ? *info : std::shared_ptr<FiffDirNode>{};
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> MNERawInfo::find_raw(const std::shared_ptr<FiffDirNode>& node)
{
    std::shared_ptr<FiffDirNode> raw;
    std::vector<std::shared_ptr<FiffDirNode>> temp;
    temp = node->dir_tree_find(FIFFB_RAW_DATA);
    if (temp.size() == 0) {
        temp = node->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if (temp.size() > 0)
            raw = temp[0];
    } else
        raw = temp[0];
    return raw;
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> MNERawInfo::find_maxshield(const std::shared_ptr<FiffDirNode>& node)

{
    std::shared_ptr<FiffDirNode> raw;
    std::vector<std::shared_ptr<FiffDirNode>> temp;
    temp = node->dir_tree_find(FIFFB_SMSH_RAW_DATA);
    if (temp.size() > 0)
        raw = temp[0];
    return (raw);
}

//=============================================================================================================

int MNERawInfo::get_meas_info(FiffStream& stream,
                              std::shared_ptr<FiffDirNode>& node,
                              std::unique_ptr<FiffId>& id,
                              int* nchan,
                              float* sfreq,
                              float* highpass,
                              float* lowpass,
                              std::vector<FiffChInfo>& chp,
                              FiffCoordTrans& trans)
{
    FiffTime dummy;
    return get_meas_info(stream, node, id, nchan, sfreq, highpass, lowpass, chp, trans, dummy);
}

//=============================================================================================================

int MNERawInfo::get_meas_info(FiffStream& stream,
                              std::shared_ptr<FiffDirNode>& node,
                              std::unique_ptr<FiffId>& id,
                              int* nchan,
                              float* sfreq,
                              float* highpass,
                              float* lowpass,
                              std::vector<FiffChInfo>& chp,
                              FiffCoordTrans& trans,
                              FiffTime& start_time)
{
    std::optional<FiffTag> t_pTag;
    std::vector<FiffChInfo> ch;
    FiffChInfo this_ch;
    FiffCoordTrans t;
    int j, k;
    int to_find = 4;
    std::vector<std::shared_ptr<FiffDirNode>> hpi;
    std::shared_ptr<FiffDirNode> meas;
    std::shared_ptr<FiffDirNode> meas_info;
    fiff_int_t kind, pos;
    bool found_meas_date = false;

    trans = FiffCoordTrans();
    id.reset();
    start_time = FiffTime();
    /*
        * Find desired parents
        */
    if (meas = find_meas(node); !meas) {
        mne::Log::critical("Meas. block not found!");
        return (-1);
    }

    if (meas_info = find_meas_info(node); !meas_info) {
        mne::Log::critical("Meas. info not found!");
        return (-1);
    }
    /*
       * Is there a block id is in the FIFFB_MEAS node?
       */
    if (!meas->id.empty()) {
        id = std::make_unique<FiffId>();
        id->version = meas->id.version;
        id->machid[0] = meas->id.machid[0];
        id->machid[1] = meas->id.machid[1];
        id->time = meas->id.time;
    }
    /*
       * Others from FIFFB_MEAS_INFO
       */
    *lowpass = -1;
    *highpass = -1;
    for (k = 0; k < meas_info->nent(); k++) {
        kind = meas_info->dir[k].kind;
        pos = meas_info->dir[k].pos;
        switch (kind) {
            case FIFF_NCHAN:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                *nchan = *t_pTag->to_int();

                for (j = 0; j < *nchan; j++) {
                    ch.push_back(FiffChInfo());
                    ch[j].scanNo = -1;
                }
                to_find = to_find + *nchan - 1;
                break;

            case FIFF_SFREQ:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                *sfreq = *t_pTag->to_float();
                to_find--;
                break;

            case FIFF_LOWPASS:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                *lowpass = *t_pTag->to_float();
                to_find--;
                break;

            case FIFF_HIGHPASS:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                *highpass = *t_pTag->to_float();
                to_find--;
                break;

            case FIFF_CH_INFO:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);

                this_ch = t_pTag->to_ch_info();
                if (this_ch.scanNo <= 0 || this_ch.scanNo > *nchan) {
                    mne::Log::critical("FIFF_CH_INFO : scan # out of range!");
                    return (-1);
                } else
                    ch[this_ch.scanNo - 1] = this_ch;
                to_find--;
                break;

            case FIFF_MEAS_DATE:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                {
                    const FiffTime* pTime = reinterpret_cast<const FiffTime*>(t_pTag->data());
                    start_time = FiffTime(pTime->secs, pTime->usecs);
                    found_meas_date = true;
                }
                break;

            case FIFF_COORD_TRANS:
                if (t_pTag = stream.read_tag(pos); !t_pTag)
                    return (-1);
                t = FiffCoordTrans::from_tag(*t_pTag);
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

    if (hpi.size() > 0 && trans.empty())
        for (k = 0; k < hpi[0]->nent(); k++)
            if (hpi[0]->dir[k].kind == FIFF_COORD_TRANS) {
                if (t_pTag = stream.read_tag(hpi[0]->dir[k].pos); !t_pTag)
                    return (-1);
                t = FiffCoordTrans::from_tag(*t_pTag);
                if (t.from == FIFFV_COORD_DEVICE && t.to == FIFFV_COORD_HEAD) {
                    trans = t;
                    break;
                }
            }
    if (to_find < 3) {
        if (*lowpass < 0) {
            *lowpass = *sfreq / 2.0;
            to_find--;
        }
        if (*highpass < 0) {
            *highpass = 0.0;
            to_find--;
        }
    }
    if (to_find != 0) {
        mne::Log::critical("Not all essential tags were found!");
        return (-1);
    }
    chp = ch;
    /*
     * Resolve the measurement start time using the best available source
     */
    if (!found_meas_date) {
        if (id)
            start_time = id->time;
        else
            start_time = FiffTime();
    }
    return (0);
}

//=============================================================================================================

int MNERawInfo::load(const std::filesystem::path& name, int allow_maxshield, std::unique_ptr<MNERawInfo>& infop)
{
    FiffStream stream(name);

    int res = FIFF_FAIL;
    std::vector<FiffChInfo> chs; /* Channel info */
    FiffCoordTrans trans;        /* The coordinate transformation */
    std::unique_ptr<FiffId> id;  /* Measurement id */
    std::unique_ptr<MNERawInfo> info;
    int nchan = 0;     /* Number of channels */
    float sfreq = 0.0; /* Sampling frequency */
    float highpass;    /* Highpass filter frequency */
    float lowpass;     /* Lowpass filter frequency */
    std::shared_ptr<FiffDirNode> raw;
    FiffTime start_time;
    int k;
    int maxshield_data = false;
    /*
       * Open file
       */
    if (!stream.open()) {
        stream.close();
        return FIFF_FAIL;
    }
    raw = find_raw(stream.dirtree());
    if (raw->empty()) {
        if (allow_maxshield) {
            raw = find_maxshield(stream.dirtree());
            if (raw->empty()) {
                mne::Log::critical("No raw data in this file.");
                stream.close();
                return FIFF_FAIL;
            }
            maxshield_data = true;
        } else {
            mne::Log::critical("No raw data in this file.");
            stream.close();
            return FIFF_FAIL;
        }
    }
    /*
       * Get the essential measurement information
       */
    if (get_meas_info(stream,
                      raw,
                      id,
                      &nchan,
                      &sfreq,
                      &highpass,
                      &lowpass,
                      chs,
                      trans,
                      start_time) < 0) {
        stream.close();
        return FIFF_FAIL;
    }
    /*
       * Ready to put everything together
       */
    info = std::make_unique<MNERawInfo>();
    info->filename = name;
    info->nchan = nchan;
    info->chInfo = chs;
    info->coord_frame = FIFFV_COORD_DEVICE;
    info->trans = std::make_unique<FiffCoordTrans>(trans);
    info->sfreq = sfreq;
    info->lowpass = lowpass;
    info->highpass = highpass;
    info->maxshield_data = maxshield_data;
    if (id) {
        info->id = std::make_unique<FiffId>(*id);
    }
    /*
       * Getting starting time from measurement ID is not too accurate...
       */
    info->start_time = start_time;
    info->buf_size = 0;
    for (k = 0; k < raw->nent(); k++) {
        if (raw->dir[k].kind == FIFF_DATA_BUFFER) {
            if (raw->dir[k].type == FIFFT_DAU_PACK16 || raw->dir[k].type == FIFFT_SHORT)
                info->buf_size = raw->dir[k].size / (nchan * sizeof(fiff_short_t));
            else if (raw->dir[k].type == FIFFT_FLOAT)
                info->buf_size = raw->dir[k].size / (nchan * sizeof(fiff_float_t));
            else if (raw->dir[k].type == FIFFT_INT)
                info->buf_size = raw->dir[k].size / (nchan * sizeof(fiff_int_t));
            else if (raw->dir[k].type == FIFFT_DOUBLE)
                info->buf_size = raw->dir[k].size / (nchan * sizeof(double));
            else {
                mne::Log::critical("We are not prepared to handle raw data type: {}", raw->dir[k].type);
                stream.close();
                return FIFF_FAIL;
            }
            break;
        }
    }
    if (info->buf_size <= 0) {
        mne::Log::critical("No raw data buffers available.");
        stream.close();
        return FIFF_FAIL;
    }
    info->rawDir = raw->dir;
    infop = std::move(info);
    res = FIFF_OK;

    stream.close();
    return res;
}
