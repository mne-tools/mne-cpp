//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     dipolefit_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    dipolefit plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefit_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* DIPOLEFITPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
