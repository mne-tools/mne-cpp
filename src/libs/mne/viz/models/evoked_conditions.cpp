//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     evoked_conditions.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the evoked condition bookkeeping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "evoked_conditions.h"

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

int mne::viz::pre_stimulus_samples(std::span<const float> times)
{
    const auto onset = std::ranges::find(times, 0.0f);
    return static_cast<int>(onset - times.begin());
}
