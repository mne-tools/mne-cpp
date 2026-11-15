//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    mne_lsl shared-library export macro and build-stamp accessors used by every public LSL symbol.
 *
 * This header defines the @c LSLSHARED_EXPORT decoration that every
 * exported class and free function in mne_lsl must carry so that the
 * library can be linked both as a shared library (the default
 * mne-cpp build) and as a statically linked archive (the WebAssembly
 * and @c STATICBUILD configurations). The macro expands to
 * the export decoration when @c MNE_LSL_LIBRARY is defined during the
 * library's own translation units and to the import one when the
 * header is consumed by a downstream module, mirroring the convention
 * used throughout the other mne-cpp libraries.
 *
 * It also declares the @c mne_lsl namespace itself and three
 * @c buildDateTime / @c buildHash / @c buildHashLong free functions
 * that surface the per-translation-unit build stamp produced by
 * @c utils/buildinfo.h. Those accessors are wired into the build-info
 * dialogs of every application that links against mne_lsl so the user
 * can confirm which git revision of the LSL stack a running binary
 * was compiled from without having to launch a debugger.
 */

#ifndef LSL_GLOBAL_H
#define LSL_GLOBAL_H

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
#define LSLSHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_LSL_LIBRARY)
#define LSLSHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define LSLSHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define LSLSHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * @return Null-terminated string with the compile date and time of the library.
 */
LSLSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash the library was built from.
 */
LSLSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash the library was built from.
 */
LSLSHARED_EXPORT const char* buildHashLong();

} // namespace mne

#endif // LSL_GLOBAL_H
