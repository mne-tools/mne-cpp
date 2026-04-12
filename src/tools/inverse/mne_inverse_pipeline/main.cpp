//=============================================================================================================
/**
 * @file     main.cpp
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
 * @brief    Implements mne_inverse_pipeline — execute an MNA graph for inverse source estimation.
 *
 *           Reads a `.mna` project file describing a pipeline of processing nodes
 *           (load → filter → covariance → forward → inverse), validates the DAG,
 *           and executes it in topological order, printing a summary of each step.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_io.h>
#include <mna/mna_graph.h>
#include <mna/mna_node.h>
#include <mna/mna_op_registry.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;
using namespace UTILSLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mne_inverse_pipeline"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PROGRAM_VERSION));

    MNELogger::setupFromEnvironment();

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

    QTextStream out(stdout);

    //
    // --list-ops: print registered operators
    //
    if (parser.isSet(listOpsOpt)) {
        const auto ops = MnaOpRegistry::instance().registeredOps();
        out << QStringLiteral("Registered MNA operators (%1):\n").arg(ops.size());
        for (const auto& op : ops) {
            const auto schema = MnaOpRegistry::instance().schema(op);
            out << QStringLiteral("  %-30s  %1\n").arg(op, schema.description);
        }
        return 0;
    }

    //
    // Require project file
    //
    if (!parser.isSet(mnaFileOpt)) {
        qCritical() << "Error: --project <file> is required.";
        parser.showHelp(1);
    }

    const QString mnaPath = parser.value(mnaFileOpt);
    if (!QFile::exists(mnaPath)) {
        qCritical() << "Error: file not found:" << mnaPath;
        return 1;
    }

    //
    // Load MNA project
    //
    qInfo() << "Loading MNA project:" << mnaPath;
    MnaProject project;
    try {
        project = MnaIO::read(mnaPath);
    } catch (const std::exception& e) {
        qCritical() << "Failed to load project:" << e.what();
        return 1;
    }

    MnaGraph& graph = project.graph;

    //
    // Validate the graph
    //
    QStringList errors;
    if (!graph.validate(&errors)) {
        qCritical() << "Graph validation failed:";
        for (const auto& err : errors)
            qCritical() << "  " << err;
        return 1;
    }
    qInfo() << "Graph validated successfully.";

    //
    // Topological sort
    //
    const QStringList order = graph.topologicalSort();
    qInfo() << "Execution order:" << order.join(QStringLiteral(" -> "));

    //
    // Dry run — stop here
    //
    if (parser.isSet(dryRunOpt)) {
        out << "Dry run complete. " << order.size() << " nodes validated.\n";
        return 0;
    }

    //
    // Execute nodes in topological order
    //
    qInfo() << "Executing pipeline...";
    const auto& registry = MnaOpRegistry::instance();

    for (const auto& nodeId : order) {
        const MnaNode& n = graph.node(nodeId);
        qInfo() << "  [" << nodeId << "] op:" << n.opType;

        auto func = registry.opFunc(n.opType);
        if (!func) {
            qWarning() << "    No execution function registered for" << n.opType << "— skipping.";
            continue;
        }

        // Gather input variants from upstream nodes
        QVariantMap inputs;
        for (const auto& port : n.inputPorts) {
            if (!port.sourceNodeId.isEmpty()) {
                inputs.insert(port.name, n.runtimeOutputs.value(port.sourceNodeId));
            }
        }

        // Gather attributes as params
        QVariantMap params;
        for (const auto& attr : n.attributes) {
            params.insert(attr.key, attr.value);
        }

        QVariant result = func(params, inputs);
        qInfo() << "    Done.";
    }

    qInfo() << "Pipeline execution complete.";
    return 0;
}
