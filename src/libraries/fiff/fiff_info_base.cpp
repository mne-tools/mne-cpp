//=============================================================================================================
/**
 * @file     fiff_info_base.cpp
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
 * @brief    Definition of the FiffInfoBase Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info_base.h"

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfoBase::FiffInfoBase()
: filename("")
, nchan(-1)
{
}

//=============================================================================================================

FiffInfoBase::FiffInfoBase(const FiffInfoBase& p_FiffInfoBase)
: filename(p_FiffInfoBase.filename)
, bads(p_FiffInfoBase.bads)
, meas_id(FiffId(p_FiffInfoBase.meas_id))
, nchan(p_FiffInfoBase.nchan)
, chs(p_FiffInfoBase.chs)
, ch_names(p_FiffInfoBase.ch_names)
, dev_head_t(p_FiffInfoBase.dev_head_t)
, ctf_head_t(p_FiffInfoBase.ctf_head_t)
{
}

//=============================================================================================================

FiffInfoBase::~FiffInfoBase()
{
}

//=============================================================================================================

QString FiffInfoBase::channel_type(qint32 idx) const
{
    qint32 kind = this->chs[idx].kind;
    if(kind == FIFFV_MEG_CH)
    {
        if(this->chs[idx].unit == FIFF_UNIT_T_M)
            return "grad";
        else if(this->chs[idx].unit == FIFF_UNIT_T)
            return "mag";
    }
    else if(kind == FIFFV_REF_MEG_CH)
        return "ref_meg";
    else if(kind == FIFFV_EEG_CH)
        return "eeg";
    else if(kind == FIFFV_STIM_CH)
        return "stim";
    else if(kind == FIFFV_EOG_CH)
        return "eog";
    else if(kind == FIFFV_EMG_CH)
        return "emg";
    else if(kind == FIFFV_ECG_CH)
        return "ecg";
    else if(kind == FIFFV_MISC_CH)
        return "misc";
    else if (kind == FIFFV_QUAT_0 || kind == FIFFV_QUAT_1 || kind == FIFFV_QUAT_2
             || kind == FIFFV_QUAT_3 || kind == FIFFV_QUAT_4 || kind == FIFFV_QUAT_5
             || kind == FIFFV_QUAT_6 || kind == FIFFV_HPI_G || kind == FIFFV_HPI_ERR || kind == FIFFV_HPI_MOV)
        return "chpi";  // channels relative to head position monitoring
    printf("Unknown channel type\n"); //ToDo Throw
    return "";
}

//=============================================================================================================

void FiffInfoBase::clear()
{
    filename = "";
    meas_id.clear();
    nchan = -1;
    chs.clear();
    ch_names.clear();
    dev_head_t.clear();
    ctf_head_t.clear();
    bads.clear();
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_types(const QString meg, bool eeg, bool stim, const QStringList& include, const QStringList& exclude) const
{
    RowVectorXi pick = RowVectorXi::Zero(this->nchan);

    fiff_int_t kind;
    qint32 k;
    for(k = 0; k < this->nchan; ++k)
    {
        kind = this->chs[k].kind;

        if ((kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH))
        {
            if(meg.compare("all") == 0) {
                pick(k) = 1;
            } else if(meg.compare("grad") == 0 && this->chs[k].unit == FIFF_UNIT_T_M) {
                pick(k) = 1;
            } else if(meg.compare("mag") == 0 && this->chs[k].unit == FIFF_UNIT_T) {
                pick(k) = 1;
            }
        }
        else if (kind == FIFFV_EEG_CH && eeg)
            pick(k) = 1;
        else if (kind == FIFFV_STIM_CH && stim)
            pick(k) = 1;
    }

    // restrict channels to selection if provided
    qint32 p = 0;
    QStringList myinclude;
    for(k = 0; k < this->nchan; ++k)
    {
        if (pick(0, k))
        {
            myinclude << this->ch_names[k];
            ++p;
        }
    }

    if (include.size() > 0)
    {
        for (k = 0; k < include.size(); ++k)
        {
            myinclude << include[k];
            ++p;
        }
    }

    RowVectorXi sel;
    if (p != 0)
        sel = FiffInfoBase::pick_channels(this->ch_names, myinclude, exclude);

    return sel;
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_types(bool meg, bool eeg, bool stim, const QStringList& include, const QStringList& exclude) const
{
    if(meg)
        return this->pick_types(QString("all"), eeg, stim, include, exclude);
    else
        return this->pick_types(QString(""), eeg, stim, include, exclude);
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_channels(const QStringList& ch_names, const QStringList& include, const QStringList& exclude)
{
    RowVectorXi sel = RowVectorXi::Zero(ch_names.size());

    QStringList t_includedSelection;

    qint32 count = 0;
    for(qint32 k = 0; k < ch_names.size(); ++k)
    {
        if( (include.size() == 0 || include.contains(ch_names[k])) && !exclude.contains(ch_names[k]))
        {
            //make sure channel is unique
            if(!t_includedSelection.contains(ch_names[k]))
            {
                sel[count] = k;
                ++count;
                t_includedSelection << ch_names[k];
            }
        }
    }
    sel.conservativeResize(count);
    return sel;
}

//=============================================================================================================

FiffInfoBase FiffInfoBase::pick_info(const RowVectorXi* sel) const
{
    FiffInfoBase res = *this;//new FiffInfo(this);
    if (sel == NULL)
        return res;

    //ToDo when pointer List do deletion
    res.chs.clear();
    res.ch_names.clear();

    qint32 idx;
    for(qint32 i = 0; i < sel->size(); ++i)
    {
        idx = (*sel)(0,i);
        res.chs.append(this->chs[idx]);
        res.ch_names.append(this->ch_names[idx]);
    }
    res.nchan  = sel->size();

    return res;
}
//=============================================================================================================

QStringList FiffInfoBase::get_channel_types()
{
    QStringList lChannelTypes;

    for(int i = 0; i < chs.size(); ++i)
    {
        switch(chs.at(i).kind) {
            case FIFFV_MEG_CH: {
                if( chs.at(i).unit == FIFF_UNIT_T_M ) { //Gradiometers
                    if(!lChannelTypes.contains("grad")) {
                        lChannelTypes << "grad";
                    }
                } else if( chs.at(i).unit == FIFF_UNIT_T ) { //Magnetometers
                    if(!lChannelTypes.contains("mag")) {
                        lChannelTypes << "mag";
                    }
                }
                break;
            }

            case FIFFV_REF_MEG_CH: {
                if(!lChannelTypes.contains("ref_meg")) {
                    lChannelTypes << "ref_meg";
                }
                break;
            }

            case FIFFV_EEG_CH: { //EEG Channels
                if(!lChannelTypes.contains("eeg")) {
                    lChannelTypes << "eeg";
                }
                break;
            }

            case FIFFV_ECG_CH: { //ECG Channels
                if(!lChannelTypes.contains("ecg")) {
                    lChannelTypes << "ecg";
                }
                break;
            }
            case FIFFV_EMG_CH: { //EMG Channels
                if(!lChannelTypes.contains("emg")) {
                    lChannelTypes << "emg";
                }
                break;
            }
            case FIFFV_EOG_CH: { //EOG Channels
                if(!lChannelTypes.contains("eog")) {
                    lChannelTypes << "eog";
                }
                break;
            }

            case FIFFV_STIM_CH: { //STIM Channels
                if(!lChannelTypes.contains("stim")) {
                    lChannelTypes << "stim";
                }
                break;
            }

            case FIFFV_MISC_CH: { //MISC Channels
                if(!lChannelTypes.contains("misc")) {
                    lChannelTypes << "misc";
                }
                break;
            }
        }
    }

    return lChannelTypes;
}

