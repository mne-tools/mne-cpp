//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    MNE-CPP BIDS library entry point: Qt symbol-visibility plumbing and the @ref BIDSLIB namespace documentation.
 *
 * Every public symbol in the BIDS library is annotated with
 * @c BIDSSHARED_EXPORT, which expands to @c Q_DECL_EXPORT when the
 * shared library itself is being built, @c Q_DECL_IMPORT for a
 * downstream consumer, and to nothing when the project is configured
 * with @c STATICBUILD. Including this header also pulls in
 * @c utils/buildinfo.h so the @ref BIDSLIB::buildDateTime, @ref
 * BIDSLIB::buildHash and @ref BIDSLIB::buildHashLong free functions
 * are available to clients that need to log which build of the BIDS
 * library they linked against.
 */

#ifndef BIDS_GLOBAL_H
#define BIDS_GLOBAL_H

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
#  define BIDSSHARED_EXPORT
#elif defined(MNE_BIDS_LIBRARY)
#  define BIDSSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define BIDSSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace BIDSLIB
 * @brief     BIDS dataset reading, writing, path construction, and sidecar metadata handling for iEEG/EEG/MEG.
 */
namespace BIDSLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
BIDSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
BIDSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
BIDSSHARED_EXPORT const char* buildHashLong();
}

#endif // BIDS_GLOBAL_H
