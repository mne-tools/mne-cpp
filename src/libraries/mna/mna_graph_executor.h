//=============================================================================================================
/**
 * @file     mna_graph_executor.h
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
 * @brief    MnaGraphExecutor class declaration — executes a computational graph.
 *
 */

#ifndef MNA_GRAPH_EXECUTOR_H
#define MNA_GRAPH_EXECUTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_node.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMap>
#include <QVariant>
#include <QVariantMap>
#include <functional>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNALIB
{
class MnaGraph;
}

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Executes a computational graph by traversing nodes in topological order
 * and invoking registered operation functions.
 *
 * @brief Graph executor for MNA pipelines.
 */
class MNASHARED_EXPORT MnaGraphExecutor
{
public:
    /**
     * Execution context — holds intermediate results between nodes.
     */
    struct Context
    {
        /// nodeId::portName → data (QVariant wrapping domain objects or file paths)
        QMap<QString, QVariant> results;

        /// Graph-level inputs (populated before execution)
        QVariantMap graphInputs;
    };

    /**
     * Execute the full graph (all nodes in topological order).
     * @param graph         The graph to execute.
     * @param graphInputs   Named inputs fed into graph-level input ports.
     * @return Execution context with all results.
     */
    static Context execute(MnaGraph& graph, const QVariantMap& graphInputs);

    /**
     * Execute only dirty nodes and their downstream dependents.
     * @param graph     The graph to execute.
     * @param existing  Existing context with prior results.
     * @return Updated context.
     */
    static Context executeIncremental(MnaGraph& graph, Context& existing);

    /**
     * Execute a single node (for testing/debugging).
     * @param node      The node to execute.
     * @param inputs    Input data keyed by port name.
     * @return Output data keyed by port name.
     */
    static QVariantMap executeNode(const MnaNode& node,
                                    const QVariantMap& inputs);

    /// Progress callback type.
    using ProgressCallback = std::function<void(const QString& nodeId,
                                                 int current, int total)>;

    /**
     * Set a progress callback invoked for each node execution.
     */
    static void setProgressCallback(ProgressCallback cb);

private:
    static ProgressCallback s_progressCallback;
};

} // namespace MNALIB

#endif // MNA_GRAPH_EXECUTOR_H
