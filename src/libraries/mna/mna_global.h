//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Export/import macros, build-info accessors, and mne_mna namespace anchor for the MNE Analysis Container Format.
 *
 * mne_mna implements the @c .mna (JSON) and @c .mnx (CBOR) project
 * container — a declarative, reproducible record of MEG/EEG analyses
 * spanning subjects, recordings, processing pipelines, parameter
 * trees, verification checks, and provenance. The container is
 * self-describing, lossless on round-trip (unknown keys preserved
 * via @c extras) and version-tagged via @ref mne::MnaProject::CURRENT_SCHEMA_VERSION "MnaProject::CURRENT_SCHEMA_VERSION".
 *
 * This header defines the @c MNASHARED_EXPORT visibility macro that
 * gates every public symbol in the library, the static-build override,
 * and two free functions returning the build date/time and git hash
 * burned into the binary by @c utils/buildinfo.h. The macro must be
 * defined once before any other mne_mna include so the same translation
 * unit cannot accidentally mix import and export linkage.
 */

#ifndef MNA_GLOBAL_H
#define MNA_GLOBAL_H

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
#define MNASHARED_EXPORT
#elif defined(_WIN32)
#if defined(MNE_MNA_LIBRARY)
#define MNASHARED_EXPORT __declspec(dllexport) /**< Exported when building the shared library. */
#else
#define MNASHARED_EXPORT __declspec(dllimport) /**< Imported by clients of the shared library. */
#endif
#else
#define MNASHARED_EXPORT __attribute__((visibility("default"))) /**< Default visibility on ELF/Mach-O. */
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
MNASHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 *
 * @return Null-terminated string with the short git commit hash the library was built from.
 */
MNASHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 *
 * @return Null-terminated string with the full git commit hash the library was built from.
 */
MNASHARED_EXPORT const char* buildHashLong();
}

#endif // MNA_GLOBAL_H
