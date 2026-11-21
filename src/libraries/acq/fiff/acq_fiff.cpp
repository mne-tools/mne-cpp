//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Conversion between AcqStreamInfo and mne::FiffInfo.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_fiff.h"

#include "../detail/acq_fiff_codes.h"
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <cmath>
#include <cstring>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE LOCAL
//=============================================================================================================

namespace
{

constexpr const char* kMeasInfoSchema = "fiff.MeasInfo/1";

// The FIFF codes of the standard-library table are those of the FIFF library
static_assert(detail::fiff_kind::kMeg == FIFFV_MEG_CH && detail::fiff_kind::kEeg == FIFFV_EEG_CH && detail::fiff_kind::kStim == FIFFV_STIM_CH &&
              detail::fiff_kind::kEog == FIFFV_EOG_CH && detail::fiff_kind::kRefMeg == FIFFV_REF_MEG_CH && detail::fiff_kind::kEmg == FIFFV_EMG_CH &&
              detail::fiff_kind::kEcg == FIFFV_ECG_CH && detail::fiff_kind::kMisc == FIFFV_MISC_CH && detail::fiff_kind::kResp == FIFFV_RESP_CH &&
              detail::fiff_kind::kSeeg == FIFFV_SEEG_CH && detail::fiff_kind::kDbs == FIFFV_DBS_CH && detail::fiff_kind::kEcog == FIFFV_ECOG_CH &&
              detail::fiff_kind::kFnirs == FIFFV_FNIRS_CH && detail::fiff_kind::kTemperature == FIFFV_TEMPERATURE_CH &&
              detail::fiff_kind::kGalvanic == FIFFV_GALVANIC_CH);
static_assert(detail::fiff_unit::kNone == FIFF_UNIT_NONE && detail::fiff_unit::kMeter == FIFF_UNIT_M && detail::fiff_unit::kSecond == FIFF_UNIT_SEC && detail::fiff_unit::kAmpere == FIFF_UNIT_A &&
              detail::fiff_unit::kMol == FIFF_UNIT_MOL && detail::fiff_unit::kPascal == FIFF_UNIT_PA && detail::fiff_unit::kWatt == FIFF_UNIT_W &&
              detail::fiff_unit::kVolt == FIFF_UNIT_V && detail::fiff_unit::kOhm == FIFF_UNIT_OHM && detail::fiff_unit::kTesla == FIFF_UNIT_T &&
              detail::fiff_unit::kCelsius == FIFF_UNIT_CEL && detail::fiff_unit::kTeslaPerMeter == FIFF_UNIT_T_M &&
              detail::fiff_unit::kAmpereMeter == FIFF_UNIT_AM);
static_assert(detail::fiff_coord::kUnknown == FIFFV_COORD_UNKNOWN && detail::fiff_coord::kDevice == FIFFV_COORD_DEVICE &&
              detail::fiff_coord::kHead == FIFFV_COORD_HEAD);

// MNE-Python tells oxy- from deoxyhaemoglobin by the coil type and names the channels "S1_D1 hbo".
int coilType(const AcqChannelInfo& channel, int kind)
{
    if (kind == FIFFV_EEG_CH) {
        return FIFFV_COIL_EEG;
    }
    if (kind == FIFFV_FNIRS_CH) {
        const auto endsWith = [&](const char* suffix) {
            const std::size_t n = std::strlen(suffix);
            return channel.name.size() >= n && channel.name.compare(channel.name.size() - n, n, suffix) == 0;
        };
        return endsWith("hbo") ? FIFFV_COIL_FNIRS_HBO
            : endsWith("hbr")  ? FIFFV_COIL_FNIRS_HBR
                               : FIFFV_COIL_FNIRS_CW_AMPLITUDE;
    }
    return FIFFV_COIL_NONE;
}

std::vector<std::byte> serialize(const FiffInfo& info)
{
    // The blocks of the source file are not copied: the payload must not depend on files elsewhere.
    FiffInfo copy = info;
    copy.filename.clear();
    auto memory = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *memory;
    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(std::move(memory), copy, cals, defaultMatrixXi, false);
    stream->finish_writing_raw();
    return written.data();
}

bool deserialize(const std::vector<std::byte>& data, FiffInfo& info)
{
    FiffStream stream(std::make_unique<mne::MemoryByteStream>(data));
    if (!stream.open()) {
        return false;
    }
    std::shared_ptr<FiffDirNode> node;
    return stream.read_meas_info(stream.dirtree(), info, node);
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqStreamInfo mne::fromFiffInfo(const FiffInfo& info, AcqSampleFormat format)
{
    AcqStreamInfo stream;
    stream.name = "raw";
    stream.sourceId = info.filename;
    stream.format = format;
    stream.layout = AcqLayout::Interleaved;
    stream.sfreq = static_cast<double>(info.sfreq);
    stream.channels.reserve(static_cast<std::size_t>(info.chs.size()));
    for (const FiffChInfo& ch : info.chs) {
        AcqChannelInfo channel;
        channel.name = ch.ch_name;
        channel.kind = detail::acq_kind_of(ch.kind);
        channel.unit = detail::acq_unit_of(ch.unit);
        channel.unitExponent = ch.unit_mul;
        channel.scale = static_cast<double>(ch.cal) * static_cast<double>(ch.range);
        AcqFiffChannel fiff;
        fiff.kind = ch.kind;
        fiff.coilType = ch.chpos.coil_type;
        fiff.logicalNo = ch.logNo;
        fiff.scannerNo = ch.scanNo;
        fiff.unit = ch.unit;
        fiff.unitMul = ch.unit_mul;
        fiff.coordFrame = ch.coord_frame;
        fiff.cal = ch.cal;
        fiff.range = ch.range;
        for (std::size_t i = 0; i < 3; ++i) {
            const auto e = static_cast<Eigen::Index>(i);
            fiff.loc[i] = ch.chpos.r0[e];
            fiff.loc[3 + i] = ch.chpos.ex[e];
            fiff.loc[6 + i] = ch.chpos.ey[e];
            fiff.loc[9 + i] = ch.chpos.ez[e];
        }
        channel.fiff = fiff;
        stream.channels.push_back(std::move(channel));
    }
    AcqExtension extension;
    extension.schema = kMeasInfoSchema;
    extension.data = serialize(info);
    stream.extensions.push_back(std::move(extension));
    return stream;
}

//=============================================================================================================

AcqExpected<FiffInfo> mne::toFiffInfo(const AcqStreamInfo& info)
{
    if (isComplex(info.format) || info.segmentation.mode != AcqSegmentation::Mode::Continuous) {
        return AcqError(AcqErrorCode::UnsupportedFormat, "FIFF raw data holds continuous real samples only");
    }
    FiffInfo fiff;
    bool fromFiff = false;
    for (const AcqExtension& extension : info.extensions) {
        if (extension.schema == kMeasInfoSchema) {
            if (!deserialize(extension.data, fiff)) {
                return AcqError(AcqErrorCode::InvalidParameter, "corrupt fiff.MeasInfo/1 extension");
            }
            fromFiff = true;
        }
    }
    if (!fromFiff) {
        fiff.highpass = 0.0f;
        fiff.lowpass = static_cast<float>(info.sfreq / 2.0);
        fiff.proj_id = 0;
        fiff.meas_date[0] = -1;
        fiff.meas_date[1] = 0;
    }
    fiff.sfreq = static_cast<float>(info.sfreq);
    fiff.chs.clear();
    fiff.ch_names.clear();
    int index = 0;
    for (const AcqChannelInfo& channel : info.channels) {
        FiffChInfo ch;
        ch.ch_name = channel.name;
        ch.scanNo = ++index;
        if (channel.fiff) {
            const AcqFiffChannel& f = *channel.fiff;
            ch.kind = f.kind;
            ch.chpos.coil_type = f.coilType;
            ch.logNo = f.logicalNo;
            ch.unit = f.unit;
            ch.coord_frame = f.coordFrame;
            for (std::size_t i = 0; i < 3; ++i) {
                const auto e = static_cast<Eigen::Index>(i);
                ch.chpos.r0[e] = f.loc[i];
                ch.chpos.ex[e] = f.loc[3 + i];
                ch.chpos.ey[e] = f.loc[6 + i];
                ch.chpos.ez[e] = f.loc[9 + i];
            }
            const bool unchanged = channel.scale == static_cast<double>(f.cal) * static_cast<double>(f.range) &&
                channel.unitExponent == f.unitMul;
            ch.cal = unchanged ? f.cal : static_cast<float>(channel.scale * std::pow(10.0, channel.unitExponent));
            ch.range = unchanged ? f.range : 1.0f;
            ch.unit_mul = unchanged ? f.unitMul : FIFF_UNITM_NONE;
        } else {
            // MNE-Python applies cal * range but not unit_mul, so the power of ten goes into cal.
            ch.kind = channel.unit == AcqUnit::Unknown ? FIFFV_MISC_CH : detail::fiff_kind_of(channel.kind);
            ch.unit = detail::fiff_unit_of(channel.unit);
            ch.unit_mul = FIFF_UNITM_NONE;
            ch.logNo = index;
            ch.chpos.coil_type = coilType(channel, ch.kind);
            ch.coord_frame = ch.kind == FIFFV_EEG_CH ? FIFFV_COORD_HEAD : FIFFV_COORD_UNKNOWN;
            ch.cal = static_cast<float>(channel.scale * std::pow(10.0, channel.unitExponent));
            ch.range = 1.0f;
        }
        fiff.chs.push_back(ch);
        fiff.ch_names.push_back(ch.ch_name);
    }
    fiff.nchan = static_cast<fiff_int_t>(fiff.chs.size());
    return fiff;
}
