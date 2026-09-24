//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivity_global.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the NeuronalConnectivity library export/import macros.
 */

#ifndef NEURONALCONNECTIVITY_GLOBAL_H
#define NEURONALCONNECTIVITY_GLOBAL_H

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

#if defined(SCAN_NEURONALCONNECTIVITY_PLUGIN)
#  define NEURONALCONNECTIVITYSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define NEURONALCONNECTIVITYSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace NEURONALCONNECTIVITYPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
NEURONALCONNECTIVITYSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
NEURONALCONNECTIVITYSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
NEURONALCONNECTIVITYSHARED_EXPORT const char* buildHashLong();
}

#endif // CONNECTIVITY_GLOBAL_H
