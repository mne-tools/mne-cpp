//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     buildinfo.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     August 2021
 * @brief    Compile-time helpers that capture the build date, time and git commit hash into the binary.
 *
 * Every accessor is a header-only @c constexpr function so the
 * @c __DATE__ / @c __TIME__ macros and the @c MNE_GIT_HASH_*
 * defines (injected by CMake at configure time) are evaluated
 * in the translation unit that invokes them. The values are
 * picked up by @ref utils_global.cpp exactly once so every
 * mne-cpp binary reports a single, stable build identifier in
 * its About dialog, CLI @c --version banner, and crash logs.
 *
 * When git metadata is unavailable (source tarball builds,
 * shallow CI checkouts) the hash accessors return a constant
 * sentinel string instead of failing the build.
 */

#ifndef BUILD_INFO_LIB
#define BUILD_INFO_LIB

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstring>

namespace UTILSLIB{

//=============================================================================================================
/**
 * Returns build time (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto timeNowNow()
{
    return __TIME__;
}

//=============================================================================================================
/**
 * Returns build date (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto dateToday()
{
    return __DATE__;
}

//=============================================================================================================
/**
 * Returns build date and time (time preprocessor was run). Must be called in compiled function to return
 * correctly.
 */
constexpr auto dateTimeNow()
{
    return __DATE__ " " __TIME__;
}

//=============================================================================================================
/**
 * Returns short version of the hash of the current git commit
 */
constexpr auto gitHash()
{
#ifdef MNE_GIT_HASH_SHORT
    return MNE_GIT_HASH_SHORT;
#else
    return "Git hash not defined.";
#endif
}

//=============================================================================================================
/**
 * Returns entire hash of the current git commit
 */
constexpr auto gitHashLong()
{
#ifdef MNE_GIT_HASH_LONG
    return MNE_GIT_HASH_LONG;
#else
    return "Git hash long not defined.";
#endif
}

}

#endif
