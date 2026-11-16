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

#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_graph.h>
#include <mna/mna_graph_executor.h>
#include <mna/mna_op_registry.h>

#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

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
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mne_inverse_pipeline"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PROGRAM_VERSION));

    mne::Log::install_qt_message_handler();

    //
    // Command-line parsing
    //
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Execute an MNA analysis pipeline for inverse source estimation.\n"
                       "Reads a .mna project file, validates the processing graph, and\n"
                       "runs each node in topological order."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption mnaFileOpt(
        QStringList() << QStringLiteral("p") << QStringLiteral("project"),
        QStringLiteral("Path to the .mna project file."),
        QStringLiteral("file"));
    parser.addOption(mnaFileOpt);

    QCommandLineOption dryRunOpt(
        QStringList() << QStringLiteral("n") << QStringLiteral("dry-run"),
        QStringLiteral("Validate the graph without executing it."));
    parser.addOption(dryRunOpt);

    QCommandLineOption listOpsOpt(
        QStringLiteral("list-ops"),
        QStringLiteral("List all registered operators and exit."));
    parser.addOption(listOpsOpt);

    parser.process(app);

    //
    // --list-ops: print registered operators
    //
    if (parser.isSet(listOpsOpt)) {
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
    if (!parser.isSet(mnaFileOpt)) {
        Log::critical("Error: --project <file> is required.");
        parser.showHelp(1);
    }

    //
    // Load MNA project
    //
    const std::filesystem::path mnaPath = path_from_utf8(parser.value(mnaFileOpt).toStdString());
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
    if (parser.isSet(dryRunOpt)) {
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
