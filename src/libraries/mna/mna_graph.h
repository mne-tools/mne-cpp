//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_graph.h
 * @since April 2026
 * @brief In-memory directed acyclic graph of @ref MnaNode operations — connectivity, validation, topological sort and serialization.
 *
 * @ref MnaGraph is the executable spine of an MNA project: a
 * collection of @ref MnaNode operations wired together through
 * named @ref MnaPort connections, surrounded by graph-level inputs
 * and outputs that expose the pipeline to its host application,
 * and parametrised by a shared @ref MnaParamTree.
 *
 * The class owns four responsibilities. (1) @em Composition: add /
 * remove nodes, look them up by id, and @ref connect output ports
 * to input ports by name. (2) @em Validation: @ref validate checks
 * acyclicity, that every required input is connected, that
 * @ref MnaDataKind matches across each edge, and that every node
 * conforms to its @ref MnaOpSchema. (3) @em Scheduling:
 * @ref topologicalSort, @ref upstreamNodes, @ref downstreamNodes
 * and @ref dirtyNodes feed the executor's incremental re-run
 * logic. (4) @em Persistence: lossless JSON and CBOR round-trip
 * mirroring the @ref MnaIO codec choices.
 */

#ifndef MNA_GRAPH_H
#define MNA_GRAPH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_node.h"
#include "mna_port.h"
#include "mna_param_tree.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>
#include <QCborMap>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Directed acyclic graph of processing nodes forming a computational pipeline.
 *
 * @brief In-memory DAG of @ref MnaNode operations with validation, topological sort and JSON/CBOR persistence.
 */
class MNASHARED_EXPORT MnaGraph
{
public:
    MnaGraph();

    //=========================================================================================================
    // Node management
    //=========================================================================================================

    void addNode(const MnaNode& node);
    void removeNode(const QString& nodeId);
    MnaNode& node(const QString& nodeId);
    const MnaNode& node(const QString& nodeId) const;
    QList<MnaNode>& nodes();
    const QList<MnaNode>& nodes() const;
    bool hasNode(const QString& nodeId) const;

    //=========================================================================================================
    // Graph-level ports
    //=========================================================================================================

    QList<MnaPort> graphInputs;     ///< Named, typed entry points
    QList<MnaPort> graphOutputs;    ///< Named, typed exit points

    //=========================================================================================================
    // Parameter tree
    //=========================================================================================================

    MnaParamTree paramTree;         ///< Hierarchical parameter store with formula-driven bindings

    //=========================================================================================================
    // Connection helper
    //=========================================================================================================

    /**
     * Connect output port of srcNode to input port of dstNode.
     * Sets the sourceNodeId and sourcePortName on the destination input port.
     * @return true if connection was made, false if ports not found.
     */
    bool connect(const QString& srcNodeId, const QString& srcPortName,
                 const QString& dstNodeId, const QString& dstPortName);

    //=========================================================================================================
    // Validation
    //=========================================================================================================

    /**
     * Validate the graph: acyclicity, port connections, data-kind compatibility,
     * op schema compliance, required attributes.
     * @param errors    Optional list to receive error messages.
     * @return true if valid.
     */
    bool validate(QStringList* errors = nullptr) const;

    //=========================================================================================================
    // Topological sort
    //=========================================================================================================

    QStringList topologicalSort() const;

    //=========================================================================================================
    // Dependency queries
    //=========================================================================================================

    QStringList upstreamNodes(const QString& nodeId) const;
    QStringList downstreamNodes(const QString& nodeId) const;
    QStringList dirtyNodes() const;

    //=========================================================================================================
    // Serialization
    //=========================================================================================================

    QJsonObject toJson() const;
    static MnaGraph fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaGraph fromCbor(const QCborMap& cbor);

private:
    QList<MnaNode> m_nodes;
};

} // namespace MNALIB

#endif // MNA_GRAPH_H
