//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     utils_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Public umbrella header for UTILSLIB exposing the @c UTILSSHARED_EXPORT macro and build-info accessors.
 *
 * UTILSLIB is the cross-cutting helper library that every other
 * mne-cpp module links against: I/O helpers, layout / selection
 * parsers, command and observer patterns, thread-safe circular
 * buffers, the Polhemus digitizer stack, the Python runner used
 * by tests, and the standard EEG montages. This header carries
 * the @c Q_DECL_EXPORT / @c Q_DECL_IMPORT switch so symbols
 * resolve correctly whether UTILSLIB is built as a shared
 * library, a static archive (@c STATICBUILD) or imported by a
 * downstream consumer.
 *
 * The free functions defined here (@ref buildDateTime,
 * @ref buildHash, @ref buildHashLong) are the runtime-callable
 * entry points for the compile-time constants in
 * @ref buildinfo.h and are surfaced in the About dialog of the
 * mne-cpp applications and in version banners written by CLI
 * tools so bug reports always identify the exact build.
 */

#ifndef UTILS_GLOBAL_H
#define UTILS_GLOBAL_H

#include "buildinfo.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define UTILSSHARED_EXPORT
#elif defined(MNE_UTILS_LIBRARY)
#  define UTILSSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define UTILSSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace UTILSLIB
 * @brief     Shared utilities (I/O helpers, spectral analysis, layout management, warp algorithms).
 */
namespace UTILSLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
UTILSSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
UTILSSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
UTILSSHARED_EXPORT const char* buildHashLong();
}

#endif // MNEUTILS_GLOBAL_H
