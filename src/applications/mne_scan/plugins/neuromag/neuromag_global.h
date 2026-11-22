//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromag_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    Contains the Neuromag library export/import macros.
 */

#ifndef NEUROMAG_GLOBAL_H
#define NEUROMAG_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/base_global.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_NEUROMAG_PLUGIN)
#define NEUROMAGSHARED_EXPORT Q_DECL_EXPORT /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#define NEUROMAGSHARED_EXPORT Q_DECL_IMPORT /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace NEUROMAGPLUGIN
{

//=============================================================================================================
/**
 * Returns build date and time.
 */
NEUROMAGSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
NEUROMAGSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
NEUROMAGSHARED_EXPORT const char* buildHashLong();
}

#endif // NEUROMAG_GLOBAL_H
