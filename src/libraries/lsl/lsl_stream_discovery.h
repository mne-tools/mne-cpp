//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_discovery.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Declares the resolve_streams / resolve_stream free functions used to enumerate LSL outlets currently visible on the LAN.
 *
 * Stream discovery is the entry point of every mne_lsl consumer: an
 * acquisition plug-in or analysis tool first calls
 * @ref mne::resolve_streams to obtain the set of @ref mne::stream_info
 * descriptors currently being advertised on the network, then
 * constructs a @ref mne::stream_inlet from the one it is
 * interested in. @ref mne::resolve_stream layers a property
 * filter on top so that callers who already know the @c name,
 * @c type, @c source_id, @c uid or @c hostname they expect can wait
 * only for that match.
 *
 * Both functions send liblsl's discovery query by UDP (multicast,
 * broadcast and localhost, repeated every half second) and collect
 * the answers of every outlet, from this library or from liblsl, for
 * at most the supplied timeout. They deduplicate streams by their
 * per-instance UUID.
 */

#ifndef LSL_STREAM_DISCOVERY_H
#define LSL_STREAM_DISCOVERY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"
#include "lsl_stream_info.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Resolve all streams currently available on the network.
 *
 * Queries the network for outlets; blocks for the specified timeout while collecting the answers.
 *
 * @param[in] timeout   Duration (in seconds) to collect answers. Default: 1.0.
 * @return              A vector of stream_info objects for all discovered streams.
 */
LSLSHARED_EXPORT std::vector<stream_info> resolve_streams(double timeout = 1.0);

//=============================================================================================================
/**
 * @brief Resolve streams by a specific property value.
 *
 * Queries the network with a predicate on the property and returns only streams whose
 * property matches the given value.
 *
 * @param[in] prop      The stream property to match (e.g. "name", "type", "source_id").
 * @param[in] value     The value that the property must have.
 * @param[in] timeout   Duration (in seconds) to collect answers. Default: 1.0.
 * @param[in] minimum   Return as soon as this many matching streams answered; 0 collects for the whole timeout.
 * @return              A vector of matching stream_info objects.
 */
LSLSHARED_EXPORT std::vector<stream_info> resolve_stream(const std::string& prop,
                                                         const std::string& value,
                                                         double timeout = 1.0,
                                                         int minimum = 0);

} // namespace mne

#endif // LSL_STREAM_DISCOVERY_H
