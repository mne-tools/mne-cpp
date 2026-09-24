//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefit_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     October, 2020
 * @brief    Contains the InvDipoleFit library export/import macros.
 */

#ifndef DIPOLEFIT_GLOBAL_H
#define DIPOLEFIT_GLOBAL_H

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

#if defined(ANALYZE_DIPOLEFIT_PLUGIN)
#  define DIPOLEFITSHARED_EXPORT Q_DECL_EXPORT   /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DIPOLEFITSHARED_EXPORT Q_DECL_IMPORT   /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace DIPOLEFITPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
DIPOLEFITSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DIPOLEFITSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DIPOLEFITSHARED_EXPORT const char* buildHashLong();
}

#endif // DIPOLEFIT_GLOBAL_H
