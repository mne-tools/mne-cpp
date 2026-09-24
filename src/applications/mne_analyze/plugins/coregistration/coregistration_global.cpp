//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     coregistration_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    coregistration plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregistration_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* COREGISTRATIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* COREGISTRATIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* COREGISTRATIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
