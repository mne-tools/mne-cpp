//=============================================================================================================
/**
 * @file     skillhostservice.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the isolated extension-host service shell.
 */

#include "skillhostservice.h"

#include <jsonrpcmessage.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QFileInfo>
#include <QLocalSocket>
#include <QStringList>
#include <QUuid>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

SkillHostService::SkillHostService(QObject* parent)
: QObject(parent)
, m_router(this)
, m_registry(this)
{
    m_router.registerMethod("resources/list", [this](const QJsonObject&) {
        return handleResourcesList();
    });
    m_router.registerMethod("tools/list", [this](const QJsonObject&) {
        return handleToolsList();
    });
    m_router.registerMethod("tools/call", [this](const QJsonObject& params) {
        return handleToolCall(params);
    });
    m_router.registerMethod("extensions/reload", [this](const QJsonObject& params) {
        return handleExtensionsReload(params);
    });
    m_router.registerMethod("extensions.reload", [this](const QJsonObject& params) {
        return handleExtensionsReload(params);
    });
    m_router.registerMethod("views/open", [this](const QJsonObject& params) {
        return handleViewsOpen(params);
    });
    m_router.registerMethod("views/list", [this](const QJsonObject&) {
        return handleViewsList();
    });
    m_router.registerMethod("views/command", [this](const QJsonObject& params) {
        return handleViewsCommand(params);
    });
}

bool SkillHostService::start(const QString& socketName, const QString& extensionsDirectory)
{
    m_extensionsDirectory = extensionsDirectory;
    reloadExtensions(extensionsDirectory, QStringList());

    QLocalServer::removeServer(socketName);
    if(!m_server.listen(socketName)) {
        return false;
    }

    connect(&m_server, &QLocalServer::newConnection, this, [this]() {
        while(QLocalSocket* socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket]() {
                while(socket->canReadLine()) {
                    const QByteArray payload = socket->readLine();
                    QJsonObject request;
                    QString errorString;
                    QJsonObject response;

                    if(JsonRpcMessage::deserialize(payload, request, errorString)) {
                        response = m_router.route(request);
                    } else {
                        response = JsonRpcMessage::createError(QJsonValue(), -32700, errorString);
                    }

                    socket->write(JsonRpcMessage::serialize(response));
                    socket->flush();
                }
            });
        }
    });

    return true;
}

bool SkillHostService::reloadExtensions(const QString& extensionsDirectory, const QStringList& disabledExtensionIds)
{
    m_registry.loadFromDirectory(extensionsDirectory);
    m_registry.setDisabledExtensionIds(disabledExtensionIds);

    QHash<QString, QJsonObject> retainedSessions;
    for(auto it = m_viewSessions.constBegin(); it != m_viewSessions.constEnd(); ++it) {
        const QJsonObject session = it.value();
        if(m_registry.isExtensionEnabled(session.value("extension_id").toString())) {
            retainedSessions.insert(it.key(), session);
        }
    }
    m_viewSessions = retainedSessions;

    return true;
}

QJsonObject SkillHostService::handleResourcesList() const
{
    QJsonArray resources;
    const QVector<ExtensionManifest> manifests = m_registry.manifests();
    for(const ExtensionManifest& manifest : manifests) {
        resources.append(QJsonObject{
            {"id", manifest.id},
            {"display_name", manifest.displayName},
            {"version", manifest.version},
            {"entry_point", manifest.entryPoint},
            {"root_path", manifest.rootPath},
            {"view_provider_count", static_cast<int>(manifest.viewProviders.size())},
            {"tool_count", static_cast<int>(manifest.tools.size())}
        });
    }

    return QJsonObject{
        {"tool_name", "resources/list"},
        {"message", QString("Extension host discovered %1 extension manifests.").arg(resources.size())},
        {"resources", resources}
    };
}

QJsonObject SkillHostService::handleToolsList() const
{
    const QJsonArray tools = m_registry.toolDefinitions();
    return QJsonObject{
        {"tool_name", "tools/list"},
        {"message", QString("Extension host tools: %1").arg(tools.size())},
        {"tools", tools}
    };
}

QJsonObject SkillHostService::handleToolCall(const QJsonObject& params) const
{
    const QString toolName = params.value("name").toString();
    const QJsonObject arguments = params.value("arguments").toObject();

    if(toolName == "dummy3d.set_opacity") {
        const double opacity = arguments.value("opacity").toDouble(1.0);
        return QJsonObject{
            {"tool_name", toolName},
            {"status", "ok"},
            {"message", QString("Dummy 3D extension opacity set to %1.").arg(QString::number(opacity, 'f', 2))},
            {"opacity", opacity}
        };
    }

    if(toolName == "fiffbrowser.reveal_active_state") {
        Q_UNUSED(arguments)
        return QJsonObject{
            {"tool_name", toolName},
            {"status", "ok"},
            {"message", "FIFF browser extension is ready to expose the active browser session state through the hosted view."}
        };
    }

    return QJsonObject{
        {"tool_name", toolName},
        {"status", "ignored"},
        {"message", QString("No extension-host tool registered for %1.").arg(toolName)}
    };
}

QJsonObject SkillHostService::handleExtensionsReload(const QJsonObject& params)
{
    const QString extensionsDirectory = params.value("extensions_directory").toString().trimmed().isEmpty()
        ? m_extensionsDirectory
        : params.value("extensions_directory").toString().trimmed();

    QStringList disabledExtensionIds;
    const QJsonArray disabledArray = params.value("disabled_extension_ids").toArray();
    for(const QJsonValue& value : disabledArray) {
        const QString extensionId = value.toString().trimmed();
        if(!extensionId.isEmpty()) {
            disabledExtensionIds.append(extensionId);
        }
    }

    QJsonArray invalidatedSessionIds;
    QJsonArray invalidatedSessions;
    for(auto it = m_viewSessions.constBegin(); it != m_viewSessions.constEnd(); ++it) {
        const QJsonObject session = it.value();
        if(disabledExtensionIds.contains(session.value("extension_id").toString())) {
            invalidatedSessionIds.append(it.key());
            invalidatedSessions.append(QJsonObject{
                {"session_id", it.key()},
                {"extension_id", session.value("extension_id").toString()},
                {"provider_id", session.value("provider_id").toString()},
                {"file", session.value("file").toString()},
                {"title", session.value("title").toString()}
            });
        }
    }

    reloadExtensions(extensionsDirectory, disabledExtensionIds);

    return QJsonObject{
        {"tool_name", "extensions/reload"},
        {"status", "ok"},
        {"message", QString("Reloaded extension host registry. Active extensions: %1 | Disabled extensions: %2")
                        .arg(m_registry.manifests().size())
                        .arg(m_registry.disabledExtensionIds().size())},
        {"active_extension_count", static_cast<int>(m_registry.manifests().size())},
        {"disabled_extension_count", static_cast<int>(m_registry.disabledExtensionIds().size())},
        {"invalidated_session_count", invalidatedSessionIds.size()},
        {"invalidated_session_ids", invalidatedSessionIds},
        {"invalidated_sessions", invalidatedSessions}
    };
}

QJsonObject SkillHostService::handleViewsOpen(const QJsonObject& params)
{
    const QString filePath = params.value("file").toString().trimmed();
    const QString providerId = params.value("provider_id").toString().trimmed();
    const QString sceneId = params.value("sceneId").toString().trimmed();

    if(filePath.isEmpty() || providerId.isEmpty()) {
        return QJsonObject{
            {"tool_name", "views/open"},
            {"status", "error"},
            {"message", "Extension view session requires both file and provider_id."}
        };
    }

    QJsonObject descriptor;
    const QVector<ExtensionManifest> manifests = m_registry.manifests();
    for(const ExtensionManifest& manifest : manifests) {
        for(const ViewProviderContribution& provider : manifest.viewProviders) {
            if(provider.id != providerId) {
                continue;
            }

            const QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            const QJsonObject dummy3dStateSchema{
                {"type", "object"},
                {"properties", QJsonObject{
                     {"opacity", QJsonObject{{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
                     {"hemisphere", QJsonObject{{"type", "string"}, {"enum", QJsonArray{"left", "right", "both"}}}},
                     {"camera", QJsonObject{{"type", "string"}}}
                 }},
                {"required", QJsonArray{"opacity", "hemisphere", "camera"}}
            };
            const QJsonObject dummy3dActionResultSchema{
                {"type", "object"},
                {"properties", QJsonObject{
                     {"session_id", QJsonObject{{"type", "string"}}},
                     {"command", QJsonObject{{"type", "string"}}},
                     {"state_before", dummy3dStateSchema},
                     {"state_after", dummy3dStateSchema}
                 }}
            };
            descriptor = QJsonObject{
                {"tool_name", "views/open"},
                {"status", "ok"},
                {"session_id", sessionId},
                {"file", filePath},
                {"provider_id", provider.id},
                {"provider_display_name", provider.displayName},
                {"widget_type", provider.widgetType},
                {"slot", provider.slot},
                {"scene_id", sceneId},
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName},
                {"title", QString("%1").arg(provider.displayName)},
                {"message", QString("Opened hosted view session for %1 via %2.")
                                .arg(filePath, manifest.displayName)},
                {"capabilities", QJsonObject{
                     {"set_opacity", provider.id == "dummy3d.surface_view"},
                     {"focus_left", provider.id == "dummy3d.surface_view"},
                     {"focus_right", provider.id == "dummy3d.surface_view"},
                     {"reset_view", provider.id == "dummy3d.surface_view"}
                 }},
                {"controls", QJsonObject{
                     {"opacity", QJsonObject{
                          {"minimum", 0.0},
                          {"maximum", 1.0},
                          {"step", 0.05},
                          {"value", 0.8}
                      }}
                 }},
                {"actions", QJsonArray{
                     QJsonObject{
                         {"command", "focus_left"},
                         {"label", "Left Hemisphere"},
                         {"description", "Rotate the hosted 3D view toward the left hemisphere."},
                         {"result_schema", dummy3dActionResultSchema}
                     },
                     QJsonObject{
                         {"command", "focus_right"},
                         {"label", "Right Hemisphere"},
                         {"description", "Rotate the hosted 3D view toward the right hemisphere."},
                         {"result_schema", dummy3dActionResultSchema}
                     },
                     QJsonObject{
                         {"command", "reset_view"},
                         {"label", "Reset View"},
                         {"description", "Reset camera and opacity to the default placeholder scene state."},
                         {"result_schema", dummy3dActionResultSchema}
                     }
                 }},
                {"state", QJsonObject{
                     {"opacity", 0.8},
                     {"hemisphere", "both"},
                     {"camera", "default"}
                 }},
                {"state_schema", dummy3dStateSchema}
            };

            if(provider.id == "fiffbrowser.raw_view") {
                descriptor = QJsonObject{
                    {"tool_name", "views/open"},
                    {"status", "ok"},
                    {"session_id", sessionId},
                    {"file", filePath},
                    {"provider_id", provider.id},
                    {"provider_display_name", provider.displayName},
                    {"widget_type", provider.widgetType},
                    {"slot", provider.slot},
                    {"extension_id", manifest.id},
                    {"extension_display_name", manifest.displayName},
                    {"title", QFileInfo(filePath).fileName()},
                    {"message", QString("Opened FIFF browser extension session for %1.").arg(filePath)},
                    {"capabilities", QJsonObject{
                         {"raw_browser_embedded", true}
                     }},
                    {"state", QJsonObject{
                         {"buffer_kind", "fiff"},
                         {"session_role", "signal_browser"}
                     }},
                    {"state_schema", QJsonObject{
                         {"type", "object"},
                         {"properties", QJsonObject{
                              {"buffer_kind", QJsonObject{{"type", "string"}}},
                              {"session_role", QJsonObject{{"type", "string"}}}
                          }},
                         {"required", QJsonArray{"buffer_kind", "session_role"}}
                     }}
                };
            }

            m_viewSessions.insert(sessionId, descriptor);
            return descriptor;
        }
    }

    return QJsonObject{
        {"tool_name", "views/open"},
        {"status", "error"},
        {"message", QString("No registered extension view provider found for %1.").arg(providerId)}
    };
}

QJsonObject SkillHostService::handleViewsList() const
{
    QJsonArray sessions;
    for(auto it = m_viewSessions.constBegin(); it != m_viewSessions.constEnd(); ++it) {
        sessions.append(it.value());
    }

    return QJsonObject{
        {"tool_name", "views/list"},
        {"message", QString("Extension host sessions: %1").arg(sessions.size())},
        {"sessions", sessions}
    };
}

QJsonObject SkillHostService::handleViewsCommand(const QJsonObject& params)
{
    const QString sessionId = params.value("session_id").toString().trimmed();
    const QString commandName = params.value("command").toString().trimmed();
    const QJsonObject arguments = params.value("arguments").toObject();

    if(sessionId.isEmpty() || commandName.isEmpty()) {
        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "error"},
            {"message", "Extension view command requires session_id and command."}
        };
    }

    if(!m_viewSessions.contains(sessionId)) {
        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "error"},
            {"session_id", sessionId},
            {"message", QString("Unknown extension view session %1.").arg(sessionId)}
        };
    }

    QJsonObject descriptor = m_viewSessions.value(sessionId);
    const QJsonObject stateBefore = descriptor.value("state").toObject();
    QJsonObject state = stateBefore;
    QJsonObject controls = descriptor.value("controls").toObject();
    const QJsonObject stateSchema = descriptor.value("state_schema").toObject();

    if(commandName == "set_opacity") {
        const double opacity = std::max(0.0, std::min(1.0, arguments.value("opacity").toDouble(0.8)));
        state.insert("opacity", opacity);
        if(controls.contains("opacity")) {
            QJsonObject opacityControl = controls.value("opacity").toObject();
            opacityControl.insert("value", opacity);
            controls.insert("opacity", opacityControl);
        }
        descriptor.insert("state", state);
        descriptor.insert("controls", controls);
        descriptor.insert("message", QString("Updated extension view opacity to %1.").arg(QString::number(opacity, 'f', 2)));
        m_viewSessions.insert(sessionId, descriptor);

        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "ok"},
            {"session_id", sessionId},
            {"command", commandName},
            {"state_before", stateBefore},
            {"state_after", state},
            {"state", state},
            {"state_schema", stateSchema},
            {"controls", controls},
            {"message", descriptor.value("message").toString()}
        };
    }

    if(commandName == "focus_left") {
        state.insert("hemisphere", "left");
        state.insert("camera", "left_lateral");
        descriptor.insert("state", state);
        descriptor.insert("message", "Rotated hosted extension view to the left hemisphere.");
        m_viewSessions.insert(sessionId, descriptor);
        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "ok"},
            {"session_id", sessionId},
            {"command", commandName},
            {"state_before", stateBefore},
            {"state_after", state},
            {"state", state},
            {"state_schema", stateSchema},
            {"message", descriptor.value("message").toString()}
        };
    }

    if(commandName == "focus_right") {
        state.insert("hemisphere", "right");
        state.insert("camera", "right_lateral");
        descriptor.insert("state", state);
        descriptor.insert("message", "Rotated hosted extension view to the right hemisphere.");
        m_viewSessions.insert(sessionId, descriptor);
        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "ok"},
            {"session_id", sessionId},
            {"command", commandName},
            {"state_before", stateBefore},
            {"state_after", state},
            {"state", state},
            {"state_schema", stateSchema},
            {"message", descriptor.value("message").toString()}
        };
    }

    if(commandName == "reset_view") {
        state.insert("hemisphere", "both");
        state.insert("camera", "default");
        state.insert("opacity", 0.8);
        if(controls.contains("opacity")) {
            QJsonObject opacityControl = controls.value("opacity").toObject();
            opacityControl.insert("value", 0.8);
            controls.insert("opacity", opacityControl);
        }
        descriptor.insert("state", state);
        descriptor.insert("controls", controls);
        descriptor.insert("message", "Reset hosted extension view to the default placeholder scene state.");
        m_viewSessions.insert(sessionId, descriptor);
        return QJsonObject{
            {"tool_name", "views/command"},
            {"status", "ok"},
            {"session_id", sessionId},
            {"command", commandName},
            {"state_before", stateBefore},
            {"state_after", state},
            {"state", state},
            {"state_schema", stateSchema},
            {"controls", controls},
            {"message", descriptor.value("message").toString()}
        };
    }

    return QJsonObject{
        {"tool_name", "views/command"},
        {"status", "ignored"},
        {"session_id", sessionId},
        {"command", commandName},
        {"message", QString("Extension view session %1 does not support command %2.").arg(sessionId, commandName)}
    };
}
