//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     events_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     March, 2021
 * @brief    event library export/import macros.
 */

#ifndef EVENTS_GLOBAL_H
#define EVENTS_GLOBAL_H

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

#if defined(STATICBUILD)
#  define EVENTS_EXPORT
#elif defined(MNE_EVENTS_LIBRARY)
#  define EVENTS_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define EVENTS_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace EVENTSLIB
 * @brief     Event annotation management (creation, grouping, shared-memory transport).
 */
namespace EVENTSLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
EVENTS_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
EVENTS_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
EVENTS_EXPORT const char* buildHashLong();
}

#endif // EVENTS_GLOBAL_H
