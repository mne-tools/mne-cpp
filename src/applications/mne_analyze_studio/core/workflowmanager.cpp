//=============================================================================================================
/**
 * @file     workflowmanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the workflow manager that owns the active DAG and skill operators.
 */

#include "workflowmanager.h"

#include "capabilityutils.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

constexpr const char* kActiveGraphResourceUri = "mne://workspace/active_graph";

QJsonObject stringSchema(const QString& title, const QString& description = QString())
{
    QJsonObject schema{
        {"type", "string"},
        {"title", title}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QStringList requiredFieldNames(const QJsonObject& schema)
{
    QStringList required;
    const QJsonArray requiredArray = schema.value(QStringLiteral("required")).toArray();
    for(const QJsonValue& value : requiredArray) {
        const QString key = value.toString().trimmed();
        if(!key.isEmpty()) {
            required.append(key);
        }
    }

    return required;
}

QJsonObject propertySchemaWithSection(const QJsonObject& schema, const QString& sectionName)
{
    QJsonObject propertySchema = schema;
    propertySchema.insert(QStringLiteral("x_workflow_section"), sectionName);
    return propertySchema;
}

QString valueToUid(const QJsonValue& value)
{
    return value.toString().trimmed();
}

}

WorkflowManager::WorkflowManager(QObject* parent)
: QObject(parent)
, m_pipelineParser(this)
{
}

void WorkflowManager::registerOperator(ISkillOperator* skillOperator)
{
    if(!skillOperator) {
        throw WorkflowValidationError(QStringLiteral("Attempted to register a null skill operator."));
    }

    if(skillOperator->parent() != this) {
        skillOperator->setParent(this);
    }

    const QJsonObject definition = skillOperator->getOperatorDefinition();
    const QString toolName = definition.value(QStringLiteral("tool_name")).toString().trimmed();
    const QString skillId = definition.value(QStringLiteral("skill_id")).toString().trimmed();

    if(toolName.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Registered skill operator is missing `tool_name`."));
    }

    if(skillId.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Registered skill operator `%1` is missing `skill_id`.").arg(toolName));
    }

    if(m_registrationsByToolName.contains(toolName)) {
        const OperatorRegistration existingRegistration = m_registrationsByToolName.value(toolName);
        throw WorkflowValidationError(QStringLiteral("Workflow tool name `%1` is already registered for skill `%2`.")
                                          .arg(toolName, existingRegistration.skillId));
    }

    if(m_registrationsBySkillId.contains(skillId)) {
        const OperatorRegistration existingRegistration = m_registrationsBySkillId.value(skillId);
        throw WorkflowValidationError(QStringLiteral("Workflow skill id `%1` is already registered for tool `%2`.")
                                          .arg(skillId, existingRegistration.toolName));
    }

    OperatorRegistration registration;
    registration.toolName = toolName;
    registration.skillId = skillId;
    registration.displayName = definition.value(QStringLiteral("display_name")).toString(toolName);
    registration.description = definition.value(QStringLiteral("description")).toString();
    registration.extensionId = definition.value(QStringLiteral("extension_id")).toString();
    registration.extensionDisplayName = definition.value(QStringLiteral("extension_display_name")).toString();
    registration.definition = definition;
    registration.skillOperator = skillOperator;

    m_registrationsByToolName.insert(toolName, registration);
    m_registrationsBySkillId.insert(skillId, registration);
}

bool WorkflowManager::canHandleTool(const QString& toolName) const
{
    return m_registrationsByToolName.contains(toolName.trimmed());
}

QJsonArray WorkflowManager::toolDefinitions() const
{
    QStringList toolNames = m_registrationsByToolName.keys();
    std::sort(toolNames.begin(), toolNames.end());

    QJsonArray tools;
    for(const QString& toolName : toolNames) {
        tools.append(translateOperatorToToolDefinition(m_registrationsByToolName.value(toolName)));
    }

    return tools;
}

QJsonArray WorkflowManager::resourceDefinitions() const
{
    return QJsonArray{
        QJsonObject{
            {"id", activeGraphResourceUri()},
            {"uri", activeGraphResourceUri()},
            {"display_name", "Active Workflow Graph"},
            {"description", "Serialized provenance for the currently active .mne DAG."},
            {"mime_type", "application/json"},
            {"kind", "workflow_graph"},
            {"extension_id", "temporal-filter-skill"},
            {"extension_display_name", "Workflow Manager"}
        }
    };
}

QJsonObject WorkflowManager::readResource(const QString& resourceUri) const
{
    const QString trimmedUri = resourceUri.trimmed();
    if(trimmedUri != activeGraphResourceUri()) {
        return QJsonObject{
            {"tool_name", "resources/read"},
            {"status", "error"},
            {"message", QString("Workflow manager does not expose resource `%1`.").arg(trimmedUri)}
        };
    }

    return QJsonObject{
        {"tool_name", "resources/read"},
        {"status", "ok"},
        {"uri", trimmedUri},
        {"mime_type", "application/json"},
        {"message", "Serialized active workflow graph."},
        {"source_file", m_activeGraphSourceFile},
        {"contents", QString::fromUtf8(QJsonDocument(m_activeGraph.toJson()).toJson(QJsonDocument::Indented))},
        {"graph", m_activeGraph.toJson()}
    };
}

const WorkflowGraph& WorkflowManager::activeGraph() const
{
    return m_activeGraph;
}

QString WorkflowManager::activeGraphSourceFile() const
{
    return m_activeGraphSourceFile;
}

void WorkflowManager::setActiveGraph(const WorkflowGraph& workflowGraph)
{
    workflowGraph.validateReferences();
    workflowGraph.topologicalSort();
    m_activeGraph = workflowGraph;
    emit activeGraphChanged(m_activeGraph.toJson());
}

void WorkflowManager::loadAnalysisFile(const QString& filePath)
{
    const WorkflowGraph workflowGraph = m_pipelineParser.parseFile(filePath);
    m_activeGraphSourceFile = filePath;
    setActiveGraph(workflowGraph);
    executePendingNodes();
}

void WorkflowManager::saveAnalysisFile(const QString& filePath)
{
    if(m_activeGraph.nodes().isEmpty() && m_activeGraph.resources().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("No active workflow graph is available to save."));
    }

    const QString resolvedFilePath = filePath.trimmed().isEmpty()
        ? m_activeGraphSourceFile.trimmed()
        : QFileInfo(filePath.trimmed()).absoluteFilePath();
    if(resolvedFilePath.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow save requires a target `.mne` file path."));
    }

    const QJsonDocument document(m_activeGraph.toDeclarativeJson());
    QSaveFile outputFile(resolvedFilePath);
    if(!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw WorkflowValidationError(QStringLiteral("Failed to open workflow file `%1` for writing.")
                                          .arg(resolvedFilePath));
    }

    if(outputFile.write(document.toJson(QJsonDocument::Indented)) < 0) {
        throw WorkflowValidationError(QStringLiteral("Failed to write workflow file `%1`.").arg(resolvedFilePath));
    }

    if(!outputFile.commit()) {
        throw WorkflowValidationError(QStringLiteral("Failed to finalize workflow save for `%1`.").arg(resolvedFilePath));
    }

    m_activeGraphSourceFile = resolvedFilePath;
}

QString WorkflowManager::activeGraphResourceUri() const
{
    return QString::fromLatin1(kActiveGraphResourceUri);
}

QJsonObject WorkflowManager::appendNodeAndExecute(const QString& toolName, const QJsonObject& arguments)
{
    const OperatorRegistration registration = registrationForTool(toolName);

    WorkflowGraph candidateGraph = m_activeGraph;
    const WorkflowNode node = buildNodeFromToolArguments(registration, arguments, candidateGraph);
    candidateGraph.addNode(node);
    candidateGraph.validateReferences();
    candidateGraph.topologicalSort();

    m_activeGraph = candidateGraph;
    emit activeGraphChanged(m_activeGraph.toJson());

    executePendingNodes();

    const WorkflowNode& completedNode = m_activeGraph.node(node.uid);
    return QJsonObject{
        {"tool_name", registration.toolName},
        {"status", completedNode.executionStatus},
        {"message", completedNode.lastResult.value(QStringLiteral("message"))
                        .toString(QStringLiteral("Executed workflow node `%1`.").arg(completedNode.uid))},
        {"node_uid", completedNode.uid},
        {"skill_id", completedNode.skillId},
        {"source_file", m_activeGraphSourceFile},
        {"graph_resource_uri", activeGraphResourceUri()},
        {"outputs", completedNode.resolvedOutputs},
        {"graph", m_activeGraph.toJson()}
    };
}

QJsonObject WorkflowManager::translateOperatorToToolDefinition(const OperatorRegistration& registration) const
{
    const QJsonObject inputsSchema = registration.definition.value(QStringLiteral("inputs_schema")).toObject();
    const QJsonObject parametersSchema = registration.definition.value(QStringLiteral("parameters_schema")).toObject();
    const QJsonObject outputsSchema = registration.definition.value(QStringLiteral("outputs_schema")).toObject();

    QJsonObject properties{
        {"uid", stringSchema("Node UID",
                             QStringLiteral("Optional workflow node identifier. If omitted, the workflow manager generates one."))}
    };

    QJsonArray required;

    const auto appendSchemaSection = [&properties, &required](const QJsonObject& schema,
                                                              const QString& sectionName,
                                                              bool includeRequired) {
        const QJsonObject sectionProperties = schema.value(QStringLiteral("properties")).toObject();
        const QStringList sectionRequired = requiredFieldNames(schema);
        for(auto it = sectionProperties.constBegin(); it != sectionProperties.constEnd(); ++it) {
            properties.insert(it.key(), propertySchemaWithSection(it.value().toObject(), sectionName));
        }

        if(includeRequired) {
            for(const QString& requiredField : sectionRequired) {
                if(!required.contains(requiredField)) {
                    required.append(requiredField);
                }
            }
        }
    };

    appendSchemaSection(inputsSchema, QStringLiteral("inputs"), true);
    appendSchemaSection(parametersSchema, QStringLiteral("parameters"), true);
    appendSchemaSection(outputsSchema, QStringLiteral("outputs"), false);

    QJsonObject resultSchema = registration.definition.value(QStringLiteral("result_schema")).toObject();
    if(resultSchema.isEmpty()) {
        resultSchema = objectSchema(QJsonObject{
            {"status", stringSchema("Status")},
            {"node_uid", stringSchema("Node UID")},
            {"skill_id", stringSchema("Skill ID")},
            {"graph_resource_uri", stringSchema("Graph Resource URI")}
        }, QJsonArray{"status", "node_uid", "skill_id", "graph_resource_uri"});
    }

    QJsonObject tool{
        {"name", registration.toolName},
        {"display_name", registration.displayName},
        {"description", registration.description},
        {"input_schema", objectSchema(properties, required)},
        {"result_schema", resultSchema},
        {"skill_id", registration.skillId},
        {"capability_id", QStringLiteral("workflow_skill:%1").arg(registration.skillId)},
        {"capability_kind", QStringLiteral("workflow_skill")},
        {"capability_aliases", QJsonArray::fromStringList(QStringList()
             << registration.skillId
             << registration.toolName)},
        {"workflow_operator", true},
        {"graph_mutation", "append_node"}
    };

    if(!registration.extensionId.isEmpty()) {
        tool.insert("extension_id", registration.extensionId);
    }
    if(!registration.extensionDisplayName.isEmpty()) {
        tool.insert("extension_display_name", registration.extensionDisplayName);
    }

    return annotatePlannerMetadata(tool,
                                   false,
                                   QStringLiteral("medium"),
                                   QStringLiteral("Mutates the active workflow graph by appending a new workflow node."),
                                   QStringLiteral("suggestion_only"),
                                   QStringLiteral("Workflow graph edits should be proposed explicitly rather than auto-executed."),
                                   false);
}

WorkflowNode WorkflowManager::buildNodeFromToolArguments(const OperatorRegistration& registration,
                                                         const QJsonObject& arguments,
                                                         const WorkflowGraph& candidateGraph) const
{
    const QJsonObject inputSchema = translateOperatorToToolDefinition(registration).value(QStringLiteral("input_schema")).toObject();
    const QJsonObject properties = inputSchema.value(QStringLiteral("properties")).toObject();
    const QStringList required = requiredFieldNames(inputSchema);

    WorkflowNode node;
    node.skillId = registration.skillId;

    for(auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        const QString propertyName = it.key();
        if(propertyName == QLatin1String("uid")) {
            continue;
        }

        const QJsonObject propertySchema = it.value().toObject();
        const QString sectionName = propertySchema.value(QStringLiteral("x_workflow_section")).toString();
        const QJsonValue value = arguments.value(propertyName);

        if(!value.isUndefined() && !value.isNull()) {
            if(sectionName == QLatin1String("inputs")) {
                node.inputs.insert(propertyName, value);
            } else if(sectionName == QLatin1String("parameters")) {
                node.parameters.insert(propertyName, value);
            } else if(sectionName == QLatin1String("outputs")) {
                node.outputs.insert(propertyName, value);
            }
        }

        if(required.contains(propertyName) && value.isUndefined()) {
            throw WorkflowValidationError(QStringLiteral("Tool `%1` is missing required argument `%2`.")
                                              .arg(registration.toolName, propertyName));
        }
    }

    const QString preferredUid = arguments.value(QStringLiteral("uid")).toString().trimmed();
    node.uid = ensureUniqueNodeUid(preferredUid.isEmpty() ? sanitizeIdentifier(registration.toolName) : preferredUid,
                                   candidateGraph);

    const QJsonObject outputsSchema = registration.definition.value(QStringLiteral("outputs_schema")).toObject().value(QStringLiteral("properties")).toObject();
    for(auto it = outputsSchema.constBegin(); it != outputsSchema.constEnd(); ++it) {
        const QString outputRole = it.key();
        const QString providedOutputUid = valueToUid(node.outputs.value(outputRole));
        const QString outputUid = providedOutputUid.isEmpty()
            ? defaultOutputUid(node.uid, outputRole, candidateGraph)
            : ensureUniqueOutputUid(providedOutputUid, candidateGraph);
        node.outputs.insert(outputRole, outputUid);
    }

    return node;
}

void WorkflowManager::executePendingNodes()
{
    const QVector<QString> executionOrder = m_activeGraph.topologicalSort();
    for(const QString& nodeUid : executionOrder) {
        WorkflowNode& workflowNode = m_activeGraph.node(nodeUid);
        if(workflowNode.executionStatus == QLatin1String("completed")) {
            continue;
        }

        const OperatorRegistration registration = registrationForSkill(workflowNode.skillId);
        if(!registration.skillOperator) {
            workflowNode.executionStatus = QStringLiteral("failed");
            workflowNode.lastResult = QJsonObject{
                {"status", "failed"},
                {"message", QString("No registered skill operator found for `%1`.").arg(workflowNode.skillId)}
            };
            emit activeGraphChanged(m_activeGraph.toJson());
            throw WorkflowValidationError(workflowNode.lastResult.value(QStringLiteral("message")).toString());
        }

        workflowNode.resolvedInputs = resolvedInputsForNode(workflowNode);
        workflowNode.executionStatus = QStringLiteral("running");

        const QJsonObject executionResult = registration.skillOperator->executeSkill(workflowNode);
        const QString executionStatus = executionResult.value(QStringLiteral("status")).toString(QStringLiteral("completed"));
        workflowNode.lastResult = executionResult;

        if(executionStatus == QLatin1String("error") || executionStatus == QLatin1String("failed")) {
            workflowNode.executionStatus = QStringLiteral("failed");
            emit activeGraphChanged(m_activeGraph.toJson());
            throw WorkflowValidationError(executionResult.value(QStringLiteral("message"))
                                              .toString(QStringLiteral("Execution failed for workflow node `%1`.").arg(nodeUid)));
        }

        const QJsonObject outputUris = executionResult.value(QStringLiteral("outputs")).toObject();
        for(auto it = workflowNode.outputs.constBegin(); it != workflowNode.outputs.constEnd(); ++it) {
            const QString outputRole = it.key();
            const QString outputUid = it.value().toString().trimmed();
            const QString outputUri = outputUris.value(outputRole).toString(defaultOutputUri(outputUid));
            const WorkflowResource resource = materializeOutputResource(registration, workflowNode, outputRole, outputUri);
            m_activeGraph.upsertResource(resource);
            workflowNode.resolvedOutputs.insert(outputRole, resource.toJson());
        }

        workflowNode.executionStatus = QStringLiteral("completed");
        emit activeGraphChanged(m_activeGraph.toJson());
    }
}

QJsonObject WorkflowManager::resolvedInputsForNode(const WorkflowNode& node) const
{
    QJsonObject resolvedInputs;
    for(auto it = node.inputs.constBegin(); it != node.inputs.constEnd(); ++it) {
        const QString inputUid = valueToUid(it.value());
        if(inputUid.isEmpty()) {
            continue;
        }

        const WorkflowResource& resource = m_activeGraph.resource(inputUid);
        resolvedInputs.insert(it.key(), resource.toJson());
    }

    return resolvedInputs;
}

WorkflowResource WorkflowManager::materializeOutputResource(const OperatorRegistration& registration,
                                                            const WorkflowNode& node,
                                                            const QString& outputRole,
                                                            const QString& outputUri) const
{
    const QJsonObject outputSchema = registration.definition.value(QStringLiteral("outputs_schema"))
                                         .toObject()
                                         .value(QStringLiteral("properties"))
                                         .toObject()
                                         .value(outputRole)
                                         .toObject();

    WorkflowResource resource;
    resource.uid = node.outputs.value(outputRole).toString().trimmed();
    resource.type = outputSchema.value(QStringLiteral("resource_type")).toString(QStringLiteral("workflow_artifact"));
    resource.uri = outputUri;
    resource.metadata = QJsonObject{
        {"producer_node_uid", node.uid},
        {"producer_skill_id", node.skillId},
        {"output_role", outputRole}
    };

    return resource;
}

QString WorkflowManager::ensureUniqueNodeUid(const QString& preferredUid, const WorkflowGraph& graph) const
{
    QString candidate = sanitizeIdentifier(preferredUid);
    if(candidate.isEmpty()) {
        candidate = QStringLiteral("workflow_node");
    }

    QString uniqueCandidate = candidate;
    int suffix = 1;
    while(graph.hasNode(uniqueCandidate)) {
        uniqueCandidate = QStringLiteral("%1_%2").arg(candidate).arg(++suffix);
    }

    return uniqueCandidate;
}

QString WorkflowManager::ensureUniqueOutputUid(const QString& preferredUid, const WorkflowGraph& graph) const
{
    const QString candidate = preferredUid.trimmed();
    if(candidate.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow output uid cannot be empty."));
    }

    if(graph.hasArtifact(candidate)) {
        throw WorkflowValidationError(QStringLiteral("Workflow output uid `%1` already exists in the active graph.")
                                          .arg(candidate));
    }

    return candidate;
}

QString WorkflowManager::defaultOutputUid(const QString& nodeUid,
                                          const QString& outputRole,
                                          const WorkflowGraph& graph) const
{
    QString candidateBase = QStringLiteral("%1_%2").arg(sanitizeIdentifier(nodeUid),
                                                        sanitizeIdentifier(outputRole));
    if(candidateBase.isEmpty()) {
        candidateBase = QStringLiteral("workflow_output");
    }

    QString candidate = candidateBase;
    int suffix = 1;
    while(graph.hasArtifact(candidate)) {
        candidate = QStringLiteral("%1_%2").arg(candidateBase).arg(++suffix);
    }

    return candidate;
}

QString WorkflowManager::defaultOutputUri(const QString& outputUid) const
{
    return QStringLiteral("mne://workspace/%1").arg(outputUid.trimmed());
}

QString WorkflowManager::sanitizeIdentifier(const QString& text) const
{
    QString sanitized = text.trimmed();
    sanitized.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]+")), QStringLiteral("_"));
    sanitized.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    sanitized.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return sanitized;
}

WorkflowManager::OperatorRegistration WorkflowManager::registrationForTool(const QString& toolName) const
{
    const QString trimmedToolName = toolName.trimmed();
    const auto it = m_registrationsByToolName.constFind(trimmedToolName);
    if(it == m_registrationsByToolName.constEnd()) {
        throw WorkflowValidationError(QStringLiteral("No registered workflow tool found for `%1`.").arg(trimmedToolName));
    }

    return it.value();
}

WorkflowManager::OperatorRegistration WorkflowManager::registrationForSkill(const QString& skillId) const
{
    const QString trimmedSkillId = skillId.trimmed();
    const auto it = m_registrationsBySkillId.constFind(trimmedSkillId);
    if(it == m_registrationsBySkillId.constEnd()) {
        throw WorkflowValidationError(QStringLiteral("No registered skill operator found for `%1`.").arg(trimmedSkillId));
    }

    return it.value();
}
