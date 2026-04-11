//=============================================================================================================
/**
 * @file     mna_graph.h
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
 * @brief    MnaGraph class declaration — directed acyclic graph of processing nodes.
 *
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
 * @brief MNA computational graph.
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
