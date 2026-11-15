//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     datamanager_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    datamanager plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamanager_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DATAMANAGERPLUGIN::buildDateTime()
{
    return mne::compile_date_time();
}

//=============================================================================================================

const char* DATAMANAGERPLUGIN::buildHash()
{
    return mne::git_hash();
}

//=============================================================================================================

const char* DATAMANAGERPLUGIN::buildHashLong()
{
    return mne::git_hash_long();
}
