//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     babymeg_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    babymeg plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* BABYMEGPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* BABYMEGPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* BABYMEGPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
