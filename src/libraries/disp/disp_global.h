//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     disp_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Export macros and build-info hooks for the DISPLIB shared library.
 *
 * Defines @c DISPSHARED_EXPORT / @c DISPSHARED_IMPORT for Windows DLL
 * visibility and exposes a single @c DISPLIB::buildDateTime() entry
 * point so any of the disp viewers / plots / models can stamp the same
 * MNE-CPP build date into status bars or log files. Every public class
 * in this library declares itself with @c DISPSHARED_EXPORT to be
 * usable from downstream applications and plugins.
 */

#ifndef DISP_GLOBAL_H
#define DISP_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define DISPSHARED_EXPORT
#elif defined(MNE_DISP_LIBRARY)
#  define DISPSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DISPSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace DISPLIB
 * @brief     2-D display widgets and visualisation helpers (charts, topography, colour maps).
 */
namespace DISPLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
DISPSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DISPSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DISPSHARED_EXPORT const char* buildHashLong();
}

#endif // DISP_GLOBAL_H
