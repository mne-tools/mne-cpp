//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_info.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffInfo: read / write of FIFFB_MEAS / FIFFB_MEAS_INFO and the channel-picking, projector and compensator surgery.
 *
 * Drop-in counterpart of @c mne.Info; carries everything needed to
 * interpret a continuous, epoched or evoked data matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"
#include "fiff_ch_name.h"
#include "fiff_file.h"
#include "fiff_stream.h"

#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfo::FiffInfo()
: FiffInfoBase() //nchan(-1)
, sfreq(-1.0)
, linefreq(-1.0)
, highpass(-1.0)
, lowpass(-1.0)
, gantry_angle(-1)
, acq_pars("")
, acq_stim("")
{
    meas_date[0] = -1;
}

//=============================================================================================================

FiffInfo::FiffInfo(const FiffInfo& p_FiffInfo)
: FiffInfoBase(p_FiffInfo)
, file_id(p_FiffInfo.file_id)
, sfreq(p_FiffInfo.sfreq)
, linefreq(p_FiffInfo.linefreq)
, highpass(p_FiffInfo.highpass)
, lowpass(p_FiffInfo.lowpass)
, proj_id(p_FiffInfo.proj_id)
, proj_name(p_FiffInfo.proj_name)
, xplotter_layout(p_FiffInfo.xplotter_layout)
, experimenter(p_FiffInfo.experimenter)
, description(p_FiffInfo.description)
, utc_offset(p_FiffInfo.utc_offset)
, gantry_angle(p_FiffInfo.gantry_angle)
, dev_ctf_t(p_FiffInfo.dev_ctf_t)
, dig(p_FiffInfo.dig)
, dig_trans(p_FiffInfo.dig_trans)
, projs(p_FiffInfo.projs)
, comps(p_FiffInfo.comps)
, acq_pars(p_FiffInfo.acq_pars)
, acq_stim(p_FiffInfo.acq_stim)
, hpi_coil_freqs(p_FiffInfo.hpi_coil_freqs)
, device_info(p_FiffInfo.device_info)
{
    meas_date[0] = p_FiffInfo.meas_date[0];
    meas_date[1] = p_FiffInfo.meas_date[1];
}

//=============================================================================================================

FiffInfo::~FiffInfo()
{
}

//=============================================================================================================

void FiffInfo::clear()
{
    FiffInfoBase::clear();
    file_id = FiffId();
    meas_date[0] = -1;
    sfreq = -1.0;
    linefreq = -1.0;
    highpass = -1.0;
    lowpass = -1.0;
    proj_id = -1;
    proj_name = "";
    xplotter_layout = "";
    experimenter = "";
    description = "";
    utc_offset = ""; /**< UTC offset of related meas_date (sHH:MM).*/
    gantry_angle = -1;
    dev_ctf_t.clear();
    dig.clear();
    dig_trans.clear();
    projs.clear();
    comps.clear();
    acq_pars = "";
    acq_stim = "";
    hpi_coil_freqs.clear();
    device_info = FiffDeviceInfo();
}

//=============================================================================================================

std::int32_t FiffInfo::get_current_comp()
{
    std::int32_t comp = 0;
    std::int32_t first_comp = -1;

    std::int32_t k = 0;
    for (k = 0; k < this->nchan; ++k) {
        if (this->chs[k].kind == FIFFV_MEG_CH) {
            comp = this->chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (comp != first_comp)
                Log::warning("Compensation is not set equally on all MEG channels");
        }
    }
    return comp;
}

//=============================================================================================================

bool FiffInfo::make_compensator(fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs) const
{
    MatrixXd C1, C2, comp_tmp;

    //    if(ctf_comp.data)
    //        delete ctf_comp.data;
    ctf_comp.data.clear();

    if (from == to) {
        ctf_comp.data.data = MatrixXd::Identity(this->nchan, this->nchan);
        return false;
    }

    if (from == 0)
        C1 = MatrixXd::Zero(this->nchan, this->nchan);
    else {
        if (!this->make_compensator(from, C1)) {
            Log::warning("Cannot create compensator C1\n");
            Log::warning("Desired compensation matrix (kind = {}) not found\n", from);
            return false;
        }
    }

    if (to == 0)
        C2 = MatrixXd::Zero(this->nchan, this->nchan);
    else {
        if (!this->make_compensator(to, C2)) {
            Log::warning("Cannot create compensator C2\n");
            Log::warning("Desired compensation matrix (kind = {}) not found\n", to);
            return false;
        }
    }
    //
    //   s_from = (I - C1)*s_orig  =>  s_orig = (I - C1)^-1 * s_from
    //   s_to   = (I - C2)*s_orig
    //   (I + C1) is only the inverse when C1*C1 = 0, which fails as soon as
    //   reference channels are themselves compensated (e.g. CTF grade 1).
    //
    const MatrixXd eye = MatrixXd::Identity(this->nchan, this->nchan);
    comp_tmp = (eye - C2) * (eye - C1).partialPivLu().inverse();

    std::int32_t k;
    if (exclude_comp_chs) {
        VectorXi pick = VectorXi::Zero(this->nchan);
        std::int32_t npick = 0;
        for (k = 0; k < this->nchan; ++k) {
            if (this->chs[k].kind != FIFFV_REF_MEG_CH) {
                pick(npick) = k;
                ++npick;
            }
        }
        if (npick == 0) {
            Log::warning("Nothing remains after excluding the compensation channels\n");
            return false;
        }

        ctf_comp.data.data.resize(npick, this->nchan);
        for (k = 0; k < npick; ++k)
            ctf_comp.data.data.row(k) = comp_tmp.block(pick(k), 0, 1, this->nchan);
    } else {
        ctf_comp.data.data = comp_tmp;
    }
    // FiffRawData applies the compensator only when kind is set.
    ctf_comp.kind = to;

    return true;
}

//=============================================================================================================

bool FiffInfo::make_compensator(fiff_int_t kind, MatrixXd& this_comp) const //private method
{
    const FiffNamedMatrix* this_data = nullptr;
    MatrixXd presel, postsel;
    std::int32_t k, col, c, ch = 0, row, row_ch = 0, channelAvailable;

    for (k = 0; k < std::ssize(this->comps); ++k) {
        if (this->comps[k].kind == kind) {
            this_data = &this->comps[k].data;

            //
            //   Create the preselector
            //
            presel = MatrixXd::Zero(this_data->ncol, this->nchan);

            for (col = 0; col < this_data->ncol; ++col) {
                channelAvailable = 0;
                for (c = 0; c < static_cast<int>(this->ch_names.size()); ++c) {
                    if ((this_data->col_names.at(col) == this->ch_names.at(c))) {
                        ++channelAvailable;
                        ch = c;
                    }
                }
                if (channelAvailable == 0) {
                    Log::warning("Channel {} is not available in data\n", this_data->col_names.at(col));
                    return false;
                } else if (channelAvailable > 1) {
                    Log::warning("Ambiguous channel {}", this_data->col_names.at(col));
                    return false;
                }
                presel(col, ch) = 1.0;
            }
            //
            //   Create the postselector
            //
            postsel = MatrixXd::Zero(this->nchan, this_data->nrow);

            for (c = 0; c < this->nchan; ++c) {
                channelAvailable = 0;
                for (row = 0; row < static_cast<int>(this_data->row_names.size()); ++row) {
                    if ((this->ch_names.at(c) == this_data->row_names.at(row))) {
                        ++channelAvailable;
                        row_ch = row;
                    }
                }
                if (channelAvailable > 1) {
                    Log::warning("Ambiguous channel {}", this->ch_names.at(c));
                    return false;
                } else if (channelAvailable == 1) {
                    postsel(c, row_ch) = 1.0;
                }
            }
            this_comp = postsel * this_data->data * presel;
            return true;
        }
    }
    this_comp = defaultMatrixXd;
    return false;
}

//=============================================================================================================

FiffInfo FiffInfo::pick_info(const RowVectorXi& sel) const
{
    FiffInfo res = *this; //new FiffInfo(this);
    if (sel.size() == 0)
        return res;

    //ToDo when pointer List do delation
    res.chs.clear();
    res.ch_names.clear();

    std::int32_t idx;
    for (std::int32_t i = 0; i < sel.size(); ++i) {
        idx = sel[i];
        res.chs.push_back(this->chs[idx]);
        res.ch_names.push_back(this->ch_names[idx]);
    }
    res.nchan = static_cast<int>(sel.size());
    std::erase_if(res.bads, [&res](const std::string& bad) { return !mne::has_channel(res.ch_names, bad); });

    return res;
}

//=============================================================================================================

std::vector<FiffChInfo> FiffInfo::set_current_comp(std::vector<FiffChInfo>& listFiffChInfo, fiff_int_t value)
{
    std::vector<FiffChInfo> newList;
    std::int32_t k;
    fiff_int_t coil_type;

    for (k = 0; k < std::ssize(listFiffChInfo); ++k)
        newList.push_back(listFiffChInfo[k]);

    std::int32_t lower_half = 65535; // hex2dec('FFFF');
    for (k = 0; k < std::ssize(listFiffChInfo); ++k) {
        if (listFiffChInfo[k].kind == FIFFV_MEG_CH) {
            coil_type = listFiffChInfo[k].chpos.coil_type & lower_half;
            newList[k].chpos.coil_type = (coil_type | (value << 16));
        }
    }
    return newList;
}

//=============================================================================================================

bool FiffInfo::readMegEegChannels(const std::string& name,
                                  bool do_meg,
                                  bool do_eeg,
                                  const StringList& bads,
                                  std::vector<FiffChInfo>& chsp,
                                  int& nmegp,
                                  int& neegp)
{
    FiffStream stream(name);

    if (!stream.open())
        return false;

    FiffInfo info;
    std::shared_ptr<FiffDirNode> infoNode;
    if (!stream.read_meas_info(stream.dirtree(), info, infoNode)) {
        Log::critical("{} : could not read measurement info", name);
        stream.close();
        return false;
    }
    stream.close();

    std::vector<FiffChInfo> meg;
    std::vector<FiffChInfo> eeg;

    for (int k = 0; k < std::ssize(info.chs); k++) {
        if (mne::has_channel(bads, info.chs[k].ch_name))
            continue;
        if (do_meg && info.chs[k].kind == FIFFV_MEG_CH)
            meg.push_back(info.chs[k]);
        else if (do_eeg && info.chs[k].isValidEeg())
            eeg.push_back(info.chs[k]);
    }

    chsp.clear();
    chsp.reserve(meg.size() + eeg.size());
    chsp.insert(chsp.end(), meg.begin(), meg.end());
    chsp.insert(chsp.end(), eeg.begin(), eeg.end());

    nmegp = static_cast<int>(meg.size());
    neegp = static_cast<int>(eeg.size());
    return true;
}

//=============================================================================================================

void FiffInfo::writeToStream(FiffStream& stream) const
{
    //
    //   We will always write floats
    //
    fiff_int_t data_type = 4;
    std::vector<FiffChInfo> chsToWrite(this->chs.begin(), this->chs.begin() + this->nchan);

    fiff_int_t nchanToWrite = static_cast<int>(chsToWrite.size());

    //
    // write the essentials
    //
    stream.start_block(FIFFB_MEAS); //4
    stream.write_id(FIFF_BLOCK_ID); //5
    if (this->meas_id.version != -1) {
        stream.write_id(FIFF_PARENT_BLOCK_ID, this->meas_id); //6
    }
    //
    //    Measurement info
    //
    stream.start_block(FIFFB_MEAS_INFO); //7

    //
    //    Blocks from the original -> skip this
    //
    //        std::vector<fiff_int_t> blocks;
    //        blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY;
    bool have_hpi_result = false;
    bool have_isotrak = false;
    //
    //    megacq parameters
    //
    if (!this->acq_pars.empty() || !this->acq_stim.empty()) {
        stream.start_block(FIFFB_DACQ_PARS);
        if (!this->acq_pars.empty())
            stream.write_string(FIFF_DACQ_PARS, this->acq_pars);

        if (!this->acq_stim.empty())
            stream.write_string(FIFF_DACQ_STIM, this->acq_stim);

        stream.end_block(FIFFB_DACQ_PARS);
    }
    stream.write_device_info(this->device_info);
    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result) {
        if (!this->dev_head_t.empty())
            stream.write_coord_trans(this->dev_head_t);

        if (!this->ctf_head_t.empty())
            stream.write_coord_trans(this->ctf_head_t);
    }
    //
    //    Polhemus data
    //
    if (this->dig.size() > 0 && !have_isotrak) {
        stream.start_block(FIFFB_ISOTRAK);
        for (std::int32_t k = 0; k < std::ssize(this->dig); ++k)
            stream.write_dig_point(this->dig[k]);

        stream.end_block(FIFFB_ISOTRAK);
    }
    //
    //    Projectors
    //
    stream.write_proj(this->projs);
    //
    //    CTF compensation info
    //
    stream.write_ctf_comp(this->comps);
    //
    //    Bad channels
    //
    if (this->bads.size() > 0) {
        stream.start_block(FIFFB_MNE_BAD_CHANNELS);
        stream.write_name_list(FIFF_MNE_CH_NAME_LIST, this->bads);
        stream.end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //    General
    //
    stream.write_float(FIFF_SFREQ, &this->sfreq);
    if (this->linefreq >= 0.0f)
        stream.write_float(FIFF_LINE_FREQ, &this->linefreq);
    stream.write_float(FIFF_HIGHPASS, &this->highpass);
    stream.write_float(FIFF_LOWPASS, &this->lowpass);
    stream.write_string(FIFF_EXPERIMENTER, this->experimenter);
    stream.write_string(FIFF_DESCRIPTION, this->description);
    stream.write_string(FIFF_UTC_OFFSET, this->utc_offset);
    stream.write_string(FIFF_PROJ_NAME, this->proj_name);
    stream.write_int(FIFF_PROJ_ID, &this->proj_id);
    stream.write_int(FIFF_GANTRY_ANGLE, &this->gantry_angle);
    stream.write_int(FIFF_NCHAN, &nchanToWrite);
    stream.write_int(FIFF_DATA_PACK, &data_type);
    if (this->meas_date[0] != -1)
        stream.write_int(FIFF_MEAS_DATE, this->meas_date, 2);
    //
    //    Channel info
    //
    MatrixXd cals(1, nchanToWrite);

    for (std::int32_t k = 0; k < nchanToWrite; ++k) {
        //
        //    Scan numbers may have been messed up
        //
        chsToWrite[k].scanNo = k + 1;                        //+1 because
                                                             //        chs[k].range  = 1.0f;//Why? -> cause its already calibrated through reading
        cals(0, k) = static_cast<double>(chsToWrite[k].cal); //ToDo whats going on with cals?
        stream.write_ch_info(chsToWrite[k]);
    }
    //
    //
    stream.end_block(FIFFB_MEAS_INFO);
}

//=============================================================================================================

void FiffInfo::print() const
{
    Log::info("Sample frequency: {}", sfreq);
    Log::info("LineFreq: {} | Highpass: {} | Lowpass: {}", linefreq, highpass, lowpass);
    Log::info("Number of digitizer points: {}", dig.size());
    for (auto& point : dig) {
        if (point.kind == FIFFV_POINT_HPI) {
            Log::info("HPI Point {} - {}, {}, {}", point.ident, point.r[0], point.r[1], point.r[2]);
        }
    }
}
