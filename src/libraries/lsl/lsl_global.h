//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_global.h
 * @since February 2026
 * @brief LSLLIB shared-library export macro and build-stamp accessors used by every public LSL symbol.
 *
 * This header defines the @c LSLSHARED_EXPORT decoration that every
 * exported class and free function in LSLLIB must carry so that the
 * library can be linked both as a Qt shared library (the default
 * mne-cpp build) and as a statically linked archive (the WebAssembly
 * and @c STATICBUILD configurations). The macro expands to
 * @c Q_DECL_EXPORT when @c MNE_LSL_LIBRARY is defined during the
 * library's own translation units and to @c Q_DECL_IMPORT when the
 * header is consumed by a downstream module, mirroring the convention
 * used throughout the other mne-cpp libraries.
 *
 * It also declares the @ref LSLLIB namespace itself and three
 * @c buildDateTime / @c buildHash / @c buildHashLong free functions
 * that surface the per-translation-unit build stamp produced by
 * @c utils/buildinfo.h. Those accessors are wired into the build-info
 * dialogs of every application that links against LSLLIB so the user
 * can confirm which git revision of the LSL stack a running binary
 * was compiled from without having to launch a debugger.
 */

#ifndef LSL_GLOBAL_H
#define LSL_GLOBAL_H

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
#  define LSLSHARED_EXPORT
#elif defined(MNE_LSL_LIBRARY)
#  define LSLSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define LSLSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace LSLLIB
 * @brief     Lab Streaming Layer (LSL) integration for real-time data exchange.
 */
namespace LSLLIB {

//=============================================================================================================
/**
 * Returns build date and time.
 */
LSLSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
LSLSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
LSLSHARED_EXPORT const char* buildHashLong();

} // namespace LSLLIB

#endif // LSL_GLOBAL_H
