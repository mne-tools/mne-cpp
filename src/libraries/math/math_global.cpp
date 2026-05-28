//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file math_global.cpp
 * @since March 2026
 * @brief Thin forwarders that surface UTILSLIB build-info into MATHLIB.
 *
 * The three accessors declared in @ref math_global.h simply delegate to
 * the equivalent helpers in @ref UTILSLIB::buildinfo so that every
 * mne-cpp library exposes the same build-stamp API surface without
 * duplicating the @c __DATE__ / @c __TIME__ macros or having to know
 * how the CMake git-hash injection works. Keeping the implementation in
 * a separate translation unit also pins the build stamp to the moment
 * MATHLIB itself was compiled, not to whichever client first includes
 * @ref math_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* MATHLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* MATHLIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* MATHLIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
