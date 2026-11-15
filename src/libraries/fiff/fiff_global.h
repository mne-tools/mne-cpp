//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Export/import macros and build-info accessors for the mne_fiff shared library.
 *
 * This header is included first by every public mne_fiff symbol and is the
 * only place where the export / import visibility decoration
 * is chosen. STATICBUILD strips the decoration entirely so the same
 * sources can be linked into a monolithic static archive (used for the
 * WebAssembly build and for the all-in-one fat library on Windows).
 *
 * The library also re-exports three string accessors backed by
 * ``utils/buildinfo.h`` so that downstream tools can stamp the FIFF
 * build identity (timestamp, short hash, long hash) into the FIFF
 * @c FIFF_MNE_CREATOR and @c FIFF_PROC_HISTORY tags when writing files,
 * keeping provenance with parity to MNE-Python's @c mne.__version__ field
 * that lands in the same tags.
 */

#ifndef FIFF_GLOBAL_H
#define FIFF_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>
#include <base/string_utils.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FIFFSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FIFF_LIBRARY)
#define FIFFSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FIFFSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FIFFSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * @return Null-terminated string with the compile date and time of the library.
 */
FIFFSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash the library was built from.
 */
FIFFSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash the library was built from.
 */
FIFFSHARED_EXPORT const char* buildHashLong();
}

#endif // FIFF_GLOBAL_H
