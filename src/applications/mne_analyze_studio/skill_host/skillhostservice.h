//=============================================================================================================
/**
 * @file     skillhostservice.h
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
