//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     controlmanager_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    controlmanager plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "controlmanager_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* CONTROLMANAGERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* CONTROLMANAGERPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* CONTROLMANAGERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
