//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     ftbuffer_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    ftbuffer plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* FTBUFFERPLUGIN::buildDateTime()
{
    return mne::build_date_time();
}

//=============================================================================================================

const char* FTBUFFERPLUGIN::buildHash()
{
    return mne::build_hash();
}

//=============================================================================================================

const char* FTBUFFERPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
}
