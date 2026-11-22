//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     events_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    events plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* EVENTSPLUGIN::buildDateTime()
{
    return mne::build_date_time();
}

//=============================================================================================================

const char* EVENTSPLUGIN::buildHash()
{
    return mne::build_hash();
}

//=============================================================================================================

const char* EVENTSPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
}
