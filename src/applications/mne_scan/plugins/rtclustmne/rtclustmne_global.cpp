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
    return UTILSLIB::dateTimeNow();
}

//=============================================================================================================

const char* RTCLUSTMNEPLUGIN::buildHash()
{
    return UTILSLIB::gitHash();
}

//=============================================================================================================

const char* RTCLUSTMNEPLUGIN::buildHashLong()
{
    return UTILSLIB::gitHashLong();
}
