//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromag_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    neuromag plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* NEUROMAGPLUGIN::buildDateTime()
{
    return mne::build_date_time();
}

//=============================================================================================================

const char* NEUROMAGPLUGIN::buildHash()
{
    return mne::build_hash();
}

//=============================================================================================================

const char* NEUROMAGPLUGIN::buildHashLong()
{
    return mne::build_hash_long();
}
