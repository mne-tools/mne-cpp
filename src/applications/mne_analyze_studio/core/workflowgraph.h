//=============================================================================================================
/**
 * @file     workflowgraph.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
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
