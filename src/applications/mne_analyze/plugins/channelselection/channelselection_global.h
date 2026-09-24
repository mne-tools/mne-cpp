//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     channelselection_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     July, 2020
 * @brief    Contains the Channel Selection export/import macros.
 */

#ifndef CHANNELSELECTION_GLOBAL_H
#define CHANNELSELECTION_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(ANALYZE_CHANNELSELECTION_PLUGIN)
#  define CHANNELSELECTIONSHARED_EXPORT Q_DECL_EXPORT   /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define CHANNELSELECTIONSHARED_EXPORT Q_DECL_IMPORT   /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace CHANNELSELECTIONPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
CHANNELSELECTIONSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
CHANNELSELECTIONSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
CHANNELSELECTIONSHARED_EXPORT const char* buildHashLong();
}

#endif // DATAMANAGER_GLOBAL_H
