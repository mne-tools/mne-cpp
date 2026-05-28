//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file com_global.cpp
 * @since 2026
 * @date  March 2026
 * @brief Translation unit for the @c COMLIB build-info entry points declared in @ref com_global.h.
 *
 * Provides the three out-of-line definitions of @ref COMLIB::buildDateTime,
 * @ref COMLIB::buildHash and @ref COMLIB::buildHashLong. Each delegates
 * to the equivalent function in @c UTILSLIB so the actual @c __DATE__ /
 * @c __TIME__ / @c MNE_GIT_HASH_* compile-time strings are owned by a
 * single utility translation unit and the values reported across all
 * mne-cpp libraries stay consistent.
 *
 * The CMake build re-runs this file whenever any other source under
 * @c src/libraries/com/ changes (@c OBJECT_DEPENDS in the parent
 * @c CMakeLists.txt) so the embedded build timestamp tracks the most
 * recent edit to the library rather than the last full reconfigure.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "com_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* COMLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* COMLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* COMLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
