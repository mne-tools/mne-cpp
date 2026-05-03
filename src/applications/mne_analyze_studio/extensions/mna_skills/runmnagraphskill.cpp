//=============================================================================================================
/**
 * @file     runmnagraphskill.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    RunMnaGraphSkill implementation.
 *
 *           Executes the pipeline of an MNA project. The project is loaded
 *           from disk, its `pipeline` list (List<MnaNode>) is wrapped into a
 *           transient `MnaGraph`, and `MnaGraphExecutor::execute` is called.
 *           The produced result keys are reported back into the workflow
 *           graph as a JSON summary so downstream Studio steps can branch
 *           on success/failure.
 */

#include "runmnagraphskill.h"

#include <workflowgraph.h>

#include <mna/mna_graph.h>
#include <mna/mna_graph_executor.h>
#include <mna/mna_io.h>
#include <mna/mna_project.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

using namespace MNEANALYZESTUDIO;
using namespace MNALIB;

namespace {

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QString resolveUri(const QString& uri)
{
    if (uri.startsWith(QLatin1String("file://"))) {
        return uri.mid(7);
    }
    return uri;
}

} // namespace

RunMnaGraphSkill::RunMnaGraphSkill(QObject* parent)
    : ISkillOperator(parent)
{
}

QJsonObject RunMnaGraphSkill::getOperatorDefinition() const
{
    return QJsonObject{
        {"skill_id", "mne.skills.run_mna_graph"},
        {"tool_name", "run_mna_graph"},
        {"display_name", "Run MNA Pipeline"},
        {"description", "Execute the pipeline of an MNA project and report the resulting context keys."},
        {"extension_id", "mna-skills"},
        {"extension_display_name", "MNA Project Skills"},
        {"inputs_schema", objectSchema(QJsonObject{
            {"project_path", QJsonObject{
                {"type", "string"},
                {"title", "MNA project path"}
            }}
        }, QJsonArray{"project_path"})},
        {"parameters_schema", objectSchema(QJsonObject{
            {"graph_inputs", QJsonObject{
                {"type", "object"},
                {"title", "Graph inputs"},
                {"description", "Optional name->value map injected into MnaGraphExecutor::execute."}
            }}
        })},
        {"outputs_schema", objectSchema(QJsonObject{
            {"result_keys", QJsonObject{
                {"type", "array"},
                {"title", "Result keys"},
                {"description", "List of nodeId::portName keys produced by the executor."}
            }},
            {"node_count", QJsonObject{
                {"type", "integer"},
                {"title", "Node count"}
            }}
        }, QJsonArray{"result_keys", "node_count"})}
    };
}

QJsonObject RunMnaGraphSkill::executeSkill(const WorkflowNode& nodeState)
{
    const QJsonObject inObj = nodeState.resolvedInputs.value("project_path").toObject();
    QString uri = inObj.value("uri").toString();
    if (uri.isEmpty()) {
        uri = nodeState.resolvedInputs.value("project_path").toString();
    }
    if (uri.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Run MNA node `%1`: missing `project_path` input.").arg(nodeState.uid)}
        };
    }

    const QString path = resolveUri(uri);
    if (!QFileInfo::exists(path)) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Run MNA node `%1`: project `%2` not found.").arg(nodeState.uid, path)}
        };
    }

    const MnaProject project = MnaIO::read(path);

    // Wrap the project pipeline into a transient graph for the executor.
    MnaGraph graph;
    for (const MnaNode& node : project.pipeline) {
        graph.addNode(node);
    }

    QStringList validationErrors;
    if (!graph.validate(&validationErrors)) {
        QJsonArray errs;
        for (const QString& e : validationErrors) {
            errs.append(e);
        }
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Run MNA node `%1`: graph validation failed.").arg(nodeState.uid)},
            {"errors", errs}
        };
    }

    QVariantMap graphInputs = nodeState.parameters.value("graph_inputs")
                                  .toObject().toVariantMap();

    const auto context = MnaGraphExecutor::execute(graph, graphInputs);

    QJsonArray keys;
    for (auto it = context.results.constBegin(); it != context.results.constEnd(); ++it) {
        keys.append(it.key());
    }

    return QJsonObject{
        {"status", "completed"},
        {"message", QStringLiteral("Executed MNA pipeline `%1` (%2 nodes, %3 result keys).")
                        .arg(QFileInfo(path).fileName())
                        .arg(graph.nodes().size())
                        .arg(keys.size())},
        {"outputs", QJsonObject{
            {"result_keys", keys},
            {"node_count", static_cast<int>(graph.nodes().size())}
        }}
    };
}
