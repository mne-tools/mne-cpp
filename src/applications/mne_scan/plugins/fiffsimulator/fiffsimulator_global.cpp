//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    fiffsimulator plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* FIFFSIMULATORPLUGIN::buildDateTime()
{
    return mne::compile_date_time();
}

//=============================================================================================================

const char* FIFFSIMULATORPLUGIN::buildHash()
{
    return mne::git_hash();
}

//=============================================================================================================

const char* FIFFSIMULATORPLUGIN::buildHashLong()
{
    return mne::git_hash_long();
}
