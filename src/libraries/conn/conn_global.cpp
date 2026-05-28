//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file conn_global.cpp
 * @since 2026
 * @date  March 2026
 * @brief Translation unit anchoring the @c CONNLIB shared-library symbols.
 *
 * The file intentionally contains no executable code: it exists so that
 * the build system has at least one @c .cpp unit to compile when building
 * @c CONNLIB as a shared library on platforms (Windows) that require an
 * explicit object to host the exported symbols declared via the
 * @c CONNSHARED_EXPORT macro in @ref conn_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "conn_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* CONNLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* CONNLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* CONNLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
