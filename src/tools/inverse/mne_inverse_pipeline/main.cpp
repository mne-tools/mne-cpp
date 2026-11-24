//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Implements mne_inverse_pipeline — execute an MNA graph for inverse source estimation.
 *
 *           Reads a `.mna` project file describing a pipeline of processing nodes
 *           (load → filter → covariance → forward → inverse), validates the DAG,
 *           and executes it in topological order, printing a summary of each step.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>
#include <mne/mna/mna_graph.h>
#include <mne/mna/mna_graph_executor.h>
#include <mne/mna/mna_op_registry.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <print>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    //
    // Command-line parsing
    //
    CommandLineParser parser("Execute an MNA analysis pipeline for inverse source estimation.\n"
                             "Reads a .mna project file, validates the processing graph, and\n"
                             "runs each node in topological order.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"p", "project"}, "Path to the .mna project file.", "file", {}});
    parser.add_option({{"n", "dry-run"}, "Validate the graph without executing it.", {}, {}});
    parser.add_option({{"list-ops"}, "List all registered operators and exit.", {}, {}});

    parser.process(argc, argv, "mne_inverse_pipeline", PROGRAM_VERSION);

    //
    // --list-ops: print registered operators
    //
    if (parser.is_set("list-ops")) {
        const MnaOpRegistry& registry = MnaOpRegistry::instance();
        const StringList ops = registry.registered_ops();
        std::println("Registered MNA operators ({}):", ops.size());
        for (const std::string& op : ops) {
            std::println("  {:<30}  {}", op, registry.schema(op).description);
        }
        return 0;
    }

    //
    // Require project file
    //
    if (!parser.is_set("project")) {
        Log::critical("Error: --project <file> is required.");
        std::print("{}", parser.help_text("mne_inverse_pipeline"));
        return 1;
    }

    //
    // Load MNA project
    //
    const std::filesystem::path mnaPath = path_from_utf8(parser.value("project"));
    Log::info("Loading MNA project: {}", utf8_from_path(mnaPath));
    Expected<MnaProject> project = MnaIO::read(mnaPath);
    if (!project) {
        Log::critical("Failed to load project: {}", project.error().what());
        return 1;
    }
    if (project->pipeline.empty()) {
        Log::critical("Error: project pipeline is empty.");
        return 1;
    }
    Log::info("Project: {} — {} pipeline nodes.", project->name, project->pipeline.size());

    //
    // Build the execution graph from pipeline nodes
    //
    MnaGraph graph;
    for (MnaNode& node : project->pipeline)
        graph.add_node(std::move(node));

    Log::info("Execution order: {}", join(graph.topological_sort(), " -> "));

    //
    // Validate
    //
    if (const StringList errors = graph.validate(); !errors.empty()) {
        Log::critical("Graph validation failed:");
        for (const std::string& err : errors)
            Log::critical("  {}", err);
        return 1;
    }

    //
    // Dry run — stop here
    //
    if (parser.is_set("dry-run")) {
        std::println("Dry run complete. {} nodes validated.", graph.nodes().size());
        return 0;
    }

    //
    // Execute via MnaGraphExecutor
    //
    Log::info("Executing pipeline...");
    const MnaGraphExecutor::Context context = MnaGraphExecutor::execute(graph);
    if (!context.aborted_node.empty()) {
        Log::critical("Pipeline stopped at node '{}' by a failed check.", context.aborted_node);
        return 1;
    }

    Log::info("Pipeline execution complete.");
    return 0;
}
