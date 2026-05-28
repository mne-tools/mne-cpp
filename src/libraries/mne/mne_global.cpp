//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     mne_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    MNELIB build-info helper implementation.
 *
 * Provides the out-of-line definition of @ref MNELIB::buildDateTime which
 * returns the timestamp baked in by @c utils/buildinfo.h. Kept in a
 * separate translation unit so that touching the build date does not
 * force a recompile of every consumer of @c mne_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* MNELIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* MNELIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* MNELIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
