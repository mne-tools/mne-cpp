//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     events_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    events plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* EVENTSPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* EVENTSPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* EVENTSPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}

