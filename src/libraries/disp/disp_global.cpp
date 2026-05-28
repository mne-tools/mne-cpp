//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     disp_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Implementation of the build-info hook for the DISPLIB shared library.
 *
 * Delegates @c DISPLIB::buildDateTime() to @c UTILSLIB::dateTimeNow() so
 * every consumer of the disp library reports the same MNE-CPP build
 * timestamp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* DISPLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* DISPLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* DISPLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
