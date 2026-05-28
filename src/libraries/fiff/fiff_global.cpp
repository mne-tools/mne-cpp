//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_global.cpp
 * @since 2022
 * @date  January 2024
 * @brief Definitions for the FIFFLIB build-info accessors declared in @ref fiff_global.h.
 *
 * Each accessor forwards to the matching macro defined by
 * @c utils/buildinfo.h so the FIFF build identity (timestamp, short hash,
 * long hash) can be stamped into @c FIFF_MNE_CREATOR / @c FIFF_PROC_HISTORY
 * on write.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* FIFFLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* FIFFLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* FIFFLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
