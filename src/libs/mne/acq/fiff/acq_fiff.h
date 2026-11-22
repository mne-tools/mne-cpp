//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Lossless conversion between AcqStreamInfo and mne::FiffInfo.
 *
 * Part of the separate bridge library mne_acq_fiff, the only mne_acq component that depends on FIFF
 * (and therefore Qt); mne_acq itself stays standard-library only.
 *
 * Round trip: per-channel fields map to AcqChannelInfo (name, kind, unit, scale = cal * range) and
 * are kept verbatim in AcqChannelInfo::fiff; everything else of FiffInfo is stored serialized in an
 * AcqExtension with schema "fiff.MeasInfo/1". toFiffInfo(fromFiffInfo(x)) equals x field by field.
 * When a channel was edited in AcqStreamInfo, the edited per-channel fields win.
 */

#ifndef ACQ_FIFF_H
#define ACQ_FIFF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_fiff_global.h"
#include "../acq_error.h"
#include "../acq_sample_format.h"
#include "../acq_stream_info.h"

#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Describes a FIFF measurement as a stream.
 *
 * @param[in] info     FIFF measurement info.
 * @param[in] format   Native format the producer will store (FIFF replay: the file's data type).
 *
 * @return The stream description, version 0.
 */
ACQFIFFSHARED_EXPORT AcqStreamInfo fromFiffInfo(const mne::FiffInfo& info,
                                                AcqSampleFormat format = AcqSampleFormat::Float32);

//=============================================================================================================
/**
 * Builds FIFF measurement info for a stream, e.g. for recording any source to FIFF.
 *
 * @param[in] info   Stream description. Channels without AcqChannelInfo::fiff get FIFF fields derived
 *                   from kind and unit; an AcqUnit::Unknown channel becomes FIFFV_MISC_CH with
 *                   FIFF_UNIT_NONE rather than a guessed unit.
 *
 * @return The FIFF info, or UnsupportedFormat for complex or segmented streams (no FIFF representation).
 */
ACQFIFFSHARED_EXPORT AcqExpected<mne::FiffInfo> toFiffInfo(const AcqStreamInfo& info);

} // namespace mne

#endif // ACQ_FIFF_H
