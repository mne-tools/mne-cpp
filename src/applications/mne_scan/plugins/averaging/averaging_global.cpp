//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     averaging_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    averaging plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* AVERAGINGPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* AVERAGINGPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* AVERAGINGPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
