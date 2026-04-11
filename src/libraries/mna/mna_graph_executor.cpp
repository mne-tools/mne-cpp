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
 * @brief    MnaGraphExecutor class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_graph_executor.h"
#include "mna_graph.h"
#include "mna_op_registry.h"

#include <QProcess>
#include <QDir>

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
    // IPC execution
    if (node.execMode == MnaNodeExecMode::Ipc) {
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
