//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August 2012
 * @brief    mne_mne shared-library export/import macros and library build metadata.
 *
 * mne_mne is the central mne-cpp data library: it ties the on-disk FIFF
 * containers exposed by mne_fiff to the higher-level neuroscience objects
 * (source spaces, hemispheres, BEM surfaces, forward solutions, inverse
 * operators, covariance matrices, raw/epoch/evoked aggregates and the
 * MSH display surfaces consumed by DISP3D). This translation unit only
 * declares the @c MNESHARED_EXPORT visibility macro and the
 * @ref mne::buildDateTime helper so every other compilation unit can
 * resolve symbol visibility identically across static and shared builds.
 */

#ifndef MNE_GLOBAL_H
#define MNE_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>
#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MNESHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MNE_LIBRARY)
#define MNESHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MNESHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MNESHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
MNESHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Static C string with the short git commit hash, or a "not defined" notice if unavailable at build time.
 */
MNESHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Static C string with the full git commit hash, or a "not defined" notice if unavailable at build time.
 */
MNESHARED_EXPORT const char* buildHashLong();
}

#endif // MNE_GLOBAL_H
