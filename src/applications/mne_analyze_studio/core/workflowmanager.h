//=============================================================================================================
/**
 * @file     workflowmanager.h
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
 * @brief    Declares the workflow manager that owns the active DAG and registered skill operators.
 */

#ifndef MNE_ANALYZE_STUDIO_WORKFLOWMANAGER_H
#define MNE_ANALYZE_STUDIO_WORKFLOWMANAGER_H

#include "iskilloperator.h"
#include "pipelineparser.h"
#include "studio_core_global.h"
#include "workflowgraph.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Manages the active declarative workflow graph and skill operator execution.
 */
class STUDIOCORESHARED_EXPORT WorkflowManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkflowManager(QObject* parent = nullptr);

    void registerOperator(ISkillOperator* skillOperator);

    bool canHandleTool(const QString& toolName) const;
    QJsonArray toolDefinitions() const;
    QJsonArray resourceDefinitions() const;
    QJsonObject readResource(const QString& resourceUri) const;

    const WorkflowGraph& activeGraph() const;
    QString activeGraphSourceFile() const;
    void setActiveGraph(const WorkflowGraph& workflowGraph);
    void loadAnalysisFile(const QString& filePath);
    void saveAnalysisFile(const QString& filePath = QString());

    QString activeGraphResourceUri() const;

    QJsonObject appendNodeAndExecute(const QString& toolName, const QJsonObject& arguments);

signals:
    void activeGraphChanged(const QJsonObject& graph);

private:
    struct OperatorRegistration
    {
        QString toolName;
        QString skillId;
        QString displayName;
        QString description;
        QString extensionId;
        QString extensionDisplayName;
        QJsonObject definition;
        ISkillOperator* skillOperator = nullptr;
    };

    QJsonObject translateOperatorToToolDefinition(const OperatorRegistration& registration) const;
    WorkflowNode buildNodeFromToolArguments(const OperatorRegistration& registration,
                                            const QJsonObject& arguments,
                                            const WorkflowGraph& candidateGraph) const;
    void executePendingNodes();
    QJsonObject resolvedInputsForNode(const WorkflowNode& node) const;
    WorkflowResource materializeOutputResource(const OperatorRegistration& registration,
                                               const WorkflowNode& node,
                                               const QString& outputRole,
                                               const QString& outputUri) const;
    QString ensureUniqueNodeUid(const QString& preferredUid, const WorkflowGraph& graph) const;
    QString ensureUniqueOutputUid(const QString& preferredUid, const WorkflowGraph& graph) const;
    QString defaultOutputUid(const QString& nodeUid,
                             const QString& outputRole,
                             const WorkflowGraph& graph) const;
    QString defaultOutputUri(const QString& outputUid) const;
    QString sanitizeIdentifier(const QString& text) const;
    OperatorRegistration registrationForTool(const QString& toolName) const;
    OperatorRegistration registrationForSkill(const QString& skillId) const;

    PipelineParser m_pipelineParser;
    WorkflowGraph m_activeGraph;
    QString m_activeGraphSourceFile;
    QHash<QString, OperatorRegistration> m_registrationsByToolName;
    QHash<QString, OperatorRegistration> m_registrationsBySkillId;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_WORKFLOWMANAGER_H
