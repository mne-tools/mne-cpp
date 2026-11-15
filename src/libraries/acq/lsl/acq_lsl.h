//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_lsl.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    The lsl:// source: reads a Lab Streaming Layer stream.
 *
 * lsl://NAME opens the LSL stream called NAME (or the first stream matching type= or source_id=) as a device
 * with one input stream. liblsl is loaded at runtime when it is installed: MNE_CPP_LIBLSL (a file path), the
 * library that pylsl ships (pip install pylsl), then the system library paths. Without liblsl the in-tree
 * mne_lsl implementation is used, which speaks the same protocol (discovery, data protocol 1.10, time
 * correction) with any LSL outlet. AcqDeviceDescription::backendVersion names the path in use ("liblsl 1.16"
 * or "in-tree"); library=liblsl or library=intree forces one.
 *
 * With liblsl, samples keep the stream's format (float32, double64, int32, int16; int8 is widened to int16 and
 * int64 is read as double) and channel names, kinds and units come from the stream description
 * (desc/channels/channel with label, type and unit). The in-tree path delivers float32 (exact for float32,
 * int16 and int8 streams) and names channels after the stream type. On both paths the clock model maps
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

#include <string>

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

//=============================================================================================================
/**
 * @return "liblsl <version> (<path>)" for a liblsl that loads, otherwise "in-tree" and why liblsl is missing.
 */
ACQLSLSHARED_EXPORT std::string lslRuntime();

} // namespace mne

#endif // ACQ_LSL_H
