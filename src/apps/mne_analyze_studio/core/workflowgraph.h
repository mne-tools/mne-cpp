//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     workflowgraph.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the declarative workflow DAG model used by MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_WORKFLOWGRAPH_H
#define MNE_ANALYZE_STUDIO_WORKFLOWGRAPH_H

#include "studio_core_global.h"

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <stdexcept>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Validation error thrown when a workflow graph is malformed or cyclic.
 */
class STUDIOCORESHARED_EXPORT WorkflowValidationError : public std::runtime_error
{
public:
    explicit WorkflowValidationError(const QString& message);
};

/**
 * @brief Declarative resource record used by .mne workflow graphs.
 */
struct STUDIOCORESHARED_EXPORT WorkflowResource
{
    QString uid;
    QString type;
    QString uri;
    QJsonObject metadata;

    QJsonObject toJson() const;
};

/**
 * @brief Declarative workflow node record used by .mne workflow graphs.
 */
struct STUDIOCORESHARED_EXPORT WorkflowNode
{
    QString uid;
    QString skillId;
    QString label;
    QString stage;
    QString description;
    QJsonObject inputs;
    QJsonObject parameters;
    QJsonObject outputs;

    // Runtime state populated by the workflow manager after dependency resolution.
    QString executionStatus = QStringLiteral("pending");
    QJsonObject resolvedInputs;
    QJsonObject resolvedOutputs;
    QJsonObject lastResult;

    QJsonObject toJson(bool includeRuntime = true) const;
};

/**
 * @brief Owns the declarative DAG and resolves node dependencies.
 */
class STUDIOCORESHARED_EXPORT WorkflowGraph
{
public:
    WorkflowGraph() = default;

    void clear();

    void addResource(const WorkflowResource& resource);
    void upsertResource(const WorkflowResource& resource);
    void addNode(const WorkflowNode& node);

    bool hasResource(const QString& uid) const;
    bool hasNode(const QString& uid) const;
    bool hasArtifact(const QString& uid) const;

    const WorkflowResource& resource(const QString& uid) const;
    const WorkflowNode& node(const QString& uid) const;
    WorkflowNode& node(const QString& uid);

    QVector<WorkflowResource> resources() const;
    QVector<WorkflowNode> nodes() const;

    QString producerForArtifact(const QString& artifactUid) const;
    QVector<QString> dependencyUids(const QString& nodeUid) const;

    void validateReferences() const;
    QVector<QString> topologicalSort() const;

    QJsonObject toJson() const;
    QJsonObject toDeclarativeJson() const;

private:
    void validateResource(const WorkflowResource& resource) const;
    void validateNodeShape(const WorkflowNode& node) const;

    QVector<WorkflowResource> m_resources;
    QVector<WorkflowNode> m_nodes;
    QHash<QString, int> m_resourceIndexByUid;
    QHash<QString, int> m_nodeIndexByUid;
    QHash<QString, QString> m_outputProducerByUid;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_WORKFLOWGRAPH_H
