//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_graph.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MnaGraph: node management, port connectivity, schema-aware validation, Kahn topological sort and JSON/CBOR round-trip.
 *
 * The implementation backs the graph with a single @c std::vector of
 * @ref MnaNode and resolves node ids through linear search, which
 * stays cheap at the pipeline sizes MNA targets (tens to hundreds
 * of nodes). @ref MnaGraph::validate walks the node list, queries
 * @ref MnaOpRegistry for each @c op_type, runs the per-node
 * @ref MnaOpSchema::validate, then sweeps the inter-node edges to
 * check that every required input port is wired and that the
 * @ref MnaDataKind on both ends agrees.
 *
 * @ref MnaGraph::topological_sort implements Kahn's algorithm over
 * the input-port @c source_node_id references and surfaces an empty
 * list when a cycle is present, which the validator turns into an
 * explicit error. @ref upstream_nodes / @ref downstream_nodes use a
 * @c QQueue-driven BFS to support incremental re-execution, and
 * the JSON / CBOR codecs delegate every nested struct to its own
 * codec so a project survives a round trip without loss.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph.h"

#include "mna_codec.h"
#include "mna_op_registry.h"
#include "mna_op_schema.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <deque>
#include <format>
#include <map>
#include <set>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** Edges from each node to the nodes that read from it, ordered by id; edges to unknown nodes are left out. */
using Adjacency = std::map<std::string, std::set<std::string>, std::less<>>;

Adjacency forwardEdges(const std::vector<MnaNode>& nodes)
{
    Adjacency adj;
    for (const MnaNode& n : nodes) {
        adj[n.id];
    }
    for (const MnaNode& n : nodes) {
        for (const MnaPort& p : n.inputs) {
            if (const auto it = adj.find(p.source_node_id); !p.source_node_id.empty() && it != adj.end()) {
                it->second.insert(n.id);
            }
        }
    }
    return adj;
}

/** Kahn's algorithm; the result is shorter than the node count if the graph has a cycle. */
StringList kahn(const Adjacency& adj)
{
    std::map<std::string, int, std::less<>> inDegree;
    for (const auto& [id, next] : adj) {
        inDegree[id];
        for (const std::string& n : next) {
            ++inDegree[n];
        }
    }
    std::deque<std::string> queue;
    for (const auto& [id, degree] : inDegree) {
        if (degree == 0) {
            queue.push_back(id);
        }
    }
    StringList sorted;
    while (!queue.empty()) {
        std::string current = std::move(queue.front());
        queue.pop_front();
        for (const std::string& n : adj.find(current)->second) {
            if (--inDegree[n] == 0) {
                queue.push_back(n);
            }
        }
        sorted.push_back(std::move(current));
    }
    return sorted;
}

/** Breadth-first walk from the neighbours of @p start. */
template<class Neighbours>
StringList walk(std::string_view start, Neighbours neighbours)
{
    StringList order;
    std::set<std::string, std::less<>> visited;
    std::deque<std::string> queue;
    const auto visit = [&](const std::string& id) {
        if (!id.empty() && visited.insert(id).second) {
            queue.push_back(id);
        }
    };
    neighbours(start, visit);
    while (!queue.empty()) {
        std::string current = std::move(queue.front());
        queue.pop_front();
        neighbours(current, visit);
        order.push_back(std::move(current));
    }
    return order;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MnaGraph::add_node(MnaNode node)
{
    m_nodes.push_back(std::move(node));
}

//=============================================================================================================

void MnaGraph::remove_node(std::string_view node_id)
{
    if (const auto it = std::ranges::find(m_nodes, node_id, &MnaNode::id); it != m_nodes.end()) {
        m_nodes.erase(it);
    }
}

//=============================================================================================================

MnaNode* MnaGraph::find_node(std::string_view node_id)
{
    const auto it = std::ranges::find(m_nodes, node_id, &MnaNode::id);
    return it != m_nodes.end() ? &*it : nullptr;
}

//=============================================================================================================

const MnaNode* MnaGraph::find_node(std::string_view node_id) const
{
    const auto it = std::ranges::find(m_nodes, node_id, &MnaNode::id);
    return it != m_nodes.end() ? &*it : nullptr;
}

//=============================================================================================================

MnaNode& MnaGraph::node(std::string_view node_id)
{
    if (MnaNode* n = find_node(node_id)) {
        return *n;
    }
    throw std::out_of_range(std::format("MNA graph has no node '{}'", node_id));
}

//=============================================================================================================

const MnaNode& MnaGraph::node(std::string_view node_id) const
{
    if (const MnaNode* n = find_node(node_id)) {
        return *n;
    }
    throw std::out_of_range(std::format("MNA graph has no node '{}'", node_id));
}

//=============================================================================================================

std::vector<MnaNode>& MnaGraph::nodes()
{
    return m_nodes;
}

//=============================================================================================================

const std::vector<MnaNode>& MnaGraph::nodes() const
{
    return m_nodes;
}

//=============================================================================================================

bool MnaGraph::has_node(std::string_view node_id) const
{
    return find_node(node_id) != nullptr;
}

//=============================================================================================================

bool MnaGraph::connect(std::string_view src_node_id, std::string_view src_port_name, std::string_view dst_node_id, std::string_view dst_port_name)
{
    const MnaNode* src = find_node(src_node_id);
    MnaNode* dst = find_node(dst_node_id);
    if (!src || !dst || std::ranges::find(src->outputs, src_port_name, &MnaPort::name) == src->outputs.end()) {
        return false;
    }
    const auto port = std::ranges::find(dst->inputs, dst_port_name, &MnaPort::name);
    if (port == dst->inputs.end()) {
        return false;
    }
    port->source_node_id = src_node_id;
    port->source_port_name = src_port_name;
    return true;
}

//=============================================================================================================

StringList MnaGraph::validate() const
{
    StringList errors;

    for (const MnaNode& n : m_nodes) {
        for (const MnaPort& p : n.inputs) {
            if (!p.source_node_id.empty() && !has_node(p.source_node_id)) {
                errors.push_back(std::format("Node '{}' input port '{}' references unknown source node '{}'", n.id, p.name, p.source_node_id));
            }
        }
    }
    if (kahn(forwardEdges(m_nodes)).size() != m_nodes.size()) {
        errors.emplace_back("Graph contains a cycle");
    }

    const MnaOpRegistry& registry = MnaOpRegistry::instance();
    for (const MnaNode& n : m_nodes) {
        if (!registry.has_op(n.op_type)) {
            errors.push_back(std::format("Node '{}' has unregistered op type '{}'", n.id, n.op_type));
            continue;
        }
        const MnaOpSchema& schema = registry.schema(n.op_type);
        for (const std::string& e : schema.validate(n)) {
            errors.push_back(std::format("Node '{}': {}", n.id, e));
        }
        for (const MnaOpSchemaPort& sp : schema.input_ports) {
            if (!sp.required) {
                continue;
            }
            for (const MnaPort& np : n.inputs) {
                if (np.name == sp.name && np.source_node_id.empty()) {
                    errors.push_back(std::format("Node '{}': required input port '{}' is not connected", n.id, sp.name));
                }
            }
        }
    }

    for (const MnaNode& n : m_nodes) {
        for (const MnaPort& in : n.inputs) {
            const MnaNode* src = in.source_port_name.empty() ? nullptr : find_node(in.source_node_id);
            if (!src) {
                continue;
            }
            for (const MnaPort& out : src->outputs) {
                if (out.name == in.source_port_name && in.data_kind != MnaDataKind::Custom && out.data_kind != MnaDataKind::Custom &&
                    in.data_kind != out.data_kind) {
                    errors.push_back(std::format("Edge {}.{} -> {}.{}: data kind mismatch ({} != {})", src->id, out.name, n.id, in.name,
                                                 static_cast<int>(out.data_kind), static_cast<int>(in.data_kind)));
                }
            }
        }
    }
    return errors;
}

//=============================================================================================================

StringList MnaGraph::topological_sort() const
{
    return kahn(forwardEdges(m_nodes));
}

//=============================================================================================================

StringList MnaGraph::upstream_nodes(std::string_view node_id) const
{
    return walk(node_id, [this](std::string_view id, const auto& visit) {
        if (const MnaNode* n = find_node(id)) {
            for (const MnaPort& p : n->inputs) {
                visit(p.source_node_id);
            }
        }
    });
}

//=============================================================================================================

StringList MnaGraph::downstream_nodes(std::string_view node_id) const
{
    const Adjacency adj = forwardEdges(m_nodes);
    return walk(node_id, [&adj](std::string_view id, const auto& visit) {
        if (const auto it = adj.find(id); it != adj.end()) {
            for (const std::string& next : it->second) {
                visit(next);
            }
        }
    });
}

//=============================================================================================================

StringList MnaGraph::dirty_nodes() const
{
    StringList dirty;
    for (const MnaNode& n : m_nodes) {
        if (n.dirty) {
            dirty.push_back(n.id);
        }
    }
    return dirty;
}

//=============================================================================================================

JsonObject MnaGraph::to_json() const
{
    JsonObject json;
    json["nodes"] = detail::to_json_array(m_nodes);
    if (!graph_inputs.empty()) {
        json["graph_inputs"] = detail::to_json_array(graph_inputs);
    }
    if (!graph_outputs.empty()) {
        json["graph_outputs"] = detail::to_json_array(graph_outputs);
    }
    if (JsonObject tree = param_tree.to_json(); !tree.empty()) {
        json["param_tree"] = std::move(tree);
    }
    return json;
}

//=============================================================================================================

MnaGraph MnaGraph::from_json(const JsonObject& json)
{
    MnaGraph graph;
    graph.m_nodes = detail::from_json_array<MnaNode>(json["nodes"]);
    graph.graph_inputs = detail::from_json_array<MnaPort>(json["graph_inputs"]);
    graph.graph_outputs = detail::from_json_array<MnaPort>(json["graph_outputs"]);
    if (json.contains("param_tree")) {
        graph.param_tree = MnaParamTree::from_json(json["param_tree"].to_object());
    }
    return graph;
}

//=============================================================================================================

CborMap MnaGraph::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaGraph MnaGraph::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
