//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwd_global.cpp
 * @since 2022
 * @date  March 2026
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
