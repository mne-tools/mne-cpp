//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Export/import macros and build-info accessors for the mne_fs FreeSurfer I/O library.
 *
 * Defines the @c FSSHARED_EXPORT visibility macro used by every public
 * class in mne_fs so the library can be built either as a shared
 * library (default) or as a static archive under @c STATICBUILD without
 * source changes. Also exposes the small set of build-info accessors
 * (date/time, abbreviated git hash, full git hash) that downstream
 * tools surface in their About dialogs and log headers.
 */

#ifndef FS_GLOBAL_H
#define FS_GLOBAL_H

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define FSSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_FS_LIBRARY)
#define FSSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define FSSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define FSSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * @return Null-terminated build date/time string with static storage duration.
 */
FSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated abbreviated git hash string with static storage duration.
 */
FSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated full git hash string with static storage duration.
 */
FSSHARED_EXPORT const char* buildHashLong();
}

#endif // FS_GLOBAL_H
