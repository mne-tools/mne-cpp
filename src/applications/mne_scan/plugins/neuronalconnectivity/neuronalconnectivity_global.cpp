//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivity_global.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 * @brief    neuronalconnectivity plugin global definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* NEURONALCONNECTIVITYPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* NEURONALCONNECTIVITYPLUGIN::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* NEURONALCONNECTIVITYPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();}
