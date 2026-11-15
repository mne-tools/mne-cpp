//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     brainflowboard_global.cpp
 * @author   Juan G Prieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    BrainFlow plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowboard_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* BRAINFLOWBOARDPLUGIN::buildDateTime()
{
    return mne::dateTimeNow();
}

//=============================================================================================================

const char* BRAINFLOWBOARDPLUGIN::buildHash()
{
    return mne::gitHash();
}

//=============================================================================================================

const char* BRAINFLOWBOARDPLUGIN::buildHashLong()
{
    return mne::gitHashLong();
}
