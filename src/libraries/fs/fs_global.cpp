//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fs_global.cpp
 * @since 2022
 * @date  January 2024
 * @brief Build-info accessor implementations (date/time, abbreviated and full git hash) for the FSLIB FreeSurfer I/O library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* FSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* FSLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* FSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
