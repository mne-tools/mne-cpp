//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     utils_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Out-of-line definitions for the UTILSLIB build-info accessors declared in @ref utils_global.h.
 *
 * Kept as a separate translation unit so the @c __DATE__ and
 * @c __TIME__ macros are expanded when this file is compiled
 * rather than when a downstream header is included — the
 * compile-time @c constexpr helpers in @ref buildinfo.h would
 * otherwise pick up the consumer's preprocessor invocation time
 * and report misleading build timestamps to bug reporters.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* UTILSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* UTILSLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* UTILSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
