//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    mne_inv library export/import macros, build-info accessors, and namespace docstring for the inverse-solver library.
 *
 * The mne_inv library bundles every inverse-method, beamformer and HPI-fit
 * class used by mne-cpp: minimum-norm / dSPM / sLORETA / eLORETA, LCMV and
 * DICS beamformers, RAP-MUSIC, TRAP-MUSIC, MxNE / TF-MxNE / Gamma-MAP,
 * contextual MNE (CMNE), ECD dipole fitting and continuous head-position
 * estimation. This header defines the @c INVSHARED_EXPORT macro that
 * controls symbol visibility for the shared/static build variants, declares
 * the @c mne_inv namespace, and exposes @ref mne::buildDateTime "buildDateTime" and
 * @ref mne::buildHash "buildHash" so applications can stamp the linked mne_inv version into
 * log files and reports.
 */

#ifndef INV_GLOBAL_H
#define INV_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>
#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define INVSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_INV_LIBRARY)
#define INVSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define INVSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define INVSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

namespace mne
{

using mne::StringList;

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Static C string with the compile date and time, e.g. "Sep 30 2026 14:05:12".
 */
INVSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
INVSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
INVSHARED_EXPORT const char* buildHashLong();

}

#endif // INV_GLOBAL_H
