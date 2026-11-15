//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_devices.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Registration of all device schemes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_devices.h"

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

mne::AcqError mne::register_device_schemes()
{
    for (AcqError (*registration)() : {&registerBrainAmpScheme, &registerTmsiScheme, &register_gusbamp_scheme, &register_eego_scheme,
                                       &register_neuromag_scheme, &register_polhemus_scheme, &register_ftbuffer_scheme,
                                       &register_rtserver_scheme, &register_natus_scheme, &register_babymeg_scheme,
                                       &register_brainflow_scheme, &register_streaming_board_scheme,
                                       &register_brainflow_file_scheme}) {
        if (AcqError error = registration(); !error.ok()) {
            return error;
        }
    }
    return {};
}
