//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fwd_global.cpp
 * @since August 2021
 * @brief FWDLIB build-info implementation — returns the embedded build timestamp and short/long git hashes used in the library banner.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* FWDLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* FWDLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* FWDLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
