//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     lsladapter_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    lsladapter plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* LSLADAPTERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* LSLADAPTERPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* LSLADAPTERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
