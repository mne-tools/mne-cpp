//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_port.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Typed input or output slot on a graph node — carries an @ref mne::MnaDataKind "MnaDataKind", an upstream link, an optional real-time stream binding and a cached-result reference.
 *
 * @ref mne::MnaPort is the unit of connectivity in an MNA graph: every
 * data hand-off from one @ref mne::MnaNode to another goes through a
 * named, typed port pair, never an anonymous string. Input ports
 * carry an explicit @c source_node_id / @c source_port_name pointing at
 * the producing output, which is what @c mne::MnaGraph::topological_sort "MnaGraph::topological_sort"
 * and @ref mne::MnaGraph::validate "MnaGraph::validate" rely on to detect cycles and reject
 * mismatched @ref mne::MnaDataKind "MnaDataKind" connections at edit time.
 *
 * When a port participates in a streaming pipeline (e.g. MNE Scan),
 * @c stream_protocol selects the transport — @c fiff-rt, @c lsl,
 * @c ftbuffer, @c shm or the in-process default — and
 * @c stream_endpoint / @c stream_buffer_ms configure the live
 * connection. For batch outputs, @c cached_result_path plus
 * @c cached_result_hash let the executor skip re-computation when
 * upstream inputs are unchanged. @c extras keeps any forward-
 * compatible attributes a newer registry may attach.
 */

#ifndef MNA_PORT_H
#define MNA_PORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"

#include <base/cbor.h>
#include <base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Typed input/output slot on a graph node.
 *
 * @brief Named, typed port on an MNA graph node with upstream link and optional real-time stream binding.
 *
 * @snippet ex_mna/main.cpp mna_graph_build
 */
struct MNASHARED_EXPORT MnaPort
{
    std::string name;                            ///< Port name (unique within a node)
    MnaDataKind data_kind = MnaDataKind::Custom; ///< Data kind flowing through this port
    MnaPortDir direction = MnaPortDir::Input;    ///< Input or Output

    std::string source_node_id;   ///< Which node produces this input? (empty → graph-level input)
    std::string source_port_name; ///< Which output port on that node?

    std::string stream_protocol; ///< "fiff-rt", "lsl", "ftbuffer", "shm", "" = internal signal/slot
    std::string stream_endpoint; ///< Protocol-specific address (e.g. "localhost:4218" for fiff-rt)
    int stream_buffer_ms = 0;    ///< Ring-buffer length in ms (0 = unbounded)

    std::string cached_result_path; ///< Relative path to cached result
    std::string cached_result_hash; ///< SHA-256 for invalidation

    JsonObject extras; ///< Unknown keys preserved for lossless round-trip

    /** @return The port as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A port as written by to_json(). @return The port. */
    static MnaPort from_json(const JsonObject& json);

    /** @return The port as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A port as written by to_cbor(). @return The port. */
    static MnaPort from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_PORT_H
