//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     workflowmanager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
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
