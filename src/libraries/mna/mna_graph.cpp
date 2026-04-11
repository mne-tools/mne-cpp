//=============================================================================================================
/**
 * @file     mna_graph.cpp
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
 * @brief    MnaGraph class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph.h"
#include "mna_op_registry.h"

#include <QJsonArray>
#include <QCborArray>
#include <QSet>
#include <QQueue>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaGraph::MnaGraph()
{
}

//=============================================================================================================
// Node management
//=============================================================================================================

void MnaGraph::addNode(const MnaNode& node)
{
    m_nodes.append(node);
}

//=============================================================================================================

void MnaGraph::removeNode(const QString& nodeId)
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == nodeId) {
            m_nodes.removeAt(i);
            return;
        }
    }
}

//=============================================================================================================

MnaNode& MnaGraph::node(const QString& nodeId)
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == nodeId) {
            return m_nodes[i];
        }
    }
    static MnaNode dummy;
    return dummy;
}

//=============================================================================================================

const MnaNode& MnaGraph::node(const QString& nodeId) const
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == nodeId) {
            return m_nodes[i];
        }
    }
    static const MnaNode dummy;
    return dummy;
}

//=============================================================================================================

QList<MnaNode>& MnaGraph::nodes()
{
    return m_nodes;
}

//=============================================================================================================

const QList<MnaNode>& MnaGraph::nodes() const
{
    return m_nodes;
}

//=============================================================================================================

bool MnaGraph::hasNode(const QString& nodeId) const
{
    for (const MnaNode& n : m_nodes) {
        if (n.id == nodeId) {
            return true;
        }
    }
    return false;
}

//=============================================================================================================
// Connection helper
//=============================================================================================================

bool MnaGraph::connect(const QString& srcNodeId, const QString& srcPortName,
                        const QString& dstNodeId, const QString& dstPortName)
{
    // Verify source node and port exist
    if (!hasNode(srcNodeId) || !hasNode(dstNodeId)) {
        return false;
    }

    const MnaNode& srcNode = node(srcNodeId);
    bool srcFound = false;
    for (const MnaPort& p : srcNode.outputs) {
        if (p.name == srcPortName) {
            srcFound = true;
            break;
        }
    }
    if (!srcFound) {
        return false;
    }

    // Find and update the destination input port
    MnaNode& dstNode = node(dstNodeId);
    for (int i = 0; i < dstNode.inputs.size(); ++i) {
        if (dstNode.inputs[i].name == dstPortName) {
            dstNode.inputs[i].sourceNodeId   = srcNodeId;
            dstNode.inputs[i].sourcePortName = srcPortName;
            return true;
        }
    }

    return false;
}

//=============================================================================================================
// Validation
//=============================================================================================================

bool MnaGraph::validate(QStringList* errors) const
{
    bool valid = true;
    auto addError = [&](const QString& msg) {
        valid = false;
        if (errors) {
            errors->append(msg);
        }
    };

    // Build adjacency for cycle detection
    QMap<QString, QSet<QString>> adj;
    QMap<QString, int> inDegree;
    for (const MnaNode& n : m_nodes) {
        if (!adj.contains(n.id)) {
            adj[n.id] = {};
        }
        if (!inDegree.contains(n.id)) {
            inDegree[n.id] = 0;
        }
    }

    // Populate adjacency from input port connections
    for (const MnaNode& n : m_nodes) {
        for (const MnaPort& p : n.inputs) {
            if (!p.sourceNodeId.isEmpty()) {
                if (!adj.contains(p.sourceNodeId)) {
                    addError(QStringLiteral("Node '%1' input port '%2' references unknown source node '%3'")
                             .arg(n.id, p.name, p.sourceNodeId));
                } else {
                    if (!adj[p.sourceNodeId].contains(n.id)) {
                        adj[p.sourceNodeId].insert(n.id);
                        inDegree[n.id]++;
                    }
                }
            }
        }
    }

    // Check acyclicity via Kahn's algorithm
    QQueue<QString> queue;
    for (auto it = inDegree.constBegin(); it != inDegree.constEnd(); ++it) {
        if (it.value() == 0) {
            queue.enqueue(it.key());
        }
    }

    int visited = 0;
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        visited++;
        for (const QString& neighbor : adj.value(current)) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.enqueue(neighbor);
            }
        }
    }

    if (visited != m_nodes.size()) {
        addError(QStringLiteral("Graph contains a cycle"));
    }

    // Validate each node against its schema
    const MnaOpRegistry& registry = MnaOpRegistry::instance();
    for (const MnaNode& n : m_nodes) {
        if (!registry.hasOp(n.opType)) {
            addError(QStringLiteral("Node '%1' has unregistered op type '%2'")
                     .arg(n.id, n.opType));
            continue;
        }

        MnaOpSchema schema = registry.schema(n.opType);
        QStringList schemaErrors;
        if (!schema.validate(n, &schemaErrors)) {
            for (const QString& e : schemaErrors) {
                addError(QStringLiteral("Node '%1': %2").arg(n.id, e));
            }
        }
    }

    return valid;
}

//=============================================================================================================
// Topological sort
//=============================================================================================================

QStringList MnaGraph::topologicalSort() const
{
    // Kahn's algorithm
    QMap<QString, QSet<QString>> adj;
    QMap<QString, int> inDegree;

    for (const MnaNode& n : m_nodes) {
        adj[n.id] = {};
        inDegree[n.id] = 0;
    }

    for (const MnaNode& n : m_nodes) {
        for (const MnaPort& p : n.inputs) {
            if (!p.sourceNodeId.isEmpty() && adj.contains(p.sourceNodeId)) {
                if (!adj[p.sourceNodeId].contains(n.id)) {
                    adj[p.sourceNodeId].insert(n.id);
                    inDegree[n.id]++;
                }
            }
        }
    }

    QQueue<QString> queue;
    for (auto it = inDegree.constBegin(); it != inDegree.constEnd(); ++it) {
        if (it.value() == 0) {
            queue.enqueue(it.key());
        }
    }

    QStringList sorted;
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        sorted.append(current);
        for (const QString& neighbor : adj.value(current)) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.enqueue(neighbor);
            }
        }
    }

    return sorted;
}

//=============================================================================================================
// Dependency queries
//=============================================================================================================

QStringList MnaGraph::upstreamNodes(const QString& nodeId) const
{
    QStringList upstream;
    if (!hasNode(nodeId)) {
        return upstream;
    }

    const MnaNode& n = node(nodeId);
    QSet<QString> visited;
    QQueue<QString> queue;

    for (const MnaPort& p : n.inputs) {
        if (!p.sourceNodeId.isEmpty() && !visited.contains(p.sourceNodeId)) {
            visited.insert(p.sourceNodeId);
            queue.enqueue(p.sourceNodeId);
        }
    }

    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        upstream.append(current);
        if (hasNode(current)) {
            const MnaNode& cn = node(current);
            for (const MnaPort& p : cn.inputs) {
                if (!p.sourceNodeId.isEmpty() && !visited.contains(p.sourceNodeId)) {
                    visited.insert(p.sourceNodeId);
                    queue.enqueue(p.sourceNodeId);
                }
            }
        }
    }

    return upstream;
}

//=============================================================================================================

QStringList MnaGraph::downstreamNodes(const QString& nodeId) const
{
    QStringList downstream;
    if (!hasNode(nodeId)) {
        return downstream;
    }

    // Build forward adjacency
    QMap<QString, QSet<QString>> adj;
    for (const MnaNode& n : m_nodes) {
        adj[n.id] = {};
    }
    for (const MnaNode& n : m_nodes) {
        for (const MnaPort& p : n.inputs) {
            if (!p.sourceNodeId.isEmpty() && adj.contains(p.sourceNodeId)) {
                adj[p.sourceNodeId].insert(n.id);
            }
        }
    }

    QSet<QString> visited;
    QQueue<QString> queue;
    for (const QString& neighbor : adj.value(nodeId)) {
        if (!visited.contains(neighbor)) {
            visited.insert(neighbor);
            queue.enqueue(neighbor);
        }
    }

    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        downstream.append(current);
        for (const QString& neighbor : adj.value(current)) {
            if (!visited.contains(neighbor)) {
                visited.insert(neighbor);
                queue.enqueue(neighbor);
            }
        }
    }

    return downstream;
}

//=============================================================================================================

QStringList MnaGraph::dirtyNodes() const
{
    QStringList dirty;
    for (const MnaNode& n : m_nodes) {
        if (n.dirty) {
            dirty.append(n.id);
        }
    }
    return dirty;
}

//=============================================================================================================
// Serialization
//=============================================================================================================

QJsonObject MnaGraph::toJson() const
{
    QJsonObject json;

    // Nodes
    QJsonArray nodesArr;
    for (const MnaNode& n : m_nodes) {
        nodesArr.append(n.toJson());
    }
    json[QStringLiteral("nodes")] = nodesArr;

    // Graph-level inputs
    if (!graphInputs.isEmpty()) {
        QJsonArray arr;
        for (const MnaPort& p : graphInputs) {
            arr.append(p.toJson());
        }
        json[QStringLiteral("graph_inputs")] = arr;
    }

    // Graph-level outputs
    if (!graphOutputs.isEmpty()) {
        QJsonArray arr;
        for (const MnaPort& p : graphOutputs) {
            arr.append(p.toJson());
        }
        json[QStringLiteral("graph_outputs")] = arr;
    }

    // Parameter tree
    QJsonObject ptJson = paramTree.toJson();
    if (!ptJson.isEmpty()) {
        json[QStringLiteral("param_tree")] = ptJson;
    }

    return json;
}

//=============================================================================================================

MnaGraph MnaGraph::fromJson(const QJsonObject& json)
{
    MnaGraph graph;

    const QJsonArray nodesArr = json.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& v : nodesArr) {
        graph.addNode(MnaNode::fromJson(v.toObject()));
    }

    const QJsonArray giArr = json.value(QStringLiteral("graph_inputs")).toArray();
    for (const QJsonValue& v : giArr) {
        graph.graphInputs.append(MnaPort::fromJson(v.toObject()));
    }

    const QJsonArray goArr = json.value(QStringLiteral("graph_outputs")).toArray();
    for (const QJsonValue& v : goArr) {
        graph.graphOutputs.append(MnaPort::fromJson(v.toObject()));
    }

    if (json.contains(QStringLiteral("param_tree"))) {
        graph.paramTree = MnaParamTree::fromJson(json.value(QStringLiteral("param_tree")).toObject());
    }

    return graph;
}

//=============================================================================================================

QCborMap MnaGraph::toCbor() const
{
    QCborMap cbor;

    QCborArray nodesArr;
    for (const MnaNode& n : m_nodes) {
        nodesArr.append(n.toCbor());
    }
    cbor.insert(QStringLiteral("nodes"), nodesArr);

    if (!graphInputs.isEmpty()) {
        QCborArray arr;
        for (const MnaPort& p : graphInputs) {
            arr.append(p.toCbor());
        }
        cbor.insert(QStringLiteral("graph_inputs"), arr);
    }

    if (!graphOutputs.isEmpty()) {
        QCborArray arr;
        for (const MnaPort& p : graphOutputs) {
            arr.append(p.toCbor());
        }
        cbor.insert(QStringLiteral("graph_outputs"), arr);
    }

    return cbor;
}

//=============================================================================================================

MnaGraph MnaGraph::fromCbor(const QCborMap& cbor)
{
    MnaGraph graph;

    const QCborArray nodesArr = cbor.value(QStringLiteral("nodes")).toArray();
    for (const QCborValue& v : nodesArr) {
        graph.addNode(MnaNode::fromCbor(v.toMap()));
    }

    const QCborArray giArr = cbor.value(QStringLiteral("graph_inputs")).toArray();
    for (const QCborValue& v : giArr) {
        graph.graphInputs.append(MnaPort::fromCbor(v.toMap()));
    }

    const QCborArray goArr = cbor.value(QStringLiteral("graph_outputs")).toArray();
    for (const QCborValue& v : goArr) {
        graph.graphOutputs.append(MnaPort::fromCbor(v.toMap()));
    }

    return graph;
}
