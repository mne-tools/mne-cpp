//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     rawdataviewer_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    rawdataviewer plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* RAWDATAVIEWERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* RAWDATAVIEWERPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* RAWDATAVIEWERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
