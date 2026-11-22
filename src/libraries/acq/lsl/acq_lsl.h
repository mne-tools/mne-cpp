//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_lsl.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The lsl:// source: reads a Lab Streaming Layer stream.
 *
 * lsl://NAME opens the LSL stream called NAME (or the first stream matching type= or source_id=) as a device
 * with one input stream. It uses only the in-tree mne_lsl implementation, which speaks the LSL protocol
 * (discovery, data protocol 1.10, full info, time correction) with any LSL outlet, liblsl's included; liblsl
 * is not loaded. AcqDeviceDescription::backendVersion is "in-tree".
 *
 * Samples keep the stream's format (float32, double64, int32, int16; int8 is widened to int16 and int64 is
 * read as double), and channel names, kinds and units come from the stream description (desc/channels/channel
 * with label, type and unit), falling back to the stream type and "TYPE n" names. The clock model maps
 * samples to LSL timestamps plus time_correction(), refreshed every second. Irregular-rate and string
 * (marker) streams are not supported yet.
 *
 * @code
 * mne::registerLslScheme(); // needed only where static linking drops the library's initialiser
 * auto device = mne::AcqDevice::open("lsl://BioSemi?timeout=5");
 * @endcode
 */

#ifndef ACQ_LSL_H
#define ACQ_LSL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_lsl_global.h"
#include "../acq_error.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Registers the lsl:// scheme with AcqDeviceRegistry::instance(). Loading the library registers it already;
 * calling this keeps a static link from dropping the backend. Thread-safe; later calls return the first result.
 *
 * @return Success, or the error of the registration.
 */
ACQLSLSHARED_EXPORT AcqError registerLslScheme();

} // namespace mne

#endif // ACQ_LSL_H
