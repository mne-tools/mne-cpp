//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_graph_executor.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Stateless executor that runs an @ref mne::MnaGraph in topological order, in batch or stream mode, and reports progress.
 *
 * @ref mne::MnaGraphExecutor is the bridge between the declarative
 * @ref mne::MnaGraph and the live op functions registered with
 * @ref mne::MnaOpRegistry. @ref mne::MnaGraphExecutor::execute "execute" walks the topologically-sorted
 * node list, feeds each one its inputs from the @ref mne::MnaGraphExecutor::Context
 * result map (keyed by @c nodeId::portName) plus the @c graph_inputs
 * map, invokes the registered op function and stores the outputs
 * back into the context; @ref mne::MnaGraphExecutor::execute_incremental "execute_incremental" restricts the
 * walk to dirty nodes and their downstream dependents so a
 * parameter tweak does not re-run the whole pipeline.
 *
 * Stream mode (@ref mne::MnaGraphExecutor::start_stream "start_stream" / @ref mne::MnaGraphExecutor::stop_stream "stop_stream") targets MNE Scan:
 * instead of calling op functions, the executor asks a host-supplied
 * @ref mne::MnaGraphExecutor::PluginFactory "PluginFactory" for a live @c QObject per node, applies
 * @ref mne::MnaParamTree values to the plugin's attributes, and wires
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
#include "mna_types.h"
#include "mna_verification.h"

#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class MnaGraph;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Executes a computational graph by traversing nodes in topological order and invoking registered operation
 * functions, external programs (IPC nodes) or interpreters (Script nodes).
 *
 * Results are stored as "nodeId::portName"; graph inputs as "graph::portName". Checks and parameter bindings see
 * the scalar values among them (numbers, booleans, text); {{name}} placeholders in IPC arguments and scripts are
 * replaced by the text of a node input or attribute.
 *
 * @brief Stateless batch and stream-mode runner for an @ref MnaGraph.
 *
 * @snippet ex_mna/main.cpp mna_graph_execute
 */
class MNASHARED_EXPORT MnaGraphExecutor
{
public:
    /** Intermediate results between nodes. */
    struct Context
    {
        MnaValues results;        ///< "nodeId::portName" → value
        MnaValues graph_inputs;   ///< Graph-level inputs (populated before execution)
        std::string aborted_node; ///< Node whose failed "error" check stopped the run (empty if it ran to the end)
    };

    /**
     * Stream execution context: graph nodes and the live plugin instances that run them.
     *
     * Used by MNE Scan to wire an MnaGraph to running plugins; the host application knows their type.
     */
    struct StreamContext
    {
        MnaGraph* graph = nullptr;                                 ///< The pipeline graph (owned externally)
        std::map<std::string, std::shared_ptr<void>> live_plugins; ///< nodeId → live plugin
        bool running = false;                                      ///< Whether the stream is active
        StringList execution_order;                                ///< Topological order used for startup/shutdown
    };

    //=========================================================================================================
    /**
     * Executes the full graph in topological order.
     *
     * @param[in,out] graph          The graph; parameter bindings and node verification are updated.
     * @param[in]     graph_inputs   Named inputs fed into graph-level input ports.
     *
     * @return The execution context with all results.
     */
    static Context execute(MnaGraph& graph, const MnaValues& graph_inputs = {});

    //=========================================================================================================
    /**
     * Executes only dirty nodes and their downstream dependents.
     *
     * @param[in,out] graph     The graph.
     * @param[in,out] context   Prior results; updated with the new ones.
     */
    static void execute_incremental(MnaGraph& graph, Context& context);

    //=========================================================================================================
    /**
     * Executes a single node.
     *
     * @param[in] node     The node.
     * @param[in] inputs   Input values keyed by port name.
     *
     * @return Output values keyed by port name; IPC and Script nodes give "stdout", "stderr" (std::string) and
     *         "exit_code" (int, -1 if the program did not run to completion).
     */
    static MnaValues execute_node(const MnaNode& node, const MnaValues& inputs);

    /** Receives the id of each node before it runs and its 1-based index out of the total. */
    using ProgressCallback = std::function<void(std::string_view node_id, int current, int total)>;

    /** @param[in] callback   Progress callback; an empty callback disables reporting. */
    static void set_progress_callback(ProgressCallback callback);

    /** Creates the live plugin of a node from its op type, or nullptr if the op type is unknown. */
    using PluginFactory = std::function<std::shared_ptr<void>(std::string_view op_type)>;

    //=========================================================================================================
    /**
     * Starts stream-mode execution: validates the graph, sorts it, applies the parameter tree and creates a
     * live plugin per node. Wiring the plugins together is up to the host application.
     *
     * @param[in,out] graph     The pipeline graph.
     * @param[in]     factory   Creates the live plugin of an op type.
     *
     * @return The stream context; running tells whether startup succeeded.
     */
    static StreamContext start_stream(MnaGraph& graph, const PluginFactory& factory);

    //=========================================================================================================
    /**
     * Stops a running stream, releasing the plugins in reverse topological order.
     *
     * @param[in,out] context   The stream context.
     */
    static void stop_stream(StreamContext& context);

private:
    /** Copies every "nodeId/attrKey" value of the graph's parameter tree into that node's attributes. */
    static void apply_param_tree(MnaGraph& graph);

    /** Runs one node with its verification; returns false if a failed "error" check stops the run. */
    static bool run_node(MnaGraph& graph, MnaNode& node, Context& context);

    /** Evaluates the node's checks of one phase against its attributes, inputs and outputs. */
    static std::vector<MnaVerificationResult> run_checks(const MnaGraph& graph, const MnaNode& node, std::string_view phase, const JsonObject& scope);
};

} // namespace mne

#endif // MNA_GRAPH_EXECUTOR_H
