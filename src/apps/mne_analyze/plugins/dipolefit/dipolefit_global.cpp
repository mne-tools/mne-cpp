//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     dipolefit_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    dipolefit plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefit_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildDateTime()
{
    return mne::build_date_time();
}

//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildHash()
{
    return mne::build_hash();
}

//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
}
