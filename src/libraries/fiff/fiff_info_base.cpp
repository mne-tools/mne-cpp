//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_info_base.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Florian Schlembach <fschlembach@web.de>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     February 2013
 * @brief    Implementation of @ref FiffInfoBase: stripped measurement info (channels, sfreq, bads, dev_head_t) shared by every FIFF container.
 *
 * Used directly by realtime streams and by trimmed evoked / epoched
 * fragments that do not carry the full acquisition metadata.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info_base.h"

#include <iostream>

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <stdexcept>
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
    throw std::invalid_argument("Unknown channel type");
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
    if (sel == nullptr)
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

void FiffInfoBase::mne_read_meg_comp_eeg_ch_info(QList<FiffChInfo>& megp,
                                                  int& nmegp,
                                                  QList<FiffChInfo>& meg_compp,
                                                  int& nmeg_compp,
                                                  QList<FiffChInfo>& eegp,
                                                  int& neegp,
                                                  FiffCoordTrans& meg_head_t,
                                                  FiffId& idp) const
{
    for (int k = 0; k < nchan; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            megp.append(chs[k]);
            nmegp++;
        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
            meg_compp.append(chs[k]);
            nmeg_compp++;
        } else if (chs[k].kind == FIFFV_EEG_CH) {
            eegp.append(chs[k]);
            neegp++;
        }
    }
    meg_head_t = dev_head_t;
    idp = meas_id;
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

//=============================================================================================================

bool FiffInfoBase::readBadChannelsFromFile(const QString& name, QStringList& listOut)
{
    if (name.isEmpty()) {
        listOut.clear();
        return true;
    }

    QFile file(name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open bad channel file:" << name;
        return false;
    }

    QStringList list;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        list.append(line);
    }

    if (file.error() != QFileDevice::NoError) {
        qCritical() << "Error reading bad channel file:" << name;
        return false;
    }

    listOut = list;
    return true;
}
