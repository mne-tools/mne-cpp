//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_node.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    One operation in an MNA graph — op_type, typed I/O ports, attributes, execution mode (Batch / Stream / IPC / Script) and provenance.
 *
 * @ref mne::MnaNode is the executable counterpart to a @ref mne::MnaFileRef "MnaFileRef":
 * where the latter records a derived artefact, the node records
 * @em how that artefact is produced. The @c op_type string is
 * resolved against @ref mne::MnaOpRegistry to obtain the
 * @ref mne::MnaOpSchema (and, for built-in ops, the implementation
 * function) used by @ref mne::MnaGraphExecutor; @c attributes provides
 * the per-call parameter values validated against that schema.
 *
 * The @c exec_mode field selects one of four executors:
 * @c Batch (default, file-in/file-out), @c Stream (live MNE Scan
 * plugin), @c Ipc (delegates to an external binary via
 * @c ipc_command / @c ipc_args / @c ipc_transport), or @c Script
 * (inline @ref mne::MnaScript run by an interpreter). Each mode reuses
 * the same node fields, so a pipeline can mix all four without
 * structural changes.
 *
 * @c verification carries pre/post checks, explanation text and the
 * full @ref mne::MnaProvenance snapshot captured at execution time;
 * combined with @c tool_version, @c executed_at and the @c dirty flag
 * this is what makes MNA pipelines reproducible and supports
 * incremental re-execution after a parameter edit.
 */

#ifndef MNA_NODE_H
#define MNA_NODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_port.h"
#include "mna_script.h"
#include "mna_types.h"
#include "mna_verification.h"

#include <mne/base/cbor.h>
#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * One operation in the computational graph.
 *
 * @brief Single executable step in an MNA pipeline graph, with attributes, typed ports, exec mode, verification and provenance.
 *
 * @snippet ex_mna/main.cpp mna_graph_build
 */
struct MNASHARED_EXPORT MnaNode
{
    std::string id;        ///< Unique node identifier
    std::string op_type;   ///< Operation type (looked up in MnaOpRegistry)
    JsonObject attributes; ///< Operation parameters

    std::vector<MnaPort> inputs;  ///< Input ports
    std::vector<MnaPort> outputs; ///< Output ports

    MnaNodeExecMode exec_mode = MnaNodeExecMode::Batch; ///< Execution mode

    std::string ipc_command;   ///< External executable command (exec_mode == Ipc)
    StringList ipc_args;       ///< Command-line arguments (supports {{placeholder}} tokens)
    std::string ipc_work_dir;  ///< Working directory for external process
    std::string ipc_transport; ///< "stdio", "tcp", "shm", "file"

    MnaScript script; ///< Inline source code, interpreter, language (exec_mode == Script)

    MnaVerification verification; ///< Explanation, checks, results, and provenance snapshot

    std::string tool_version;           ///< Version of tool that last executed this node
    std::optional<UtcTime> executed_at; ///< Timestamp of last execution
    bool dirty = true;                  ///< Whether node needs re-execution
    JsonObject extras;                  ///< Unknown keys preserved for lossless round-trip

    /** @return The node as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A node as written by to_json(). @return The node. */
    static MnaNode from_json(const JsonObject& json);

    /** @return The node as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A node as written by to_cbor(). @return The node. */
    static MnaNode from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_NODE_H
