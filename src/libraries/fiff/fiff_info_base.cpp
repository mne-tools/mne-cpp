//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_info_base.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
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
#include <QHash>
#include <QRegularExpression>
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
, all_coord_trans(p_FiffInfoBase.all_coord_trans)
{
}

//=============================================================================================================

FiffInfoBase::~FiffInfoBase()
{
}

//=============================================================================================================

namespace
{

/** mne-python's channel type name for @p ch, or an empty string for a kind outside the standard. */
QString channelTypeName(const FiffChInfo& ch)
{
    // mne-python's _first_rule (by kind) refined by _second_rules (MEG by unit, EEG/fNIRS/eye tracking by coil type)
    static const QHash<int, QString> byKind{
        {FIFFV_REF_MEG_CH, "ref_meg"}, {FIFFV_STIM_CH, "stim"}, {FIFFV_EOG_CH, "eog"}, {FIFFV_EMG_CH, "emg"}, {FIFFV_ECG_CH, "ecg"}, {FIFFV_RESP_CH, "resp"}, {FIFFV_MISC_CH, "misc"}, {FIFFV_EXCI_CH, "exci"}, {FIFFV_IAS_CH, "ias"}, {FIFFV_SYST_CH, "syst"}, {FIFFV_SEEG_CH, "seeg"}, {FIFFV_DBS_CH, "dbs"}, {FIFFV_BIO_CH, "bio"}, {FIFFV_DIPOLE_WAVE_CH, "dipole"}, {FIFFV_GOODNESS_FIT_CH, "gof"}, {FIFFV_ECOG_CH, "ecog"}, {FIFFV_TEMPERATURE_CH, "temperature"}, {FIFFV_GALVANIC_CH, "gsr"}};
    static const QHash<int, QString> eegByCoil{
        {FIFFV_COIL_EEG, "eeg"}, {FIFFV_COIL_EEG_BIPOLAR, "eeg"}, {FIFFV_COIL_NONE, "eeg"}, {FIFFV_COIL_EEG_CSD, "csd"}};
    static const QHash<int, QString> fnirsByCoil{
        {FIFFV_COIL_FNIRS_HBO, "hbo"}, {FIFFV_COIL_FNIRS_HBR, "hbr"}, {FIFFV_COIL_FNIRS_CW_AMPLITUDE, "fnirs_cw_amplitude"}, {FIFFV_COIL_FNIRS_FD_AC_AMPLITUDE, "fnirs_fd_ac_amplitude"}, {FIFFV_COIL_FNIRS_FD_PHASE, "fnirs_fd_phase"}, {FIFFV_COIL_FNIRS_OD, "fnirs_od"}, {FIFFV_COIL_FNIRS_TD_GATED_AMPLITUDE, "fnirs_td_gated_amplitude"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_INTENSITY, "fnirs_td_moments_intensity"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_MEAN, "fnirs_td_moments_mean"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_VARIANCE, "fnirs_td_moments_variance"}};
    static const QHash<int, QString> eyetrackByCoil{{FIFFV_COIL_EYETRACK_POS, "eyegaze"}, {FIFFV_COIL_EYETRACK_PUPIL, "pupil"}};

    const auto refine = [](const QHash<int, QString>& rule, int key) {
        return rule.value(key);
    };
    if (ch.kind == FIFFV_MEG_CH)
        return refine({{FIFF_UNIT_T_M, "grad"}, {FIFF_UNIT_T, "mag"}}, ch.unit);
    if (ch.kind == FIFFV_EEG_CH)
        return refine(eegByCoil, ch.chpos.coil_type);
    if (ch.kind == FIFFV_FNIRS_CH)
        return refine(fnirsByCoil, ch.chpos.coil_type);
    if (ch.kind == FIFFV_EYETRACK_CH)
        return refine(eyetrackByCoil, ch.chpos.coil_type);
    if (FIFFM_QUAT_CH(ch.kind))
        return "chpi"; // channels relative to head position monitoring
    return refine(byKind, ch.kind);
}

} // namespace

//=============================================================================================================

QString FiffInfoBase::channel_type(qint32 idx) const
{
    const QString type = channelTypeName(this->chs[idx]);
    if (type.isEmpty())
        throw std::invalid_argument("Unknown channel type for channel " + this->chs[idx].ch_name.toStdString());
    return type;
}

//=============================================================================================================

QMap<QString, QList<int>> FiffInfoBase::channel_indices_by_type(const QList<int>& picks) const
{
    QMap<QString, QList<int>> byType;
    for (const char* type : {"grad", "mag", "ref_meg", "eeg", "csd", "seeg", "dbs", "ecog", "eog", "emg", "ecg", "resp", "bio",
                             "misc", "stim", "exci", "syst", "ias", "gof", "dipole", "chpi", "temperature", "gsr", "hbo", "hbr",
                             "fnirs_cw_amplitude", "fnirs_fd_ac_amplitude", "fnirs_fd_phase", "fnirs_od",
                             "fnirs_td_gated_amplitude", "fnirs_td_moments_intensity", "fnirs_td_moments_mean",
                             "fnirs_td_moments_variance", "eyegaze", "pupil"}) {
        byType.insert(QString::fromLatin1(type), {});
    }
    const qsizetype count = picks.isEmpty() ? chs.size() : picks.size();
    for (qsizetype k = 0; k < count; ++k) {
        const int idx = picks.isEmpty() ? static_cast<int>(k) : picks[k];
        byType[channel_type(idx)].append(idx);
    }
    return byType;
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_data_channels(const QStringList& exclude) const
{
    static const QStringList dataTypes{"mag", "grad", "ref_meg", "eeg", "csd", "seeg", "ecog", "dbs", "hbo", "hbr"};
    QList<int> picks;
    for (int i = 0; i < chs.size(); ++i) {
        const QString type = channelTypeName(chs[i]);
        if ((dataTypes.contains(type) || type.startsWith(QLatin1String("fnirs_"))) && !exclude.contains(chs[i].ch_name))
            picks.append(i);
    }
    return Map<const RowVectorXi>(picks.data(), picks.size());
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_channels_regexp(const QStringList& ch_names, const QString& regexp)
{
    const QRegularExpression re(QRegularExpression::anchoredPattern(regexp + QStringLiteral(".*")),
                                QRegularExpression::DotMatchesEverythingOption);
    QList<int> sel;
    for (int k = 0; k < ch_names.size(); ++k) {
        if (re.match(ch_names[k]).hasMatch())
            sel.append(k);
    }
    return Eigen::Map<const RowVectorXi>(sel.constData(), sel.size());
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
    all_coord_trans.clear();
    bads.clear();
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_types(const QString meg, bool eeg, bool stim, const QStringList& include, const QStringList& exclude) const
{
    // A default-constructed info has nchan -1; count the channels actually present.
    const qint32 nchPresent = static_cast<qint32>(this->chs.size());
    RowVectorXi pick = RowVectorXi::Zero(nchPresent);

    fiff_int_t kind;
    qint32 k;
    for (k = 0; k < nchPresent; ++k) {
        kind = this->chs[k].kind;

        if ((kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH)) {
            if (meg.compare("all") == 0) {
                pick(k) = 1;
            } else if (meg.compare("grad") == 0 && this->chs[k].unit == FIFF_UNIT_T_M) {
                pick(k) = 1;
            } else if (meg.compare("mag") == 0 && this->chs[k].unit == FIFF_UNIT_T) {
                pick(k) = 1;
            }
        } else if (kind == FIFFV_EEG_CH && eeg)
            pick(k) = 1;
        else if (kind == FIFFV_STIM_CH && stim)
            pick(k) = 1;
    }

    // restrict channels to selection if provided
    qint32 p = 0;
    QStringList myinclude;
    for (k = 0; k < nchPresent; ++k) {
        if (pick(0, k)) {
            myinclude << this->ch_names[k];
            ++p;
        }
    }

    if (include.size() > 0) {
        for (k = 0; k < include.size(); ++k) {
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
    if (meg)
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
    for (qint32 k = 0; k < ch_names.size(); ++k) {
        if ((include.size() == 0 || include.contains(ch_names[k])) && !exclude.contains(ch_names[k])) {
            //make sure channel is unique
            if (!t_includedSelection.contains(ch_names[k])) {
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
    FiffInfoBase res = *this; //new FiffInfo(this);
    if (sel == nullptr)
        return res;

    //ToDo when pointer List do deletion
    res.chs.clear();
    res.ch_names.clear();

    qint32 idx;
    for (qint32 i = 0; i < sel->size(); ++i) {
        idx = (*sel)(0, i);
        res.chs.append(this->chs[idx]);
        res.ch_names.append(this->ch_names[idx]);
    }
    res.nchan = sel->size();

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
    for (const FiffChInfo& ch : chs) {
        const QString type = channelTypeName(ch);
        if (!type.isEmpty() && !lChannelTypes.contains(type))
            lChannelTypes << type;
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
