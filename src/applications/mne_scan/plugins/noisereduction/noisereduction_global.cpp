//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     noisereduction_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    noisereduction plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* NOISEREDUCTIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* NOISEREDUCTIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* NOISEREDUCTIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
