//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff_replay.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    The fiff:// source: replays a FIFF raw file as an acquisition stream.
 *
 * fiff://FILE opens a raw FIFF file as a device with one input stream. Samples keep the file's native format
 * (FIFFT_DAU_PACK16 and FIFFT_SHORT as int16, FIFFT_INT as int32, FIFFT_FLOAT as float32, FIFFT_DOUBLE as
 * float64), and each channel's scale is range * cal, so scale * sample equals FiffRawData::read_raw_segment
 * bit for bit; skipped buffers read as zeros. Stream sample 0 is the file sample selected with start.
 *
 * Events come from the trigger channel (stim, or STI101, STI 101, STI 014, then the first stim channel):
 * one at every sample where the value changes to a non-zero value, lasting while the value is held, as
 * mne.find_events(raw, consecutive=True). A value held from the first file sample is not an event.
 *
 * Parameters: loop (restart at the first file sample at the end; default 0, the stream ends), realtime and
 * speed (pacing), start (seconds into the file), block (samples per commit), events, stim, cpus, priority.
 *
 * @code
 * mne::registerFiffScheme(); // once; needed only where static linking drops the library's initialiser
 * auto device = mne::AcqDevice::open("fiff:///data/sample_audvis_raw.fif?loop=1&speed=2");
 * @endcode
 */

#ifndef ACQ_FIFF_REPLAY_H
#define ACQ_FIFF_REPLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_fiff_global.h"
#include "../acq_error.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Registers the fiff:// scheme with AcqDeviceRegistry::instance(). Loading the library registers it already;
 * calling this keeps a static link from dropping the backend. Thread-safe; later calls return the first result.
 *
 * @return Success, or the error of the registration.
 */
ACQFIFFSHARED_EXPORT AcqError registerFiffScheme();

} // namespace mne

#endif // ACQ_FIFF_REPLAY_H
