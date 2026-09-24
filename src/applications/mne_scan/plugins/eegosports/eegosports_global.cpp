//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     eegosports_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    eegosports plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosports_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* EEGOSPORTSPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* EEGOSPORTSPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* EEGOSPORTSPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
