//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl.h
 * @since February 2026
 * @brief Umbrella include header that exposes the full LSLLIB public surface in a single import.
 *
 * LSLLIB is mne-cpp's in-tree, Qt-based reimplementation of the Lab
 * Streaming Layer (LSL) client API. It is designed as an ABI- and
 * source-compatible drop-in replacement for the public subset of liblsl's
 * @c lsl_cpp.h header, so acquisition plug-ins and analysis tools can
 * talk to any LSL-speaking device on the network without pulling the
 * external @c liblsl dependency into the build.
 *
 * Including this header transitively pulls in the four public components
 * that together cover the round-trip data path: @ref LSLLIB::stream_info
 * for describing a stream's content type, channel layout and sampling
 * rate; @ref LSLLIB::stream_outlet for publishing samples over TCP while
 * advertising the stream via UDP multicast; @ref LSLLIB::stream_inlet for
 * connecting to a resolved outlet and pulling chunks of multichannel
 * float samples; and the @c resolve_streams / @c resolve_stream free
 * functions in @ref lsl_stream_discovery.h for enumerating outlets
 * currently visible on the LAN. Downstream code should depend on this
 * single header rather than the individual component headers so that the
 * public surface remains coherent as the library evolves.
 */

#ifndef LSL_H
#define LSL_H

//=============================================================================================================
// LSL LIBRARY HEADERS
//=============================================================================================================

#include "lsl_global.h"
#include "lsl_stream_info.h"
#include "lsl_stream_outlet.h"
#include "lsl_stream_inlet.h"
#include "lsl_stream_discovery.h"

#endif // LSL_H
