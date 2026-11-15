//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_devices.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Sources for vendor EEG amplifiers, each with a simulator of its wire format.
 *
 * Each amplifier is a URI scheme of AcqDeviceRegistry. The streams carry the amplifier's own sample format
 * (BrainAmp: 16-bit channels in steps of the chosen resolution, then the marker word, at 5 kHz; TMSi: a 32-bit
 * integer per channel with the SDK's gain, offset and exponent; g.USBamp: a float per channel in microvolts; eego: a double per channel in volts), so readers
 * see exactly what the driver delivers. simulate=1 replaces the vendor driver with a simulator that digitises
 * a synthetic:// session in that format, which runs on every platform and needs no hardware; seed, realtime
 * and duration work as for synthetic://. The real driver interfaces are Windows-only: BrainAmp's is compiled when
 * the vendor's SDK header is found (CMake variable MNE_BRAINAMP_SDK_DIR), TMSi's and g.USBamp's load the vendor's
 * system DLL at run time, eego's loads the eego SDK library on Windows and Linux. Without them, opening without
 * simulate=1 fails with BackendUnavailable.
 *
 * @code
 * mne::registerBrainAmpScheme(); // needed only where static linking drops the library's initialiser
 * auto device = mne::AcqDevice::open("brainamp://?simulate=1&channels=32");
 * @endcode
 */

#ifndef ACQ_DEVICES_H
#define ACQ_DEVICES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_devices_global.h"
#include "../acq_error.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * Registers the brainamp:// scheme (Brain Products BrainAmp) with AcqDeviceRegistry::instance(). Loading the
 * library registers it already; calling this keeps a static link from dropping the backend. Thread-safe;
 * later calls return the first result.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError registerBrainAmpScheme();

/**
 * Registers the tmsi:// scheme (TMSi Refa and Porti) with AcqDeviceRegistry::instance(), as
 * registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError registerTmsiScheme();

/**
 * Registers the gusbamp:// scheme (g.tec g.USBamp, up to four synchronised) with AcqDeviceRegistry::instance(), as
 * registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_gusbamp_scheme();

/**
 * Registers the eego:// scheme (ANT Neuro eego) with AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does
 * for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_eego_scheme();

/**
 * Registers the neuromag:// scheme (the data acquisition of Neuromag/MEGIN MEG systems) with
 * AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_neuromag_scheme();

/**
 * Registers the polhemus:// scheme (Polhemus Fastrak and FastSCAN digitizers on a serial port) with
 * AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_polhemus_scheme();

/**
 * Registers the ftbuffer:// scheme (a FieldTrip buffer server, e.g. filled by neuromag2ft) with
 * AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_ftbuffer_scheme();

/**
 * Registers the rtserver:// scheme (the active connector of an mne_rt_server) with AcqDeviceRegistry::instance(),
 * as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_rtserver_scheme();

/**
 * Registers the natus:// scheme (a Natus Quantum amplifier through the datagrams of its streaming machine) with
 * AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_natus_scheme();

/**
 * Registers the babymeg:// scheme (the acquisition server of a BabyMEG system) with AcqDeviceRegistry::instance(), as
 * registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_babymeg_scheme();

/**
 * Registers the brainflow:// scheme (any board BrainFlow supports, through BrainFlow's library loaded at run time)
 * with AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_brainflow_scheme();

/**
 * Registers the streaming_board:// scheme (what a BrainFlow session streams with add_streamer, received without
 * BrainFlow) with AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_streaming_board_scheme();

/**
 * Registers the brainflow_file:// scheme (a file BrainFlow wrote, replayed without BrainFlow) with
 * AcqDeviceRegistry::instance(), as registerBrainAmpScheme() does for brainamp://.
 *
 * @return Success, or the error of the registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_brainflow_file_scheme();

/**
 * Registers every scheme of this library, as the functions above do one by one; for programs that offer all
 * sources, such as mne_acq.
 *
 * @return Success, or the first error of a registration.
 */
ACQDEVICESSHARED_EXPORT AcqError register_device_schemes();

} // namespace mne

#endif // ACQ_DEVICES_H
