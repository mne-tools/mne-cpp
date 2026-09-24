//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     dummytoolbox_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     November, 2022
 * @brief    averaging plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummytoolbox_global.h"
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

//=============================================================================================================

const char* DUMMYTOOLBOXPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
