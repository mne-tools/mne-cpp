//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     skillhostservice.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the isolated skill-host service for MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_SKILLHOSTSERVICE_H
#define MNE_ANALYZE_STUDIO_SKILLHOSTSERVICE_H

#include <mcprouter.h>
#include <viewproviderregistry.h>
#include <workflowmanager.h>

#include <QJsonObject>
#include <QObject>
#include <QHash>
#include <QLocalServer>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Placeholder service object for the studio skill-host process.
 */
class SkillHostService : public QObject
{
    Q_OBJECT

public:
    explicit SkillHostService(QObject* parent = nullptr);

    bool start(const QString& socketName, const QString& extensionsDirectory);

private:
    bool reloadExtensions(const QString& extensionsDirectory, const QStringList& disabledExtensionIds);
    QJsonObject sessionCapabilitiesForProvider(const ViewProviderContribution& provider) const;
    QJsonObject viewCommandResultSchema(const QJsonObject& stateSchema) const;
    QJsonObject workflowLoadToolDefinition() const;
    QJsonObject workflowSaveToolDefinition() const;
    QJsonObject handleResourcesList() const;
    QJsonObject handleResourcesRead(const QJsonObject& params) const;
    QJsonObject handleToolsList() const;
    QJsonObject handleToolCall(const QJsonObject& params);
    QJsonObject handleExtensionsReload(const QJsonObject& params);
    QJsonObject handleViewsOpen(const QJsonObject& params);
    QJsonObject handleViewsList() const;
    QJsonObject handleViewsCommand(const QJsonObject& params);

    McpRouter m_router;
    QLocalServer m_server;
    ViewProviderRegistry m_registry;
    WorkflowManager m_workflowManager;
    QString m_extensionsDirectory;
    QHash<QString, QJsonObject> m_viewSessions;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_SKILLHOSTSERVICE_H
