//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_scale.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the channel scale functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_scale.h"
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

float mne::viz::default_channel_scale(int kind, int unit)
{
    switch (kind) {
        case FIFFV_MEG_CH:
            if (unit == FIFF_UNIT_T_M) {
                return 1e-15f;
            }
            return unit == FIFF_UNIT_T ? 1e-12f : 1e-9f;
        case FIFFV_REF_MEG_CH:
            return 1e-12f;
        case FIFFV_EEG_CH:
            return 1e-5f;
        case FIFFV_ECG_CH:
            return 1e-2f;
        case FIFFV_EOG_CH:
            return 1e-6f;
        case FIFFV_EMG_CH:
        case FIFFV_STIM_CH:
        case FIFFV_MISC_CH:
            return 1e-3f;
        default:
            return 1e-9f;
    }
}

//=============================================================================================================

float mne::viz::channel_scale(const std::map<int, float>& scales, int kind, int unit)
{
    const auto it = scales.find(kind == FIFFV_MEG_CH ? unit : kind);
    const float scale = it != scales.end() ? it->second : 0.0f;
    if (std::isnan(scale) || scale == 0.0f) {
        return default_channel_scale(kind, unit);
    }
    return scale;
}
