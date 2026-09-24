//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     channelselection_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    channelselection plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelselection_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* CHANNELSELECTIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* CHANNELSELECTIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* CHANNELSELECTIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
