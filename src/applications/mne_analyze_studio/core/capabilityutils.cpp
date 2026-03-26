//=============================================================================================================
/**
 * @file     capabilityutils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements shared helpers for normalizing Studio tool and pipeline capabilities.
 */

#include "capabilityutils.h"

#include <QRegularExpression>

using namespace MNEANALYZESTUDIO;

namespace
{

const char* kPipelineRunAliasPrefix = "studio.pipeline.run::";

}

QString MNEANALYZESTUDIO::pipelineRunAliasToolName(const QString& pipelineId)
{
    const QString trimmedPipelineId = pipelineId.trimmed();
    return trimmedPipelineId.isEmpty()
        ? QStringLiteral("studio.pipeline.run")
        : QStringLiteral("%1%2").arg(QString::fromLatin1(kPipelineRunAliasPrefix), trimmedPipelineId);
}

QString MNEANALYZESTUDIO::pipelineIdFromPipelineRunAliasToolName(const QString& toolName)
{
    const QString trimmedToolName = toolName.trimmed();
    if(!trimmedToolName.startsWith(QString::fromLatin1(kPipelineRunAliasPrefix))) {
        return QString();
    }

    return trimmedToolName.mid(QString::fromLatin1(kPipelineRunAliasPrefix).size()).trimmed();
}

QString MNEANALYZESTUDIO::normalizedPlannerToolName(const QString& toolName)
{
    const QString pipelineId = pipelineIdFromPipelineRunAliasToolName(toolName);
    return pipelineId.isEmpty() ? toolName.trimmed() : QStringLiteral("studio.pipeline.run");
}

QJsonArray MNEANALYZESTUDIO::mergeCapabilityAliases(const QJsonArray& existingAliases,
                                                    const QStringList& additionalAliases)
{
    QStringList mergedAliases;

    for(const QJsonValue& value : existingAliases) {
        const QString alias = value.toString().trimmed();
        if(!alias.isEmpty() && !mergedAliases.contains(alias)) {
            mergedAliases.append(alias);
        }
    }

    for(const QString& alias : additionalAliases) {
        const QString trimmedAlias = alias.trimmed();
        if(!trimmedAlias.isEmpty() && !mergedAliases.contains(trimmedAlias)) {
            mergedAliases.append(trimmedAlias);
        }
    }

    return QJsonArray::fromStringList(mergedAliases);
}

QJsonObject MNEANALYZESTUDIO::annotatePlannerMetadata(const QJsonObject& rawTool,
                                                      bool plannerSafeDefault,
                                                      const QString& riskLevel,
                                                      const QString& safetyReason,
                                                      const QString& executionDefault,
                                                      const QString& executionReason,
                                                      bool readOnly,
                                                      const QJsonArray& requiredContext,
                                                      const QJsonObject& extraMetadata)
{
    QJsonObject tool = rawTool;
    tool.insert(QStringLiteral("planner_safe_default"), plannerSafeDefault);
    tool.insert(QStringLiteral("planner_risk_level_default"), riskLevel.trimmed());
    tool.insert(QStringLiteral("planner_safety_reason"), safetyReason.trimmed());
    tool.insert(QStringLiteral("planner_execution_default"), executionDefault.trimmed());
    tool.insert(QStringLiteral("planner_execution_reason"), executionReason.trimmed());

    if(readOnly) {
        tool.insert(QStringLiteral("planner_read_only"), true);
    }
    if(!requiredContext.isEmpty()) {
        tool.insert(QStringLiteral("planner_required_context"), requiredContext);
    }

    for(auto it = extraMetadata.constBegin(); it != extraMetadata.constEnd(); ++it) {
        tool.insert(it.key(), it.value());
    }

    return tool;
}

QJsonObject MNEANALYZESTUDIO::annotateCapabilityMetadata(const QJsonObject& rawTool)
{
    QJsonObject tool = rawTool;
    const QString toolName = tool.value("name").toString().trimmed();
    const QString skillId = tool.value("skill_id").toString().trimmed();
    const QString pipelineId = tool.value("pipeline_id").toString(pipelineIdFromPipelineRunAliasToolName(toolName)).trimmed();

    QString capabilityKind = tool.value("capability_kind").toString().trimmed();
    if(capabilityKind.isEmpty()) {
        if(!pipelineId.isEmpty()) {
            capabilityKind = QStringLiteral("analysis_pipeline");
        } else if(tool.value("workflow_operator").toBool(false)) {
            capabilityKind = QStringLiteral("workflow_skill");
        } else if(toolName.startsWith(QLatin1String("neurokernel."))) {
            capabilityKind = QStringLiteral("neurokernel_tool");
        } else if(toolName.startsWith(QLatin1String("settings."))) {
            capabilityKind = QStringLiteral("extension_setting");
        } else if(toolName.startsWith(QLatin1String("studio."))
                  || toolName.startsWith(QLatin1String("view.raw."))
                  || toolName.startsWith(QLatin1String("view.hosted."))) {
            capabilityKind = QStringLiteral("studio_tool");
        } else if(!tool.value("extension_id").toString().trimmed().isEmpty()) {
            capabilityKind = QStringLiteral("extension_tool");
        } else {
            capabilityKind = QStringLiteral("tool");
        }
    }
    tool.insert("capability_kind", capabilityKind);

    if(!pipelineId.isEmpty()) {
        tool.insert("pipeline_id", pipelineId);
        if(tool.value("pipeline_run_tool").toString().trimmed().isEmpty()) {
            tool.insert("pipeline_run_tool", QStringLiteral("studio.pipeline.run"));
        }
    }

    QString capabilityId = tool.value("capability_id").toString().trimmed();
    if(capabilityId.isEmpty()) {
        if(!pipelineId.isEmpty()) {
            capabilityId = QStringLiteral("pipeline:%1").arg(pipelineId);
        } else if(!skillId.isEmpty()) {
            capabilityId = QStringLiteral("workflow_skill:%1").arg(skillId);
        } else if(!toolName.isEmpty()) {
            capabilityId = QStringLiteral("tool:%1").arg(toolName);
        }
    }
    if(!capabilityId.isEmpty()) {
        tool.insert("capability_id", capabilityId);
    }

    tool.insert("capability_aliases",
                mergeCapabilityAliases(tool.value("capability_aliases").toArray(),
                                       QStringList()
                                           << skillId
                                           << toolName
                                           << pipelineId
                                           << (!pipelineId.isEmpty() ? QStringLiteral("studio.pipeline.run") : QString())
                                           << (!pipelineId.isEmpty() ? pipelineRunAliasToolName(pipelineId) : QString())));

    return tool;
}

QStringList MNEANALYZESTUDIO::plannerRequiredContext(const QJsonObject& tool)
{
    QStringList contexts;
    const QJsonArray requiredContext = tool.value(QStringLiteral("planner_required_context")).toArray();
    for(const QJsonValue& value : requiredContext) {
        const QString context = value.toString().trimmed();
        if(!context.isEmpty() && !contexts.contains(context)) {
            contexts.append(context);
        }
    }

    return contexts;
}

QStringList MNEANALYZESTUDIO::extractTemplatePlaceholders(const QString& templateText)
{
    QStringList placeholders;
    QRegularExpressionMatchIterator matchIterator
        = QRegularExpression(QStringLiteral("\\$\\{([^}]+)\\}")).globalMatch(templateText);
    while(matchIterator.hasNext()) {
        const QString placeholder = matchIterator.next().captured(1).trimmed();
        if(!placeholder.isEmpty() && !placeholders.contains(placeholder)) {
            placeholders.append(placeholder);
        }
    }

    return placeholders;
}
