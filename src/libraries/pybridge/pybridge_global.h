//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pybridge_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Export macro and build-info accessors of mne_pybridge.
 *
 * mne_pybridge runs Python as a child process: scripts, one-liners and MNE-Python
 * cross-validation in tests, without linking against libpython.
 */

#ifndef PYBRIDGE_GLOBAL_H
#define PYBRIDGE_GLOBAL_H

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define PYBRIDGESHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_PYBRIDGE_LIBRARY)
#define PYBRIDGESHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define PYBRIDGESHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define PYBRIDGESHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
PYBRIDGESHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash of the build.
 */
PYBRIDGESHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash of the build.
 */
PYBRIDGESHARED_EXPORT const char* buildHashLong();
}

#endif // MNEPYBRIDGE_GLOBAL_H
