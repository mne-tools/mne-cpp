//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_graph.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    In-memory directed acyclic graph of @ref mne::MnaNode operations — connectivity, validation, topological sort and serialization.
 *
 * @ref mne::MnaGraph is the executable spine of an MNA project: a
 * collection of @ref mne::MnaNode operations wired together through
 * named @ref mne::MnaPort connections, surrounded by graph-level inputs
 * and outputs that expose the pipeline to its host application,
 * and parametrised by a shared @ref mne::MnaParamTree.
 *
 * The class owns four responsibilities. (1) @em Composition: add /
 * remove nodes, look them up by id, and @ref mne::MnaGraph::connect "connect" output ports
 * to input ports by name. (2) @em Validation: @ref mne::MnaGraph::validate "validate" checks
 * acyclicity, that every required input is connected, that
 * @ref mne::MnaDataKind "MnaDataKind" matches across each edge, and that every node
 * conforms to its @ref mne::MnaOpSchema. (3) @em Scheduling:
 * @c mne::MnaGraph::topological_sort "topological_sort", @c mne::MnaGraph::upstream_nodes "upstream_nodes", @c mne::MnaGraph::downstream_nodes "downstream_nodes"
 * and @c mne::MnaGraph::dirty_nodes "dirty_nodes" feed the executor's incremental re-run
 * logic. (4) @em Persistence: lossless JSON and CBOR round-trip
 * mirroring the @ref mne::MnaIO codec choices.
 */

#ifndef MNA_GRAPH_H
#define MNA_GRAPH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_node.h"
#include "mna_param_tree.h"
#include "mna_port.h"

#include <mne/base/cbor.h>
#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Directed acyclic graph of processing nodes forming a computational pipeline.
 *
 * @brief In-memory DAG of @ref MnaNode operations with validation, topological sort and JSON/CBOR persistence.
 *
 * @snippet ex_mna/main.cpp mna_graph_build
 */
class MNASHARED_EXPORT MnaGraph
{
public:
    /** @param[in] node   The node to append. */
    void add_node(MnaNode node);

    /** @param[in] node_id   Id of the node to remove; nothing happens if there is none. */
    void remove_node(std::string_view node_id);

    /** @param[in] node_id   A node id. @return The node, or nullptr if there is none. */
    MnaNode* find_node(std::string_view node_id);

    /** @param[in] node_id   A node id. @return The node, or nullptr if there is none. */
    const MnaNode* find_node(std::string_view node_id) const;

    /** @param[in] node_id   Id of a node of this graph. @return The node; throws std::out_of_range if there is none. */
    MnaNode& node(std::string_view node_id);

    /** @param[in] node_id   Id of a node of this graph. @return The node; throws std::out_of_range if there is none. */
    const MnaNode& node(std::string_view node_id) const;

    /** @return The nodes in insertion order. */
    std::vector<MnaNode>& nodes();

    /** @return The nodes in insertion order. */
    const std::vector<MnaNode>& nodes() const;

    /** @param[in] node_id   A node id. @return Whether the graph has the node. */
    bool has_node(std::string_view node_id) const;

    std::vector<MnaPort> graph_inputs;  ///< Named, typed entry points
    std::vector<MnaPort> graph_outputs; ///< Named, typed exit points

    MnaParamTree param_tree; ///< Hierarchical parameter store with formula-driven bindings

    //=========================================================================================================
    /**
     * Connects an output port of one node to an input port of another by setting the source of the input port.
     *
     * @param[in] src_node_id     Id of the node providing the output.
     * @param[in] src_port_name   Name of the output port on the source node.
     * @param[in] dst_node_id     Id of the node receiving the input.
     * @param[in] dst_port_name   Name of the input port on the destination node.
     *
     * @return Whether both ports exist and were connected.
     */
    bool connect(std::string_view src_node_id, std::string_view src_port_name, std::string_view dst_node_id, std::string_view dst_port_name);

    //=========================================================================================================
    /**
     * Checks acyclicity, port connections, data-kind compatibility, op schema compliance and required attributes.
     *
     * @return The problems found; empty if the graph is valid.
     */
    StringList validate() const;

    /** @return The node ids in an order where every node follows the nodes it reads from; ready nodes by id. */
    StringList topological_sort() const;

    /** @param[in] node_id   A node id. @return The nodes it reads from, directly or indirectly, nearest first. */
    StringList upstream_nodes(std::string_view node_id) const;

    /** @param[in] node_id   A node id. @return The nodes that read from it, directly or indirectly, nearest first. */
    StringList downstream_nodes(std::string_view node_id) const;

    /** @return The ids of the nodes that need re-execution. */
    StringList dirty_nodes() const;

    /** @return The graph as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A graph as written by to_json(). @return The graph. */
    static MnaGraph from_json(const JsonObject& json);

    /** @return The graph as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A graph as written by to_cbor(). @return The graph. */
    static MnaGraph from_cbor(const CborMap& cbor);

private:
    std::vector<MnaNode> m_nodes;
};

} // namespace mne

#endif // MNA_GRAPH_H
