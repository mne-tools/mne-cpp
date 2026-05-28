//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_global.cpp
 * @since August 2021
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
