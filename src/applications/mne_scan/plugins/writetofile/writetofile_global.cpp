//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     writetofile_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    writetofile plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "writetofile_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* WRITETOFILEPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* WRITETOFILEPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* WRITETOFILEPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
