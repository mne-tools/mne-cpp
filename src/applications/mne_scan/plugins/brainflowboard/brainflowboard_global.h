//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     brainflowboard_global.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the BrainFlow plugin library export/import macros.
 */

#ifndef BRAINFLOWBOARD_GLOBAL_H
#define BRAINFLOWBOARD_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/base_global.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_BRAINFLOWBOARD_PLUGIN)
#define BRAINFLOWBOARD_EXPORT Q_DECL_EXPORT /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#define BRAINFLOWBOARD_EXPORT Q_DECL_IMPORT /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace BRAINFLOWBOARDPLUGIN
{

//=============================================================================================================
/**
 * Returns build date and time.
 */
BRAINFLOWBOARD_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
BRAINFLOWBOARD_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
BRAINFLOWBOARD_EXPORT const char* buildHashLong();
}

#endif // BRAINFLOWBOARD_GLOBAL_H
