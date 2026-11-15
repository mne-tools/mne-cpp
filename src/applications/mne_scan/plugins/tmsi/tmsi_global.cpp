//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     tmsi_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    tmsi plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* TMSIPLUGIN::buildDateTime()
{
    return mne::compile_date_time();
}

//=============================================================================================================

const char* TMSIPLUGIN::buildHash()
{
    return mne::git_hash();
}

//=============================================================================================================

const char* TMSIPLUGIN::buildHashLong()
{
    return mne::git_hash_long();
}
