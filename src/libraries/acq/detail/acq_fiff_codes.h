//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff_codes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    FIFF channel kinds and units as numbers, and their AcqChannelKind and AcqUnit, without the FIFF library.
 */

#ifndef ACQ_FIFF_CODES_H
#define ACQ_FIFF_CODES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_stream_info.h"

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** FIFF channel kinds (FIFFV_*_CH). */
namespace fiff_kind
{
inline constexpr int kMeg = 1;
inline constexpr int kEeg = 2;
inline constexpr int kStim = 3;
inline constexpr int kEog = 202;
inline constexpr int kRefMeg = 301;
inline constexpr int kEmg = 302;
inline constexpr int kEcg = 402;
inline constexpr int kMisc = 502;
inline constexpr int kResp = 602;
inline constexpr int kSeeg = 802;
inline constexpr int kDbs = 803;
inline constexpr int kEcog = 902;
inline constexpr int kFnirs = 1100;
inline constexpr int kTemperature = 1200;
inline constexpr int kGalvanic = 1300;
} // namespace fiff_kind

/** FIFF units (FIFF_UNIT_*). */
namespace fiff_unit
{
inline constexpr int kNone = -1;
inline constexpr int kMeter = 1;
inline constexpr int kSecond = 3;
inline constexpr int kAmpere = 4;
inline constexpr int kMol = 6;
inline constexpr int kPascal = 103;
inline constexpr int kWatt = 105;
inline constexpr int kVolt = 107;
inline constexpr int kOhm = 109;
inline constexpr int kTesla = 112;
inline constexpr int kCelsius = 114;
inline constexpr int kTeslaPerMeter = 201;
inline constexpr int kAmpereMeter = 202;
} // namespace fiff_unit

/** FIFF coordinate frames (FIFFV_COORD_*). */
namespace fiff_coord
{
inline constexpr int kUnknown = 0;
inline constexpr int kDevice = 1;
inline constexpr int kHead = 4;
} // namespace fiff_coord

struct AcqFiffKind
{
    AcqChannelKind kind;
    int fiff;
};

/** First match wins in both directions, so the canonical FIFF kind of an AcqChannelKind comes first. */
inline constexpr AcqFiffKind kAcqFiffKinds[] = {
    {AcqChannelKind::Eeg, fiff_kind::kEeg},
    {AcqChannelKind::Meg, fiff_kind::kMeg},
    {AcqChannelKind::Meg, fiff_kind::kRefMeg},
    {AcqChannelKind::Opm, fiff_kind::kMeg},
    {AcqChannelKind::Ecg, fiff_kind::kEcg},
    {AcqChannelKind::Eog, fiff_kind::kEog},
    {AcqChannelKind::Emg, fiff_kind::kEmg},
    {AcqChannelKind::Seeg, fiff_kind::kSeeg},
    {AcqChannelKind::Ecog, fiff_kind::kEcog},
    {AcqChannelKind::Dbs, fiff_kind::kDbs},
    {AcqChannelKind::Fnirs, fiff_kind::kFnirs},
    {AcqChannelKind::Respiration, fiff_kind::kResp},
    {AcqChannelKind::Gsr, fiff_kind::kGalvanic},
    {AcqChannelKind::Stim, fiff_kind::kStim},
    {AcqChannelKind::Marker, fiff_kind::kStim},
    {AcqChannelKind::Temperature, fiff_kind::kTemperature},
    {AcqChannelKind::Misc, fiff_kind::kMisc},
};

struct AcqFiffUnit
{
    AcqUnit unit;
    int fiff;
};

inline constexpr AcqFiffUnit kAcqFiffUnits[] = {
    {AcqUnit::None, fiff_unit::kNone},
    {AcqUnit::Volt, fiff_unit::kVolt},
    {AcqUnit::Ampere, fiff_unit::kAmpere},
    {AcqUnit::Tesla, fiff_unit::kTesla},
    {AcqUnit::TeslaPerMeter, fiff_unit::kTeslaPerMeter},
    {AcqUnit::AmpereMeter, fiff_unit::kAmpereMeter},
    {AcqUnit::DegreeCelsius, fiff_unit::kCelsius},
    {AcqUnit::Second, fiff_unit::kSecond},
    {AcqUnit::Ohm, fiff_unit::kOhm},
    {AcqUnit::Watt, fiff_unit::kWatt},
    {AcqUnit::Pascal, fiff_unit::kPascal},
    {AcqUnit::Molar, fiff_unit::kMol},
    {AcqUnit::Meter, fiff_unit::kMeter},
};

/** @return The FIFF kind of @p kind, FIFFV_MISC_CH if it has none. */
constexpr int fiff_kind_of(AcqChannelKind kind) noexcept
{
    for (const AcqFiffKind& m : kAcqFiffKinds) {
        if (m.kind == kind) {
            return m.fiff;
        }
    }
    return fiff_kind::kMisc;
}

/** @return The AcqChannelKind of FIFF kind @p fiff, Unknown if it has none. */
constexpr AcqChannelKind acq_kind_of(int fiff) noexcept
{
    for (const AcqFiffKind& m : kAcqFiffKinds) {
        if (m.fiff == fiff) {
            return m.kind;
        }
    }
    return AcqChannelKind::Unknown;
}

/** @return The FIFF unit of @p unit, FIFF_UNIT_NONE if it has none. */
constexpr int fiff_unit_of(AcqUnit unit) noexcept
{
    for (const AcqFiffUnit& m : kAcqFiffUnits) {
        if (m.unit == unit) {
            return m.fiff;
        }
    }
    return fiff_unit::kNone;
}

/** @return The AcqUnit of FIFF unit @p fiff, Unknown if it has none. */
constexpr AcqUnit acq_unit_of(int fiff) noexcept
{
    for (const AcqFiffUnit& m : kAcqFiffUnits) {
        if (m.fiff == fiff) {
            return m.unit;
        }
    }
    return AcqUnit::Unknown;
}

/** @return The coordinate frame FIFF readers give a channel of kind @p fiff, as MNE-Python does. */
constexpr int fiff_coord_frame_of(int fiff) noexcept
{
    switch (fiff) {
        case fiff_kind::kMeg:
        case fiff_kind::kRefMeg:
            return fiff_coord::kDevice;
        case fiff_kind::kEeg:
        case fiff_kind::kEcog:
        case fiff_kind::kSeeg:
        case fiff_kind::kDbs:
        case fiff_kind::kFnirs:
            return fiff_coord::kHead;
        default:
            return fiff_coord::kUnknown;
    }
}

} // namespace mne::detail

#endif // ACQ_FIFF_CODES_H
