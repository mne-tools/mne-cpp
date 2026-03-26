//=============================================================================================================
/**
 * @file     capabilityutils.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares shared helpers for normalizing Studio tool and pipeline capabilities.
 */

#ifndef MNE_ANALYZE_STUDIO_CAPABILITYUTILS_H
#define MNE_ANALYZE_STUDIO_CAPABILITYUTILS_H

#include "studio_core_global.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace MNEANALYZESTUDIO
{

STUDIOCORESHARED_EXPORT QString pipelineRunAliasToolName(const QString& pipelineId);
STUDIOCORESHARED_EXPORT QString pipelineIdFromPipelineRunAliasToolName(const QString& toolName);
STUDIOCORESHARED_EXPORT QString normalizedPlannerToolName(const QString& toolName);
STUDIOCORESHARED_EXPORT QJsonArray mergeCapabilityAliases(const QJsonArray& existingAliases,
                                                          const QStringList& additionalAliases);
STUDIOCORESHARED_EXPORT QJsonObject annotatePlannerMetadata(const QJsonObject& rawTool,
                                                            bool plannerSafeDefault,
                                                            const QString& riskLevel,
                                                            const QString& safetyReason,
                                                            const QString& executionDefault,
                                                            const QString& executionReason,
                                                            bool readOnly = false,
                                                            const QJsonArray& requiredContext = QJsonArray(),
                                                            const QJsonObject& extraMetadata = QJsonObject());
STUDIOCORESHARED_EXPORT QJsonObject annotateCapabilityMetadata(const QJsonObject& rawTool);
STUDIOCORESHARED_EXPORT QStringList plannerRequiredContext(const QJsonObject& tool);
STUDIOCORESHARED_EXPORT QStringList extractTemplatePlaceholders(const QString& templateText);

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_CAPABILITYUTILS_H
