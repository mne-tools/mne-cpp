//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file conn_global.h
 * @since July 2016
 * @brief Export/import macros for the @c CONNLIB functional-connectivity library.
 *
 * @c CONNLIB implements the spectral and time-domain functional-connectivity
 * estimators used by mne-cpp's source-space and sensor-space analysis
 * pipelines: magnitude-squared coherence and complex coherency, the
 * imaginary part of coherency (Nolte et al., 2004), the phase-locking value
 * (Lachaux et al., 1999), the phase lag index (Stam et al., 2007), the
 * weighted and debiased squared weighted PLI (Vinck et al., 2011), the
 * unbiased squared PLI, Pearson and time-lagged cross-correlation, and the
 * MVAR-based directed metrics Granger causality, DTF and PDC (Baccala &
 * Sameshima, 2001). This header provides the @c CONNSHARED_EXPORT macro that
 * resolves to @c Q_DECL_EXPORT when @c CONN_LIBRARY is defined during the
 * library build and to @c Q_DECL_IMPORT for downstream consumers, keeping
 * Windows DLL symbol visibility correct without per-class boilerplate.
 */

#ifndef CONN_GLOBAL_H
#define CONN_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define CONNSHARED_EXPORT
#elif defined(MNE_CONN_LIBRARY)
#  define CONNSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define CONNSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace CONNLIB
 * @brief     Functional connectivity metrics (coherence, PLV, cross-correlation, etc.).
 */
namespace CONNLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
CONNSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
CONNSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
CONNSHARED_EXPORT const char* buildHashLong();
}

#endif // CONN_GLOBAL_H
