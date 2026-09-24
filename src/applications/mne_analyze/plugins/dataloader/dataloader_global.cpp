//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     dataloader_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    dataloader plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DATALOADERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* DATALOADERPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* DATALOADERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
