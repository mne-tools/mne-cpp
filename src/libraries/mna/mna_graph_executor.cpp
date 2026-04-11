//=============================================================================================================
/**
 * @file     mna_graph_executor.cpp
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
 * @brief    MnaGraphExecutor class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph_executor.h"
#include "mna_graph.h"
#include "mna_op_registry.h"

#include <QDir>
#include <QTemporaryFile>
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

    // Apply current parameter tree values to node attributes
    for (MnaNode& n : graph.nodes()) {
        for (const QString& path : graph.paramTree.allPaths()) {
            // Path format: "nodeId/attrKey"
            int sep = path.indexOf(QLatin1Char('/'));
            if (sep > 0) {
                QString nodeId  = path.left(sep);
                QString attrKey = path.mid(sep + 1);
                if (nodeId == n.id) {
                    n.attributes.insert(attrKey, graph.paramTree.param(path));
                }
            }
        }
    }

    const QStringList order = graph.topologicalSort();
    const int total = order.size();

    for (int i = 0; i < total; ++i) {
        const QString& nodeId = order[i];

        if (s_progressCallback) {
            s_progressCallback(nodeId, i + 1, total);
        }

        MnaNode& n = graph.node(nodeId);

        // Gather inputs from upstream results
        QVariantMap inputs;
        for (const MnaPort& p : n.inputs) {
            if (!p.sourceNodeId.isEmpty()) {
                QString key = p.sourceNodeId + QStringLiteral("::") + p.sourcePortName;
                inputs.insert(p.name, ctx.results.value(key));
            }
        }

        // Execute the node
        QVariantMap outputs = executeNode(n, inputs);

        // Store outputs in context
        for (auto it = outputs.constBegin(); it != outputs.constEnd(); ++it) {
            ctx.results.insert(nodeId + QStringLiteral("::") + it.key(), it.value());
        }

        n.dirty = false;
        n.executedAt = QDateTime::currentDateTimeUtc();
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

        MnaNode& n = graph.node(nodeId);

        QVariantMap inputs;
        for (const MnaPort& p : n.inputs) {
            if (!p.sourceNodeId.isEmpty()) {
                QString key = p.sourceNodeId + QStringLiteral("::") + p.sourcePortName;
                inputs.insert(p.name, existing.results.value(key));
            }
        }

        QVariantMap outputs = executeNode(n, inputs);

        for (auto it = outputs.constBegin(); it != outputs.constEnd(); ++it) {
            existing.results.insert(nodeId + QStringLiteral("::") + it.key(), it.value());
        }

        n.dirty = false;
        n.executedAt = QDateTime::currentDateTimeUtc();
    }

    graph.paramTree.evaluate(existing.results);

    return existing;
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
        if (script.language == QLatin1String("python"))      ext = QStringLiteral(".py");
        else if (script.language == QLatin1String("shell"))  ext = QStringLiteral(".sh");
        else if (script.language == QLatin1String("r"))      ext = QStringLiteral(".R");
        else if (script.language == QLatin1String("matlab")) ext = QStringLiteral(".m");
        else if (script.language == QLatin1String("octave")) ext = QStringLiteral(".m");
        else if (script.language == QLatin1String("julia"))  ext = QStringLiteral(".jl");

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
            if (script.language == QLatin1String("python"))      interpreter = QStringLiteral("python3");
            else if (script.language == QLatin1String("shell"))  interpreter = QStringLiteral("/bin/bash");
            else if (script.language == QLatin1String("r"))      interpreter = QStringLiteral("Rscript");
            else if (script.language == QLatin1String("matlab")) interpreter = QStringLiteral("matlab");
            else if (script.language == QLatin1String("octave")) interpreter = QStringLiteral("octave");
            else if (script.language == QLatin1String("julia"))  interpreter = QStringLiteral("julia");
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
