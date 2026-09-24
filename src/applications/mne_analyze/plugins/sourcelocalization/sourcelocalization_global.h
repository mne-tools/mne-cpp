//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     sourcelocalization_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.6
 * @date     August, 2020
 * @brief    MNE Analyze Source Localization Plugin
 */

#ifndef MNEANALYZE_SOURCELOCALIZATION_GLOBAL_H
#define MNEANALYZE_SOURCELOCALIZATION_GLOBAL_H

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

#if defined(ANALYZE_SOURCELOCALIZATION_PLUGIN)
#  define SOURCELOCALIZATIONSHARED_EXPORT Q_DECL_EXPORT   /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define SOURCELOCALIZATIONSHARED_EXPORT Q_DECL_IMPORT   /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace SOURCELOCALIZATIONPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
SOURCELOCALIZATIONSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
SOURCELOCALIZATIONSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
SOURCELOCALIZATIONSHARED_EXPORT const char* buildHashLong();
}

#endif // MNEANALYZE_SOURCELOCALIZATION_GLOBAL_H
