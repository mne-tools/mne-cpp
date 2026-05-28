//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_graph_executor.h
 * @since April 2026
 * @brief Stateless executor that runs an @ref MnaGraph in topological order, in batch or stream mode, and reports progress.
 *
 * @ref MnaGraphExecutor is the bridge between the declarative
 * @ref MnaGraph and the live op functions registered with
 * @ref MnaOpRegistry. @ref execute walks the topologically-sorted
 * node list, feeds each one its inputs from the @ref Context
 * result map (keyed by @c nodeId::portName) plus the @c graphInputs
 * map, invokes the registered op function and stores the outputs
 * back into the context; @ref executeIncremental restricts the
 * walk to dirty nodes and their downstream dependents so a
 * parameter tweak does not re-run the whole pipeline.
 *
 * Stream mode (@ref startStream / @ref stopStream) targets MNE Scan:
 * instead of calling op functions, the executor asks a host-supplied
 * @ref PluginFactory for a live @c QObject per node, applies
 * @ref MnaParamTree values to the plugin's attributes, and wires
 * up port connections so the graph drives a continuously-running
 * real-time pipeline. The mna library deliberately holds plugins
 * as @c QObject* to avoid pulling in MNE Scan as a dependency.
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
#include <QObject>
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
 * @brief Stateless batch and stream-mode runner for an @ref MnaGraph.
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

    //=========================================================================================================
    /**
     * Stream execution context — maps graph nodes to live plugin instances.
     *
     * Used by MNE Scan to wire an MnaGraph to running AbstractPlugin objects.
     * The mna library uses QObject* to avoid depending on MNE Scan libraries;
     * the host application is responsible for casting to the actual plugin type.
     */
    struct StreamContext
    {
        MnaGraph*                  graph = nullptr;         ///< The pipeline graph (owned externally)
        QMap<QString, QObject*>    livePlugins;             ///< nodeId → live plugin instance (QObject* avoids scan dependency)
        bool                       running = false;         ///< Whether the stream is active
        QStringList                executionOrder;          ///< Topological order used for startup/shutdown
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

    //=========================================================================================================
    // Stream-mode execution
    //=========================================================================================================

    /// Factory callback: given a node's opType, create the corresponding live plugin.
    /// Returns nullptr if the opType is unknown.  The host app provides this callback.
    using PluginFactory = std::function<QObject*(const QString& opType)>;

    /**
     * Start stream-mode execution of a graph.
     *
     * 1. Validates the graph
     * 2. Performs topological sort
     * 3. For each node, calls @p factory to instantiate a live plugin
     * 4. Applies MnaParamTree values to node attributes
     * 5. Wires connections based on port dataKind matching
     *
     * The mna library does NOT depend on MNE Scan — the host app provides
     * the factory and is responsible for wiring Qt signal/slot connections
     * between the returned QObject* instances.
     *
     * @param[in,out] graph     The pipeline graph.
     * @param[in]     factory   Callback that maps opType → live QObject* plugin.
     * @return Stream context.  Check ctx.running to see if startup succeeded.
     */
    static StreamContext startStream(MnaGraph& graph, PluginFactory factory);

    /**
     * Stop a running stream.  Iterates nodes in reverse topological order
     * and deletes the live plugin objects.
     *
     * @param[in,out] ctx   The stream context to stop.
     */
    static void stopStream(StreamContext& ctx);

private:
    static ProgressCallback s_progressCallback;
};

} // namespace MNALIB

#endif // MNA_GRAPH_EXECUTOR_H
