//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_global.h
 * @since 2022
 * @date  March 2026
 * @brief Export/import macros and build-info accessors for the FIFFLIB shared library.
 *
 * This header is included first by every public FIFFLIB symbol and is the
 * only place where the Q_DECL_EXPORT / Q_DECL_IMPORT visibility decoration
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

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define FIFFSHARED_EXPORT
#elif defined(MNE_FIFF_LIBRARY)
#  define FIFFSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define FIFFSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @brief FIFF file I/O, in-memory data structures and high-level readers/writers.
 *
 * Houses every class that round-trips an Elekta/Neuromag FIFF file:
 * the stream layer (@ref FiffStream, @ref FiffTag, @ref FiffDirNode,
 * @ref FiffDirEntry), the measurement metadata (@ref FiffInfo,
 * @ref FiffChInfo, @ref FiffCoordTrans, @ref FiffId, @ref FiffDigPoint),
 * the data containers (@ref FiffRawData, @ref FiffEvoked,
 * @ref FiffEvokedSet, @ref FiffCov, @ref FiffProj, @ref FiffCtfComp,
 * @ref FiffNamedMatrix, @ref FiffSparseMatrix) and the convenience facade
 * @ref FIFF. Surface-level parity with @c mne.io.fiff in MNE-Python.
 */
namespace FIFFLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
FIFFSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
FIFFSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
FIFFSHARED_EXPORT const char* buildHashLong();
}

#endif // FIFF_GLOBAL_H
