//=============================================================================================================
/**
 * @file     workflowgraph.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the declarative workflow DAG model and dependency resolver.
 */

#include "workflowgraph.h"

#include <QJsonArray>

#include <algorithm>
#include <array>
#include <ranges>
#include <unordered_map>
#include <vector>

using namespace MNEANALYZESTUDIO;

namespace
{

QString jsonValueToUid(const QJsonValue& value)
{
    return value.toString().trimmed();
}

bool isMaterializedOutputResource(const WorkflowResource& resource)
{
    return !resource.metadata.value(QStringLiteral("producer_node_uid")).toString().trimmed().isEmpty();
}

std::vector<QString> sortedKeys(const QHash<QString, int>& map)
{
    std::vector<QString> keys;
    keys.reserve(static_cast<std::size_t>(map.size()));
    for(auto it = map.constBegin(); it != map.constEnd(); ++it) {
        keys.push_back(it.key());
    }

    std::ranges::sort(keys);
    return keys;
}

std::vector<QString> sortedVector(const QVector<QString>& values)
{
    std::vector<QString> sortedValues;
    sortedValues.reserve(static_cast<std::size_t>(values.size()));
    for(const QString& value : values) {
        sortedValues.push_back(value);
    }

    std::ranges::sort(sortedValues);
    return sortedValues;
}

}

WorkflowValidationError::WorkflowValidationError(const QString& message)
: std::runtime_error(message.toStdString())
{
}

QJsonObject WorkflowResource::toJson() const
{
    QJsonObject object{
        {"uid", uid},
        {"type", type},
        {"uri", uri}
    };

    if(!metadata.isEmpty()) {
        object.insert("metadata", metadata);
    }

    return object;
}

QJsonObject WorkflowNode::toJson(bool includeRuntime) const
{
    QJsonObject object{
        {"uid", uid},
        {"skill_id", skillId},
        {"inputs", inputs},
        {"parameters", parameters},
        {"outputs", outputs}
    };

    if(!label.trimmed().isEmpty()) {
        object.insert("label", label);
    }
    if(!stage.trimmed().isEmpty()) {
        object.insert("stage", stage);
    }
    if(!description.trimmed().isEmpty()) {
        object.insert("description", description);
    }

    if(includeRuntime) {
        QJsonObject runtime;
        if(!executionStatus.trimmed().isEmpty() && executionStatus != QLatin1String("pending")) {
            runtime.insert("status", executionStatus);
        }
        if(!resolvedInputs.isEmpty()) {
            runtime.insert("resolved_inputs", resolvedInputs);
        }
        if(!resolvedOutputs.isEmpty()) {
            runtime.insert("resolved_outputs", resolvedOutputs);
        }
        if(!lastResult.isEmpty()) {
            runtime.insert("last_result", lastResult);
        }
        if(!runtime.isEmpty()) {
            object.insert("runtime", runtime);
        }
    }

    return object;
}

void WorkflowGraph::clear()
{
    m_resources.clear();
    m_nodes.clear();
    m_resourceIndexByUid.clear();
    m_nodeIndexByUid.clear();
    m_outputProducerByUid.clear();
}

void WorkflowGraph::validateResource(const WorkflowResource& resource) const
{
    if(resource.uid.trimmed().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow resource is missing a uid."));
    }

    if(resource.type.trimmed().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow resource `%1` is missing a type.").arg(resource.uid));
    }

    if(resource.uri.trimmed().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow resource `%1` is missing a uri.").arg(resource.uid));
    }
}

void WorkflowGraph::validateNodeShape(const WorkflowNode& node) const
{
    QStringList seenOutputUids;

    if(node.uid.trimmed().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow node is missing a uid."));
    }

    if(node.skillId.trimmed().isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow node `%1` is missing a skill_id.").arg(node.uid));
    }

    if(node.outputs.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("Workflow node `%1` must declare at least one output.").arg(node.uid));
    }

    for(auto it = node.outputs.constBegin(); it != node.outputs.constEnd(); ++it) {
        const QString outputUid = it.value().toString().trimmed();
        if(outputUid.isEmpty()) {
            throw WorkflowValidationError(QStringLiteral("Workflow node `%1` has an empty output uid for role `%2`.")
                                              .arg(node.uid, it.key()));
        }

        if(seenOutputUids.contains(outputUid)) {
            throw WorkflowValidationError(QStringLiteral("Workflow node `%1` reuses output uid `%2` across multiple roles.")
                                              .arg(node.uid, outputUid));
        }

        seenOutputUids.append(outputUid);
    }
}

void WorkflowGraph::addResource(const WorkflowResource& resource)
{
    validateResource(resource);

    if(m_resourceIndexByUid.contains(resource.uid)) {
        throw WorkflowValidationError(QStringLiteral("Duplicate workflow resource uid `%1`.").arg(resource.uid));
    }

    if(m_outputProducerByUid.contains(resource.uid)) {
        const QString declaredProducerNodeUid = m_outputProducerByUid.value(resource.uid).trimmed();
        const QString materializedProducerNodeUid = resource.metadata.value(QStringLiteral("producer_node_uid")).toString().trimmed();
        if(declaredProducerNodeUid.isEmpty() || materializedProducerNodeUid != declaredProducerNodeUid) {
            throw WorkflowValidationError(QStringLiteral("Resource uid `%1` collides with an existing workflow output uid.")
                                              .arg(resource.uid));
        }
    }

    m_resourceIndexByUid.insert(resource.uid, m_resources.size());
    m_resources.append(resource);
}

void WorkflowGraph::upsertResource(const WorkflowResource& resource)
{
    validateResource(resource);

    if(m_resourceIndexByUid.contains(resource.uid)) {
        m_resources[m_resourceIndexByUid.value(resource.uid)] = resource;
        return;
    }

    addResource(resource);
}

void WorkflowGraph::addNode(const WorkflowNode& node)
{
    validateNodeShape(node);

    if(m_nodeIndexByUid.contains(node.uid)) {
        throw WorkflowValidationError(QStringLiteral("Duplicate workflow node uid `%1`.").arg(node.uid));
    }

    for(auto it = node.outputs.constBegin(); it != node.outputs.constEnd(); ++it) {
        const QString outputUid = it.value().toString().trimmed();
        if(m_resourceIndexByUid.contains(outputUid) || m_outputProducerByUid.contains(outputUid)) {
            throw WorkflowValidationError(QStringLiteral("Workflow output uid `%1` from node `%2` is already defined.")
                                              .arg(outputUid, node.uid));
        }
    }

    m_nodeIndexByUid.insert(node.uid, m_nodes.size());
    m_nodes.append(node);

    for(auto it = node.outputs.constBegin(); it != node.outputs.constEnd(); ++it) {
        m_outputProducerByUid.insert(it.value().toString().trimmed(), node.uid);
    }
}

bool WorkflowGraph::hasResource(const QString& uid) const
{
    return m_resourceIndexByUid.contains(uid.trimmed());
}

bool WorkflowGraph::hasNode(const QString& uid) const
{
    return m_nodeIndexByUid.contains(uid.trimmed());
}

bool WorkflowGraph::hasArtifact(const QString& uid) const
{
    const QString trimmedUid = uid.trimmed();
    return hasResource(trimmedUid) || m_outputProducerByUid.contains(trimmedUid);
}

const WorkflowResource& WorkflowGraph::resource(const QString& uid) const
{
    const QString trimmedUid = uid.trimmed();
    const auto it = m_resourceIndexByUid.constFind(trimmedUid);
    if(it == m_resourceIndexByUid.constEnd()) {
        throw WorkflowValidationError(QStringLiteral("Unknown workflow resource `%1`.").arg(trimmedUid));
    }

    return m_resources.at(it.value());
}

const WorkflowNode& WorkflowGraph::node(const QString& uid) const
{
    const QString trimmedUid = uid.trimmed();
    const auto it = m_nodeIndexByUid.constFind(trimmedUid);
    if(it == m_nodeIndexByUid.constEnd()) {
        throw WorkflowValidationError(QStringLiteral("Unknown workflow node `%1`.").arg(trimmedUid));
    }

    return m_nodes.at(it.value());
}

WorkflowNode& WorkflowGraph::node(const QString& uid)
{
    const QString trimmedUid = uid.trimmed();
    const auto it = m_nodeIndexByUid.constFind(trimmedUid);
    if(it == m_nodeIndexByUid.constEnd()) {
        throw WorkflowValidationError(QStringLiteral("Unknown workflow node `%1`.").arg(trimmedUid));
    }

    return m_nodes[it.value()];
}

QVector<WorkflowResource> WorkflowGraph::resources() const
{
    return m_resources;
}

QVector<WorkflowNode> WorkflowGraph::nodes() const
{
    return m_nodes;
}

QString WorkflowGraph::producerForArtifact(const QString& artifactUid) const
{
    return m_outputProducerByUid.value(artifactUid.trimmed());
}

QVector<QString> WorkflowGraph::dependencyUids(const QString& nodeUid) const
{
    const WorkflowNode& workflowNode = node(nodeUid);

    QVector<QString> dependencies;
    for(auto it = workflowNode.inputs.constBegin(); it != workflowNode.inputs.constEnd(); ++it) {
        const QString inputUid = jsonValueToUid(it.value());
        if(inputUid.isEmpty()) {
            continue;
        }

        const QString producerUid = producerForArtifact(inputUid);
        if(!producerUid.isEmpty() && !dependencies.contains(producerUid)) {
            dependencies.append(producerUid);
        }
    }

    return dependencies;
}

void WorkflowGraph::validateReferences() const
{
    for(const WorkflowNode& workflowNode : m_nodes) {
        for(auto it = workflowNode.inputs.constBegin(); it != workflowNode.inputs.constEnd(); ++it) {
            const QString inputUid = jsonValueToUid(it.value());
            if(inputUid.isEmpty()) {
                throw WorkflowValidationError(QStringLiteral("Workflow node `%1` has an empty input uid for role `%2`.")
                                                  .arg(workflowNode.uid, it.key()));
            }

            if(!hasArtifact(inputUid)) {
                throw WorkflowValidationError(QStringLiteral("Workflow node `%1` requests input `%2` for role `%3`, but no resource or output defines that uid.")
                                                  .arg(workflowNode.uid, inputUid, it.key()));
            }
        }
    }
}

QVector<QString> WorkflowGraph::topologicalSort() const
{
    validateReferences();

    enum class VisitState : unsigned char {
        Unvisited,
        Visiting,
        Visited
    };

    std::unordered_map<std::string, VisitState> visitState;
    visitState.reserve(static_cast<std::size_t>(m_nodeIndexByUid.size()));

    QVector<QString> orderedNodeUids;
    orderedNodeUids.reserve(m_nodes.size());

    const auto visitNode = [this, &visitState, &orderedNodeUids](const auto& self, const QString& nodeUid) -> void {
        const std::string key = nodeUid.toStdString();
        const VisitState currentState = visitState[key];

        if(currentState == VisitState::Visited) {
            return;
        }

        if(currentState == VisitState::Visiting) {
            throw WorkflowValidationError(QStringLiteral("Cycle detected while resolving workflow node `%1`.").arg(nodeUid));
        }

        visitState[key] = VisitState::Visiting;

        const std::vector<QString> sortedDependencies = sortedVector(dependencyUids(nodeUid));
        for(const QString& dependencyUid : sortedDependencies) {
            self(self, dependencyUid);
        }

        visitState[key] = VisitState::Visited;
        orderedNodeUids.append(nodeUid);
    };

    const std::vector<QString> sortedNodeUids = sortedKeys(m_nodeIndexByUid);
    for(const QString& nodeUid : sortedNodeUids) {
        visitNode(visitNode, nodeUid);
    }

    return orderedNodeUids;
}

QJsonObject WorkflowGraph::toJson() const
{
    QJsonArray resourceArray;
    for(const WorkflowResource& resourceEntry : m_resources) {
        resourceArray.append(resourceEntry.toJson());
    }

    QJsonArray pipelineArray;
    for(const WorkflowNode& nodeEntry : m_nodes) {
        pipelineArray.append(nodeEntry.toJson());
    }

    return QJsonObject{
        {"resources", resourceArray},
        {"pipeline", pipelineArray}
    };
}

QJsonObject WorkflowGraph::toDeclarativeJson() const
{
    QJsonArray resourceArray;
    for(const WorkflowResource& resourceEntry : m_resources) {
        if(!isMaterializedOutputResource(resourceEntry)) {
            resourceArray.append(resourceEntry.toJson());
        }
    }

    QJsonArray pipelineArray;
    for(const WorkflowNode& nodeEntry : m_nodes) {
        pipelineArray.append(nodeEntry.toJson(false));
    }

    return QJsonObject{
        {"resources", resourceArray},
        {"pipeline", pipelineArray}
    };
}
