//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     dummytoolbox_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the DummyToolbox library export/import macros.
 */

#ifndef DUMMYTOOLBOX_GLOBAL_H
#define DUMMYTOOLBOX_GLOBAL_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_DUMMYTOOLBOX_PLUGIN)
#  define DUMMYTOOLBOXSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DUMMYTOOLBOXSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace DUMMYTOOLBOXPLUGIN {

//=============================================================================================================
/**
 * Returns build date and time.
 */
DUMMYTOOLBOXSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DUMMYTOOLBOXSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DUMMYTOOLBOXSHARED_EXPORT const char* buildHashLong();

}

#endif // DUMMYTOOLBOX_GLOBAL_H
