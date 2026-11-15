//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     sourcelocalization_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    sourcelocalization plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelocalization_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* SOURCELOCALIZATIONPLUGIN::buildDateTime()
{
    return mne::compile_date_time();
}

//=============================================================================================================

const char* SOURCELOCALIZATIONPLUGIN::buildHash()
{
    return mne::git_hash();
}

//=============================================================================================================

const char* SOURCELOCALIZATIONPLUGIN::buildHashLong()
{
    return mne::git_hash_long();
}
