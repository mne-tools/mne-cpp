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
    return mne::dateTimeNow();
}

//=============================================================================================================

const char* FTBUFFERPLUGIN::buildHash()
{
    return mne::gitHash();
}

//=============================================================================================================

const char* FTBUFFERPLUGIN::buildHashLong()
{
    return mne::gitHashLong();
}
