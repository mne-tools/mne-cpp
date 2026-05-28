//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_discovery.h
 * @since March 2026
 * @brief Declares the resolve_streams / resolve_stream free functions used to enumerate LSL outlets currently visible on the LAN.
 *
 * Stream discovery is the entry point of every LSLLIB consumer: an
 * acquisition plug-in or analysis tool first calls
 * @ref LSLLIB::resolve_streams to obtain the set of @ref LSLLIB::stream_info
 * descriptors currently being advertised on the network, then
 * constructs a @ref LSLLIB::stream_inlet from the one it is
 * interested in. @ref LSLLIB::resolve_stream layers a property
 * filter on top so that callers who already know the @c name,
 * @c type, @c source_id, @c uid or @c hostname they expect can wait
 * only for that match.
 *
 * Both functions block for at most the supplied timeout while
 * listening for the periodic UDP multicast announcements emitted by
 * every active @ref LSLLIB::stream_outlet on the LSL discovery
 * channel. They deduplicate observed streams by their per-instance
 * UUID so that the same outlet is not returned twice when it
 * announces itself more than once during the listen window. The
 * default 1.0 s timeout matches the upstream liblsl default and is
 * long enough to pick up any outlet whose broadcast interval is at
 * most half a second.
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
// DEFINE NAMESPACE LSLLIB
//=============================================================================================================

namespace LSLLIB {

//=============================================================================================================
/**
 * @brief Resolve all streams currently available on the network.
 *
 * Listens on the UDP multicast discovery channel for stream announcements from outlets.
 * This call blocks for the specified timeout duration while collecting results.
 *
 * @param[in] timeout   Duration (in seconds) to listen for stream announcements. Default: 1.0.
 * @return              A vector of stream_info objects for all discovered streams.
 */
LSLSHARED_EXPORT std::vector<stream_info> resolve_streams(double timeout = 1.0);

//=============================================================================================================
/**
 * @brief Resolve streams by a specific property value.
 *
 * Listens on the UDP multicast discovery channel and returns only streams whose specified
 * property matches the given value.
 *
 * @param[in] prop      The stream property to match (e.g. "name", "type", "source_id").
 * @param[in] value     The value that the property must have.
 * @param[in] timeout   Duration (in seconds) to listen for stream announcements. Default: 1.0.
 * @return              A vector of matching stream_info objects.
 */
LSLSHARED_EXPORT std::vector<stream_info> resolve_stream(const std::string& prop,
                                                         const std::string& value,
                                                         double timeout = 1.0);

} // namespace LSLLIB

#endif // LSL_STREAM_DISCOVERY_H
