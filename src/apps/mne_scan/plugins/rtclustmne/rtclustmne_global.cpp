//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     rtclustmne_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    rtclustmne plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtclustmne_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* RTCLUSTMNEPLUGIN::buildDateTime()
{
    return mne::build_date_time();
}

//=============================================================================================================

const char* RTCLUSTMNEPLUGIN::buildHash()
{
    return mne::build_hash();
}

//=============================================================================================================

const char* RTCLUSTMNEPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
}
