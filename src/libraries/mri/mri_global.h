//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Export/import macros and build-info accessors for mne_mri.
 *
 * Provides the @c MRISHARED_EXPORT decorator used by every public
 * symbol in the MRI library so that the same headers can be
 * consumed by a shared-library build (export when compiling
 * MNE_MRI_LIBRARY, import when linking against it) and a
 * static build (empty decorator). Also exposes @c buildDateTime()
 * and @c buildHash() for runtime provenance reporting --- e.g.
 * when @c mne_analyze prints the linked-library version table
 * in its About dialog or when the registry validator in
 * @c scripts/validate_api_registry.py needs to confirm which MRI
 * library snapshot is wired into a deployed binary.
 */

#ifndef MRI_GLOBAL_H
#define MRI_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define MRISHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MRI_LIBRARY)
#define MRISHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MRISHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MRISHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
 * Returns the build date and time.
 *
 * @return Static C string with the build timestamp.
 */
MRISHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns the short git hash.
 *
 * @return Static C string with the short git commit hash.
 */
MRISHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns the long git hash.
 *
 * @return Static C string with the full git commit hash.
 */
MRISHARED_EXPORT const char* buildHashLong();

} // namespace mne

#endif // MRI_GLOBAL_H
