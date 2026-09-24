//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     sampleplugin_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     August, 2020
 * @brief    Contains the DataViewer library export/import macros.
 */

#ifndef SAMPLEPLUGIN_GLOBAL_H
#define SAMPLEPLUGIN_GLOBAL_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(ANALYZE_SAMPLEPLUGIN_PLUGIN)
#  define SAMPLEPLUGINSHARED_EXPORT Q_DECL_EXPORT   /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define SAMPLEPLUGINSHARED_EXPORT Q_DECL_IMPORT   /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace SAMPLEPLUGINPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
SAMPLEPLUGINSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
SAMPLEPLUGINSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
SAMPLEPLUGINSHARED_EXPORT const char* buildHashLong();
}

#endif // SAMPLEPLUGIN_GLOBAL_H
