//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    MNE-CPP BIDS library entry point: symbol-visibility macro and the @c mne_bids namespace documentation.
 *
 * Every public symbol in the BIDS library is annotated with
 * @c BIDSSHARED_EXPORT, which exports the symbols when the
 * shared library itself is being built, imports them for a
 * downstream consumer, and to nothing when the project is configured
 * with @c STATICBUILD. Including this header also pulls in
 * @c utils/buildinfo.h so the @ref mne::buildDateTime, @ref
 * mne::buildHash and @ref mne::buildHashLong free functions
 * are available to clients that need to log which build of the BIDS
 * library they linked against.
 */

#ifndef BIDS_GLOBAL_H
#define BIDS_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define BIDSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_BIDS_LIBRARY)
#define BIDSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define BIDSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define BIDSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
#endif

// Exported classes hold Eigen and standard-library members and share their ABI with clients built by the same
// compiler, so MSVC's warnings about members and bases without dll-interface do not apply (Qt disables them alike).
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

namespace mne
{

//=============================================================================================================
/**
 * Returns build date and time.
 *
 * @return Null-terminated string with the library build date and time.
 */
BIDSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash of the build.
 */
BIDSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash of the build.
 */
BIDSSHARED_EXPORT const char* buildHashLong();
}

#endif // BIDS_GLOBAL_H
