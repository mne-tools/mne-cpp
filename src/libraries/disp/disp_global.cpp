//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file disp_global.cpp
 * @since 2022
 * @date  January 2024
 * @brief Implementation of the build-info hook for the DISPLIB shared library.
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
