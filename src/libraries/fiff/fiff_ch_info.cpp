//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_ch_info.cpp
 * @since 2022
 * @date  March 2026
 * @brief Implementation of @ref FiffChInfo: read / write of the FIFF_CH_INFO record and the conversion helpers used by the channel-picking path.
 *
 * Field layout matches the on-disk @c fiffChInfoRec; the helpers here
 * preserve parity with the @c ch_info dicts produced by
 * @c mne.io.meas_info in MNE-Python.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_ch_info.h"
#include "fiff_constants.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffChInfo::FiffChInfo()
: scanNo(0)
, logNo(0)
, kind(0)
, range(1.0f)
, cal(1.0f)
, unit(0)
, unit_mul(0)
, ch_name(QString(""))
, coil_trans()
, eeg_loc()
, coord_frame(FIFFV_COORD_UNKNOWN)
{
    coil_trans.setIdentity();
    eeg_loc.setZero();
}

//=============================================================================================================

FiffChInfo::FiffChInfo(const FiffChInfo &p_FiffChInfo)
: scanNo(p_FiffChInfo.scanNo)
, logNo(p_FiffChInfo.logNo)
, kind(p_FiffChInfo.kind)
, range(p_FiffChInfo.range)
, cal(p_FiffChInfo.cal)
, chpos(p_FiffChInfo.chpos)
, unit(p_FiffChInfo.unit)
, unit_mul(p_FiffChInfo.unit_mul)
, ch_name(p_FiffChInfo.ch_name)
, coil_trans(p_FiffChInfo.coil_trans)
, eeg_loc(p_FiffChInfo.eeg_loc)
, coord_frame(p_FiffChInfo.coord_frame)
{
}

//=============================================================================================================

FiffChInfo::~FiffChInfo()
{
}

//=============================================================================================================

bool FiffChInfo::checkEegLocations(const QList<FiffChInfo>& chs, int nch)
{
    const float close = 0.02f;
    for (int k = 0; k < nch; k++) {
        if (chs.at(k).kind == FIFFV_EEG_CH) {
            if (chs.at(k).chpos.r0.norm() < close) {
                qCritical("Some EEG channels do not have locations assigned.");
                return false;
            }
        }
    }
    return true;
}

//=============================================================================================================

bool FiffChInfo::isValidEeg() const
{
    constexpr float TOO_CLOSE = 1e-4f;
    return kind == FIFFV_EEG_CH
        && chpos.r0.norm() >= TOO_CLOSE
        && chpos.coil_type != FIFFV_COIL_NONE;
}
