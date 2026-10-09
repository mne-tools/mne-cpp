//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_graph_executor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MnaGraphExecutor — batch / incremental graph walk plus MNE-Scan-friendly stream-mode plugin wiring.
 *
 * @ref MnaGraphExecutor::execute first validates the graph, then
 * runs @ref MnaGraph::topologicalSort and visits every node in
 * order. For each node it gathers inputs from the
 * @ref Context::results map keyed by @c srcNodeId::srcPortName,
 * falls back to @ref Context::graphInputs for graph-level entry
 * ports, looks the @c opType up in @ref MnaOpRegistry, invokes
 * the registered @ref MnaOpRegistry::OpFunc with the merged
 * inputs and node attributes, and writes the returned outputs
 * back into the context map. @ref executeIncremental reuses the
 * same loop but restricts it to nodes returned by
 * @ref MnaGraph::dirtyNodes and their @ref MnaGraph::downstreamNodes.
 *
 * @ref startStream / @ref stopStream provide the MNE Scan
 * integration point: the host application supplies a
 * @ref PluginFactory that maps @c opType strings to live
 * @c QObject plugin instances; the executor instantiates one per
 * node in topological order, applies @ref MnaParamTree values to
 * their attributes, and stores the resulting @ref StreamContext
 * so the host can wire signal/slot connections by the matching
 * @ref MnaDataKind on each port and tear down the pipeline in
 * reverse order on stop.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph_executor.h"
#include "mna_graph.h"
#include "mna_op_registry.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QSysInfo>
#include <QTemporaryFile>

#include <algorithm>
#ifndef WASMBUILD
#include <QProcess>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// STATIC INITIALIZATION
//=============================================================================================================

MnaGraphExecutor::ProgressCallback MnaGraphExecutor::s_progressCallback;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaGraphExecutor::Context MnaGraphExecutor::execute(MnaGraph& graph,
                                                    const QVariantMap& graphInputs)
{
    Context ctx;
    ctx.graphInputs = graphInputs;

    // Populate context with graph-level inputs keyed as "graph::portName"
    for (auto it = graphInputs.constBegin(); it != graphInputs.constEnd(); ++it) {
        ctx.results.insert(QStringLiteral("graph::") + it.key(), it.value());
    }

    // Evaluate parameter tree bindings before execution
    graph.paramTree.evaluate(ctx.results);
    applyParamTree(graph);

    const QStringList order = graph.topologicalSort();
    const int total = order.size();

    for (int i = 0; i < total; ++i) {
        const QString& nodeId = order[i];

        if (s_progressCallback) {
            s_progressCallback(nodeId, i + 1, total);
        }

        if (!runNode(graph, graph.node(nodeId), ctx)) {
            break;
        }
    }

    // Re-evaluate parameter tree after execution (for on_change bindings)
    graph.paramTree.evaluate(ctx.results);

    return ctx;
}

//=============================================================================================================

MnaGraphExecutor::Context MnaGraphExecutor::executeIncremental(MnaGraph& graph,
                                                               Context& existing)
{
    // Find dirty nodes and all their downstream dependents
    QStringList dirty = graph.dirtyNodes();
    QSet<QString> toExecute;
    for (const QString& nodeId : dirty) {
        toExecute.insert(nodeId);
        const QStringList downstream = graph.downstreamNodes(nodeId);
        for (const QString& d : downstream) {
            toExecute.insert(d);
        }
    }

    applyParamTree(graph);

    // Get topological order, filter to only those that need execution
    const QStringList fullOrder = graph.topologicalSort();
    QStringList order;
    for (const QString& nodeId : fullOrder) {
        if (toExecute.contains(nodeId)) {
            order.append(nodeId);
        }
    }

    const int total = order.size();

    for (int i = 0; i < total; ++i) {
        const QString& nodeId = order[i];

        if (s_progressCallback) {
            s_progressCallback(nodeId, i + 1, total);
        }

        if (!runNode(graph, graph.node(nodeId), existing)) {
            break;
        }
    }

    graph.paramTree.evaluate(existing.results);

    return existing;
}

//=============================================================================================================

void MnaGraphExecutor::applyParamTree(MnaGraph& graph)
{
    for (const QString& path : graph.paramTree.allPaths()) {
        // Path format: "nodeId/attrKey"
        const int sep = path.indexOf(QLatin1Char('/'));
        if (sep > 0 && graph.hasNode(path.left(sep))) {
            graph.node(path.left(sep)).attributes.insert(path.mid(sep + 1), graph.paramTree.param(path));
        }
    }
}

//=============================================================================================================

bool MnaGraphExecutor::runNode(MnaGraph& graph, MnaNode& node, Context& ctx)
{
    QVariantMap inputs;
    for (const MnaPort& p : node.inputs) {
        if (!p.sourceNodeId.isEmpty()) {
            inputs.insert(p.name, ctx.results.value(p.sourceNodeId + QStringLiteral("::") + p.sourcePortName));
        }
    }
    // Checks see the node's attributes and inputs by name, post checks its outputs too.
    QVariantMap scope = node.attributes;
    scope.insert(inputs);

    MnaVerification& verification = node.verification;
    MnaProvenance& provenance = verification.provenance;
    provenance = MnaProvenance();
    provenance.mneCppVersion = QStringLiteral(MNE_CPP_VERSION);
    provenance.qtVersion = QString::fromLatin1(qVersion());
    provenance.osInfo = QSysInfo::prettyProductName() + QLatin1Char(' ') + QSysInfo::currentCpuArchitecture();
    provenance.hostName = QSysInfo::machineHostName();
    provenance.resolvedAttributes = node.attributes;
    for (auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
        provenance.inputHashes.insert(it.key(), QString::fromLatin1(QCryptographicHash::hash(it.value().toString().toUtf8(), QCryptographicHash::Sha256).toHex()));
    }

    const auto failsHard = [](const QList<MnaVerificationResult>& results) {
        return std::any_of(results.cbegin(), results.cend(), [](const MnaVerificationResult& r) {
            return !r.passed && r.severity == QLatin1String("error");
        });
    };
    verification.preResults = runChecks(graph, node, QStringLiteral("pre"), scope);
    verification.postResults.clear();
    if (failsHard(verification.preResults)) {
        ctx.abortedNode = node.id;
        return false;
    }

    provenance.startedAt = QDateTime::currentDateTimeUtc();
    QElapsedTimer timer;
    timer.start();
    const QVariantMap outputs = executeNode(node, inputs);
    provenance.wallTimeMs = timer.elapsed();
    provenance.finishedAt = QDateTime::currentDateTimeUtc();

    for (auto it = outputs.constBegin(); it != outputs.constEnd(); ++it) {
        ctx.results.insert(node.id + QStringLiteral("::") + it.key(), it.value());
    }
    node.dirty = false;
    node.executedAt = provenance.finishedAt;

    scope.insert(outputs);
    verification.postResults = runChecks(graph, node, QStringLiteral("post"), scope);
    if (failsHard(verification.postResults)) {
        ctx.abortedNode = node.id;
        return false;
    }
    return true;
}

//=============================================================================================================

QList<MnaVerificationResult> MnaGraphExecutor::runChecks(const MnaGraph& graph, const MnaNode& node, const QString& phase, const QVariantMap& scope)
{
    QList<MnaVerificationResult> results;
    for (const MnaVerificationCheck& check : node.verification.checks) {
        if (check.phase != phase) {
            continue;
        }
        MnaVerificationResult result;
        result.checkId = check.id;
        result.severity = check.severity;
        result.evaluatedAt = QDateTime::currentDateTimeUtc();
        if (!check.script.code.isEmpty()) {
            MnaNode scriptNode = node;
            scriptNode.execMode = MnaNodeExecMode::Script;
            scriptNode.script = check.script;
            const QVariantMap out = executeNode(scriptNode, scope);
            result.actualValue = out.value(QStringLiteral("exit_code"));
            result.passed = result.actualValue.toInt() == 0;
        } else {
            result.actualValue = graph.paramTree.evaluateExpression(check.expression, scope);
            result.passed = result.actualValue.userType() == QMetaType::Bool && result.actualValue.toBool();
        }
        result.message = result.passed ? QStringLiteral("PASS: %1").arg(check.description)
                                       : QStringLiteral("FAIL [%1]: %2").arg(check.severity, check.description);
        if (!result.passed && !check.onFail.isEmpty()) {
            result.message += QStringLiteral(" (%1)").arg(check.onFail);
        }
        results.append(result);
    }
    return results;
}

//=============================================================================================================

QVariantMap MnaGraphExecutor::executeNode(const MnaNode& node,
                                          const QVariantMap& inputs)
{
    // Script execution — inline code via interpreter
    if (node.execMode == MnaNodeExecMode::Script) {
#ifdef WASMBUILD
        QVariantMap outputs;
        outputs.insert(QStringLiteral("stderr"), QStringLiteral("Script execution not supported in WebAssembly build (QProcess unavailable)"));
        outputs.insert(QStringLiteral("exit_code"), -1);
        return outputs;
#else
        const MnaScript& script = node.script;

        // Determine file extension from language
        QString ext = QStringLiteral(".txt");
        if (script.language == QLatin1String("python"))
            ext = QStringLiteral(".py");
        else if (script.language == QLatin1String("shell"))
            ext = QStringLiteral(".sh");
        else if (script.language == QLatin1String("r"))
            ext = QStringLiteral(".R");
        else if (script.language == QLatin1String("matlab"))
            ext = QStringLiteral(".m");
        else if (script.language == QLatin1String("octave"))
            ext = QStringLiteral(".m");
        else if (script.language == QLatin1String("julia"))
            ext = QStringLiteral(".jl");

        // Substitute {{placeholder}} tokens in the code
        QString code = script.code;
        for (auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
            code.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"),
                         it.value().toString());
        }
        for (auto it = node.attributes.constBegin(); it != node.attributes.constEnd(); ++it) {
            code.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"),
                         it.value().toString());
        }

        // Write code to temporary file
        QTemporaryFile tempFile(QDir::tempPath() + QStringLiteral("/mna_script_XXXXXX") + ext);
        tempFile.setAutoRemove(!script.keepTempFile);
        if (!tempFile.open()) {
            QVariantMap outputs;
            outputs.insert(QStringLiteral("stderr"), QStringLiteral("Failed to create temporary script file"));
            outputs.insert(QStringLiteral("exit_code"), -1);
            return outputs;
        }
        tempFile.write(code.toUtf8());
        tempFile.close();

        // Determine interpreter
        QString interpreter = script.interpreter;
        if (interpreter.isEmpty()) {
            if (script.language == QLatin1String("python"))
                interpreter = QStringLiteral("python3");
            else if (script.language == QLatin1String("shell"))
                interpreter = QStringLiteral("/bin/bash");
            else if (script.language == QLatin1String("r"))
                interpreter = QStringLiteral("Rscript");
            else if (script.language == QLatin1String("matlab"))
                interpreter = QStringLiteral("matlab");
            else if (script.language == QLatin1String("octave"))
                interpreter = QStringLiteral("octave");
            else if (script.language == QLatin1String("julia"))
                interpreter = QStringLiteral("julia");
        }

        QStringList args = script.interpreterArgs;
        args.append(tempFile.fileName());

        QProcess process;
        process.start(interpreter, args);
        process.waitForFinished(-1);

        QVariantMap outputs;
        outputs.insert(QStringLiteral("stdout"), QString::fromUtf8(process.readAllStandardOutput()));
        outputs.insert(QStringLiteral("stderr"), QString::fromUtf8(process.readAllStandardError()));
        outputs.insert(QStringLiteral("exit_code"), process.exitCode());

        return outputs;
#endif
    }

    // IPC execution
    if (node.execMode == MnaNodeExecMode::Ipc) {
#ifdef WASMBUILD
        QVariantMap outputs;
        outputs.insert(QStringLiteral("stderr"), QStringLiteral("IPC execution not supported in WebAssembly build (QProcess unavailable)"));
        outputs.insert(QStringLiteral("exit_code"), -1);
        return outputs;
#else
        QProcess process;
        if (!node.ipcWorkDir.isEmpty()) {
            process.setWorkingDirectory(node.ipcWorkDir);
        }

        // Substitute {{placeholder}} tokens in arguments
        QStringList resolvedArgs;
        for (const QString& arg : node.ipcArgs) {
            QString resolved = arg;
            for (auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
                resolved.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"),
                                 it.value().toString());
            }
            // Also substitute from attributes
            for (auto it = node.attributes.constBegin(); it != node.attributes.constEnd(); ++it) {
                resolved.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"),
                                 it.value().toString());
            }
            resolvedArgs.append(resolved);
        }

        process.start(node.ipcCommand, resolvedArgs);
        process.waitForFinished(-1);

        QVariantMap outputs;
        outputs.insert(QStringLiteral("stdout"), QString::fromUtf8(process.readAllStandardOutput()));
        outputs.insert(QStringLiteral("stderr"), QString::fromUtf8(process.readAllStandardError()));
        outputs.insert(QStringLiteral("exit_code"), process.exitCode());

        // Populate outputs from cached results if specified
        for (const MnaPort& p : node.outputs) {
            if (!p.cachedResultPath.isEmpty()) {
                outputs.insert(p.name, p.cachedResultPath);
            }
        }

        return outputs;
#endif
    }

    // Look up registered op function
    const MnaOpRegistry& registry = MnaOpRegistry::instance();
    MnaOpRegistry::OpFunc func = registry.opFunc(node.opType);

    if (func) {
        return func(inputs, node.attributes);
    }

    // No implementation registered — return empty
    return {};
}

//=============================================================================================================

void MnaGraphExecutor::setProgressCallback(ProgressCallback cb)
{
    s_progressCallback = cb;
}

//=============================================================================================================
// Stream-mode execution
//=============================================================================================================

MnaGraphExecutor::StreamContext MnaGraphExecutor::startStream(MnaGraph& graph,
                                                              PluginFactory factory)
{
    StreamContext ctx;
    ctx.graph = &graph;

    // 1. Validate the graph
    QStringList errors;
    if (!graph.validate(&errors)) {
        qWarning() << "MnaGraphExecutor::startStream - graph validation failed:" << errors;
        return ctx;
    }

    // 2. Topological sort
    ctx.executionOrder = graph.topologicalSort();

    // 3. Apply current parameter tree values to node attributes
    applyParamTree(graph);

    // 4. Instantiate live plugins via factory
    for (const QString& nodeId : ctx.executionOrder) {
        const MnaNode& n = graph.node(nodeId);
        QObject* plugin = factory(n.opType);
        if (!plugin) {
            qWarning() << "MnaGraphExecutor::startStream - factory returned nullptr for opType:" << n.opType;
            // Clean up already-created plugins
            for (QObject* p : ctx.livePlugins) {
                delete p;
            }
            ctx.livePlugins.clear();
            return ctx;
        }
        ctx.livePlugins.insert(nodeId, plugin);
    }

    // 5. Wiring: the host application is responsible for connecting
    //    Qt signals/slots between the QObject* instances based on
    //    the port connections encoded in each node's input ports.
    //    The mna library provides the graph topology; the host app
    //    knows the concrete signal/slot signatures.

    ctx.running = true;
    return ctx;
}

//=============================================================================================================

void MnaGraphExecutor::stopStream(StreamContext& ctx)
{
    if (!ctx.running) {
        return;
    }

    ctx.running = false;

    // Stop in reverse topological order
    for (int i = ctx.executionOrder.size() - 1; i >= 0; --i) {
        const QString& nodeId = ctx.executionOrder[i];
        QObject* plugin = ctx.livePlugins.value(nodeId);
        delete plugin;
    }

    ctx.livePlugins.clear();
    ctx.executionOrder.clear();
    ctx.graph = nullptr;
}
