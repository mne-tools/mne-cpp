//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     dummytoolbox_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     November, 2022
 * @brief    averaging plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummytoolbox_global.h"
#include <mne/base/base_global.h>

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildDateTime()
{
    return mne::build_date_time();
};

//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildHash()
{
    return mne::build_hash();
};

//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
};
