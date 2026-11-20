//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     scshared_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2013
 * @brief    Contains the scshared library export/import macros.
 */
#ifndef SCSHARED_GLOBAL_H
#define SCSHARED_GLOBAL_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#define SCSHAREDSHARED_EXPORT
#elif defined(SCSHARED_LIBRARY)
#define SCSHAREDSHARED_EXPORT Q_DECL_EXPORT /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#define SCSHAREDSHARED_EXPORT Q_DECL_IMPORT /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

// The connector templates are instantiated in every plugin and cast across them (dynamic_cast), so their type
// information must be one per process also when a plugin hides its symbols. MSVC compares type names instead.
#if defined(_MSC_VER) || defined(STATICBUILD)
#define SCSHARED_TEMPLATE_VISIBLE
#else
#define SCSHARED_TEMPLATE_VISIBLE __attribute__((visibility("default")))
#endif

#endif // SCSHARED_GLOBAL_H
