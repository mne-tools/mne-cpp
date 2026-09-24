//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_scan_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Mapping functions between ConnectorDataType and MnaDataKind.
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
