//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_global.h
 * @since 2026
 * @date  April 2026
 * @brief Export/import macros, build-info accessors, and MNALIB namespace anchor for the MNE Analysis Container Format.
 *
 * MNALIB implements the @c .mna (JSON) and @c .mnx (CBOR) project
 * container — a declarative, reproducible record of MEG/EEG analyses
 * spanning subjects, recordings, processing pipelines, parameter
 * trees, verification checks, and provenance. The container is
 * self-describing, lossless on round-trip (unknown keys preserved
 * via @c extras) and version-tagged via @ref MnaProject::CURRENT_SCHEMA_VERSION.
 *
 * This header defines the @c MNASHARED_EXPORT visibility macro that
 * gates every public symbol in the library, the static-build override,
 * and two free functions returning the build date/time and git hash
 * burned into the binary by @c utils/buildinfo.h. The macro must be
 * defined once before any other MNALIB include so the same translation
 * unit cannot accidentally mix import and export linkage.
 */

#ifndef MNA_GLOBAL_H
#define MNA_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define MNASHARED_EXPORT
#elif defined(MNE_MNA_LIBRARY)
#  define MNASHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define MNASHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace MNALIB
 * @brief     MNE Analysis Container Format (mna/mnx).
 */
namespace MNALIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
MNASHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
MNASHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
MNASHARED_EXPORT const char* buildHashLong();
}

#endif // MNA_GLOBAL_H
