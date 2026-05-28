//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_global.cpp
 * @since 2026
 * @date  April 2026
 * @brief Definitions for the MNALIB build-info accessors declared in @ref mna_global.h.
 *
 * The two functions implemented here simply forward to the
 * @c utils/buildinfo.h helpers, which embed the build date/time
 * and abbreviated git hash at link time. Keeping the forwarders
 * out-of-line is what makes them callable across the
 * @c MNASHARED_EXPORT boundary without dragging @c utils headers
 * into every consumer translation unit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* MNALIB::buildDateTime(){ return UTILSLIB::dateTimeNow();}

//=============================================================================================================

const char* MNALIB::buildHash(){ return UTILSLIB::gitHash();}

//=============================================================================================================

const char* MNALIB::buildHashLong(){ return UTILSLIB::gitHashLong();}
