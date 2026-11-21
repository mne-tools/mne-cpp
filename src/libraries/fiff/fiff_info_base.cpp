//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_info_base.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Implementation of @ref FiffInfoBase: stripped measurement info (channels, sfreq, bads, dev_head_t) shared by every FIFF container.
 *
 * Used directly by realtime streams and by trimmed evoked / epoched
 * fragments that do not carry the full acquisition metadata.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info_base.h"

#include "fiff_ch_name.h"
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <iostream>
#include <regex>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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
std::string channelTypeName(const FiffChInfo& ch)
{
    // mne-python's _first_rule (by kind) refined by _second_rules (MEG by unit, EEG/fNIRS/eye tracking by coil type)
    static const std::unordered_map<int, std::string> byKind{
        {FIFFV_REF_MEG_CH, "ref_meg"}, {FIFFV_STIM_CH, "stim"}, {FIFFV_EOG_CH, "eog"}, {FIFFV_EMG_CH, "emg"}, {FIFFV_ECG_CH, "ecg"}, {FIFFV_RESP_CH, "resp"}, {FIFFV_MISC_CH, "misc"}, {FIFFV_EXCI_CH, "exci"}, {FIFFV_IAS_CH, "ias"}, {FIFFV_SYST_CH, "syst"}, {FIFFV_SEEG_CH, "seeg"}, {FIFFV_DBS_CH, "dbs"}, {FIFFV_BIO_CH, "bio"}, {FIFFV_DIPOLE_WAVE_CH, "dipole"}, {FIFFV_GOODNESS_FIT_CH, "gof"}, {FIFFV_ECOG_CH, "ecog"}, {FIFFV_TEMPERATURE_CH, "temperature"}, {FIFFV_GALVANIC_CH, "gsr"}};
    static const std::unordered_map<int, std::string> eegByCoil{
        {FIFFV_COIL_EEG, "eeg"}, {FIFFV_COIL_EEG_BIPOLAR, "eeg"}, {FIFFV_COIL_NONE, "eeg"}, {FIFFV_COIL_EEG_CSD, "csd"}};
    static const std::unordered_map<int, std::string> fnirsByCoil{
        {FIFFV_COIL_FNIRS_HBO, "hbo"}, {FIFFV_COIL_FNIRS_HBR, "hbr"}, {FIFFV_COIL_FNIRS_CW_AMPLITUDE, "fnirs_cw_amplitude"}, {FIFFV_COIL_FNIRS_FD_AC_AMPLITUDE, "fnirs_fd_ac_amplitude"}, {FIFFV_COIL_FNIRS_FD_PHASE, "fnirs_fd_phase"}, {FIFFV_COIL_FNIRS_OD, "fnirs_od"}, {FIFFV_COIL_FNIRS_TD_GATED_AMPLITUDE, "fnirs_td_gated_amplitude"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_INTENSITY, "fnirs_td_moments_intensity"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_MEAN, "fnirs_td_moments_mean"}, {FIFFV_COIL_FNIRS_TD_MOMENTS_VARIANCE, "fnirs_td_moments_variance"}};
    static const std::unordered_map<int, std::string> eyetrackByCoil{{FIFFV_COIL_EYETRACK_POS, "eyegaze"}, {FIFFV_COIL_EYETRACK_PUPIL, "pupil"}};

    const auto refine = [](const std::unordered_map<int, std::string>& rule, int key) {
        const auto it = rule.find(key);
        return it == rule.end() ? std::string() : it->second;
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

std::string FiffInfoBase::channel_type(std::int32_t idx) const
{
    std::string type = channelTypeName(this->chs[idx]);
    if (type.empty())
        throw std::invalid_argument("Unknown channel type for channel " + this->chs[idx].ch_name);
    return type;
}

//=============================================================================================================

int FiffInfoBase::channelIndex(std::string_view name) const
{
    return channel_index(ch_names, name);
}

//=============================================================================================================

bool FiffInfoBase::isBad(std::string_view name) const
{
    return has_channel(bads, name);
}

//=============================================================================================================

std::map<std::string, std::vector<int>> FiffInfoBase::channel_indices_by_type(const std::vector<int>& picks) const
{
    std::map<std::string, std::vector<int>> byType;
    for (const char* type : {"grad", "mag", "ref_meg", "eeg", "csd", "seeg", "dbs", "ecog", "eog", "emg", "ecg", "resp", "bio",
                             "misc", "stim", "exci", "syst", "ias", "gof", "dipole", "chpi", "temperature", "gsr", "hbo", "hbr",
                             "fnirs_cw_amplitude", "fnirs_fd_ac_amplitude", "fnirs_fd_phase", "fnirs_od",
                             "fnirs_td_gated_amplitude", "fnirs_td_moments_intensity", "fnirs_td_moments_mean",
                             "fnirs_td_moments_variance", "eyegaze", "pupil"}) {
        byType.try_emplace(type);
    }
    const std::size_t count = picks.empty() ? chs.size() : picks.size();
    for (std::size_t k = 0; k < count; ++k) {
        const int idx = picks.empty() ? static_cast<int>(k) : picks[k];
        byType[channel_type(idx)].push_back(idx);
    }
    return byType;
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_data_channels(const StringList& exclude) const
{
    static const std::vector<std::string_view> dataTypes{"mag", "grad", "ref_meg", "eeg", "csd", "seeg", "ecog", "dbs", "hbo", "hbr"};
    std::vector<int> picks;
    for (int i = 0; i < std::ssize(chs); ++i) {
        const std::string type = channelTypeName(chs[i]);
        const bool isData = std::find(dataTypes.begin(), dataTypes.end(), type) != dataTypes.end() || type.starts_with("fnirs_");
        if (isData && std::find(exclude.begin(), exclude.end(), chs[i].ch_name) == exclude.end())
            picks.push_back(i);
    }
    return Map<const RowVectorXi>(picks.data(), static_cast<Index>(picks.size()));
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_channels_regexp(const StringList& ch_names, const std::string& regexp)
{
    // Like re.match in Python: anchored at the start only
    const std::regex re(regexp);
    std::vector<int> sel;
    for (int k = 0; k < static_cast<int>(ch_names.size()); ++k) {
        if (std::regex_search(ch_names[static_cast<std::size_t>(k)], re, std::regex_constants::match_continuous))
            sel.push_back(k);
    }
    return Eigen::Map<const RowVectorXi>(sel.data(), static_cast<Eigen::Index>(sel.size()));
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

RowVectorXi FiffInfoBase::pick_types(const std::string& meg, bool eeg, bool stim, const StringList& include, const StringList& exclude) const
{
    // A default-constructed info has nchan -1; count the channels actually present.
    const std::int32_t nchPresent = static_cast<std::int32_t>(this->chs.size());
    RowVectorXi pick = RowVectorXi::Zero(nchPresent);

    fiff_int_t kind;
    std::int32_t k;
    for (k = 0; k < nchPresent; ++k) {
        kind = this->chs[k].kind;

        if ((kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH)) {
            if (meg == "all") {
                pick(k) = 1;
            } else if (meg == "grad" && this->chs[k].unit == FIFF_UNIT_T_M) {
                pick(k) = 1;
            } else if (meg == "mag" && this->chs[k].unit == FIFF_UNIT_T) {
                pick(k) = 1;
            }
        } else if (kind == FIFFV_EEG_CH && eeg)
            pick(k) = 1;
        else if (kind == FIFFV_STIM_CH && stim)
            pick(k) = 1;
    }

    // restrict channels to selection if provided
    std::int32_t p = 0;
    StringList myinclude;
    for (k = 0; k < nchPresent; ++k) {
        if (pick(0, k)) {
            myinclude.push_back(this->ch_names[static_cast<std::size_t>(k)]);
            ++p;
        }
    }
    myinclude.insert(myinclude.end(), include.begin(), include.end());
    p += static_cast<std::int32_t>(include.size());

    RowVectorXi sel;
    if (p != 0)
        sel = FiffInfoBase::pick_channels(this->ch_names, myinclude, exclude);

    return sel;
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_types(bool meg, bool eeg, bool stim, const StringList& include, const StringList& exclude) const
{
    return this->pick_types(std::string(meg ? "all" : ""), eeg, stim, include, exclude);
}

//=============================================================================================================

RowVectorXi FiffInfoBase::pick_channels(const StringList& ch_names, const StringList& include, const StringList& exclude)
{
    RowVectorXi sel = RowVectorXi::Zero(static_cast<Index>(ch_names.size()));
    std::unordered_set<std::string> selected;
    std::int32_t count = 0;
    for (std::size_t k = 0; k < ch_names.size(); ++k) {
        const std::string& name = ch_names[k];
        if ((include.empty() || has_channel(include, name)) && !has_channel(exclude, name) && selected.insert(name).second) {
            sel[count] = static_cast<int>(k);
            ++count;
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

    std::int32_t idx;
    for (std::int32_t i = 0; i < sel->size(); ++i) {
        idx = (*sel)(0, i);
        res.chs.push_back(this->chs[idx]);
        res.ch_names.push_back(this->ch_names[static_cast<std::size_t>(idx)]);
    }
    res.nchan = sel->size();
    std::erase_if(res.bads, [&res](const std::string& bad) { return res.channelIndex(bad) < 0; });

    return res;
}
//=============================================================================================================

void FiffInfoBase::mne_read_meg_comp_eeg_ch_info(std::vector<FiffChInfo>& megp,
                                                 int& nmegp,
                                                 std::vector<FiffChInfo>& meg_compp,
                                                 int& nmeg_compp,
                                                 std::vector<FiffChInfo>& eegp,
                                                 int& neegp,
                                                 FiffCoordTrans& meg_head_t,
                                                 FiffId& idp) const
{
    for (int k = 0; k < nchan; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            megp.push_back(chs[k]);
            nmegp++;
        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
            meg_compp.push_back(chs[k]);
            nmeg_compp++;
        } else if (chs[k].kind == FIFFV_EEG_CH) {
            eegp.push_back(chs[k]);
            neegp++;
        }
    }
    meg_head_t = dev_head_t;
    idp = meas_id;
}

//=============================================================================================================

StringList FiffInfoBase::get_channel_types()
{
    StringList types;
    for (const FiffChInfo& ch : chs) {
        std::string type = channelTypeName(ch);
        if (!type.empty() && std::find(types.begin(), types.end(), type) == types.end())
            types.push_back(std::move(type));
    }
    return types;
}

//=============================================================================================================

bool FiffInfoBase::readBadChannelsFromFile(const std::string& name, StringList& listOut)
{
    if (name.empty()) {
        listOut.clear();
        return true;
    }
    std::ifstream file(mne::path_from_utf8(name));
    if (!file) {
        mne::Log::critical("Cannot open bad channel file: {}", name);
        return false;
    }
    StringList list;
    std::string line;
    while (std::getline(file, line)) {
        const std::string_view trimmed = mne::trimmed(line);
        if (trimmed.empty() || trimmed.front() == '#')
            continue;
        list.emplace_back(trimmed);
    }
    if (file.bad()) {
        mne::Log::critical("Error reading bad channel file: {}", name);
        return false;
    }
    listOut = std::move(list);
    return true;
}
