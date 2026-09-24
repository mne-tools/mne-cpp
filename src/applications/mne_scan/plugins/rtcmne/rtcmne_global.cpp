//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     rtcmne_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    rtcmne plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmne_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* RTCMNEPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* RTCMNEPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* RTCMNEPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
