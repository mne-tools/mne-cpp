//=============================================================================================================
/**
 * @file     mna_scan_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Mapping functions between ConnectorDataType and MnaDataKind.
 *
 */

#ifndef MNA_SCAN_TYPES_H
#define MNA_SCAN_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"
#include "pluginconnectorconnection.h"

#include <mna/mna_types.h>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================

using MNALIB::MnaDataKind;

//=============================================================================================================
/**
 * Convert ConnectorDataType to MnaDataKind.
 *
 * @param[in] t     The ConnectorDataType value.
 * @return The corresponding MnaDataKind value.
 */
inline MnaDataKind connectorDataTypeToMnaDataKind(ConnectorDataType t)
{
    switch(t) {
    case _RTMSA: return MnaDataKind::FiffRaw;
    case _RTES:  return MnaDataKind::Evoked;
    case _RTC:   return MnaDataKind::Covariance;
    case _RTSE:  return MnaDataKind::SourceEstimate;
    case _RTHR:  return MnaDataKind::Custom;          // "hpi"
    case _RTFS:  return MnaDataKind::Forward;
    default:     return MnaDataKind::Custom;
    }
}

//=============================================================================================================
/**
 * Convert MnaDataKind to ConnectorDataType.
 *
 * Note: MnaDataKind::Custom is ambiguous — the caller must disambiguate.
 *
 * @param[in] k     The MnaDataKind value.
 * @return The corresponding ConnectorDataType value.
 */
inline ConnectorDataType mnaDataKindToConnectorDataType(MnaDataKind k)
{
    switch(k) {
    case MnaDataKind::FiffRaw:        return _RTMSA;
    case MnaDataKind::Evoked:         return _RTES;
    case MnaDataKind::Covariance:     return _RTC;
    case MnaDataKind::SourceEstimate: return _RTSE;
    case MnaDataKind::Forward:        return _RTFS;
    case MnaDataKind::Custom:         return _N;       // ambiguous — caller must disambiguate
    default:                          return _N;
    }
}

} // namespace SCSHAREDLIB

#endif // MNA_SCAN_TYPES_H
