//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     sampleplugin_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    sampleplugin plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sampleplugin_global.h"
#include <base/buildinfo.h>

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* SAMPLEPLUGINPLUGIN::buildDateTime()
{
    return mne::dateTimeNow();
};

//=============================================================================================================

const char* SAMPLEPLUGINPLUGIN::buildHash()
{
    return mne::gitHash();
};

//=============================================================================================================

const char* SAMPLEPLUGINPLUGIN::buildHashLong()
{
    return mne::gitHashLong();
};
