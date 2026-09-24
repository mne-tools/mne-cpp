//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffer_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Contains the Natus plugin library export/import macros.
 */

#ifndef FTBUFFER_GLOBAL_H
#define FTBUFFER_GLOBAL_H

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

#if defined(STATICBUILD)
#  define FTBUFFER_EXPORT
#elif defined(SCAN_FTBUFFER_PLUGIN)
#  define FTBUFFER_EXPORT Q_DECL_EXPORT
#else
#  define FTBUFFER_EXPORT Q_DECL_IMPORT
#endif

namespace FTBUFFERPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
FTBUFFER_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
FTBUFFER_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
FTBUFFER_EXPORT const char* buildHashLong();
}

#endif // FTBUFFER_GLOBAL_H
