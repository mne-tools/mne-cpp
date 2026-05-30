//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     connectivity_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Translation unit anchoring the @c CONNECTIVITYLIB shared-library symbols.
 *
 * The file intentionally contains no executable code: it exists so that
 * the build system has at least one @c .cpp unit to compile when building
 * @c CONNECTIVITYLIB as a shared library on platforms (Windows) that require an
 * explicit object to host the exported symbols declared via the
 * @c CONNECTIVITYSHARED_EXPORT macro in @ref connectivity_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* CONNECTIVITYLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* CONNECTIVITYLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* CONNECTIVITYLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
