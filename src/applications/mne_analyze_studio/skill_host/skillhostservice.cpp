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

#include <capabilityutils.h>
#include <jsonrpcmessage.h>
#include <temporalfilterskill.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QFileInfo>
#include <QLocalSocket>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QJsonObject stringSchema(const QString& title, const QString& description = QString())
{
    QJsonObject schema{
        {"type", "string"},
        {"title", title}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

}

SkillHostService::SkillHostService(QObject* parent)
: QObject(parent)
, m_router(this)
, m_registry(this)
, m_workflowManager(this)
{
    m_workflowManager.registerOperator(new TemporalFilterSkill(this));

    m_router.registerMethod("resources/list", [this](const QJsonObject&) {
        return handleResourcesList();
    });
    m_router.registerMethod("resources/read", [this](const QJsonObject& params) {
        return handleResourcesRead(params);
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

QJsonObject SkillHostService::sessionCapabilitiesForProvider(const ViewProviderContribution& provider) const
{
    QJsonObject capabilities;

    const QJsonObject controls = provider.controls;
    for(auto it = controls.constBegin(); it != controls.constEnd(); ++it) {
        const QJsonObject control = it.value().toObject();
        const QString commandName = control.value("command").toString().trimmed();
        if(!commandName.isEmpty()) {
            capabilities.insert(commandName, true);
        }
    }

    const QJsonArray actions = provider.actions;
    for(const QJsonValue& value : actions) {
        const QJsonObject action = value.toObject();
        const QString commandName = action.value("command").toString().trimmed();
        if(!commandName.isEmpty()) {
            capabilities.insert(commandName, true);
        }
    }

    if(provider.widgetType == "embedded_raw_browser") {
        capabilities.insert("raw_browser_embedded", true);
    }

    return capabilities;
}

QJsonObject SkillHostService::viewCommandResultSchema(const QJsonObject& stateSchema) const
{
    return QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
             {"session_id", QJsonObject{{"type", "string"}}},
             {"command", QJsonObject{{"type", "string"}}},
             {"state_before", stateSchema.isEmpty() ? QJsonObject{{"type", "object"}} : stateSchema},
             {"state_after", stateSchema.isEmpty() ? QJsonObject{{"type", "object"}} : stateSchema}
         }},
        {"required", QJsonArray{"session_id", "command"}}
    };
}

QJsonObject SkillHostService::workflowLoadToolDefinition() const
{
    return annotateCapabilityMetadata(annotatePlannerMetadata(QJsonObject{
        {"name", "studio.workflow.load"},
        {"description", "Load a .mne file, activate the DAG, and execute pending workflow nodes."},
        {"input_schema", objectSchema(QJsonObject{
             {"file", stringSchema("Workflow File",
                                   "Absolute path to the .mne workflow file to activate.")}
         }, QJsonArray{"file"})},
        {"result_schema", objectSchema(QJsonObject{
             {"status", stringSchema("Status")},
             {"source_file", stringSchema("Workflow File")},
             {"graph_resource_uri", stringSchema("Graph Resource URI")}
         }, QJsonArray{"status", "source_file", "graph_resource_uri"})},
        {"capability_kind", "workflow_io"},
        {"graph_mutation", "activate_graph"},
        {"extension_id", "temporal-filter-skill"},
        {"extension_display_name", "Workflow Manager"}
    },
    true,
    QStringLiteral("medium"),
    QStringLiteral("Activates a declarative workflow graph in the current workspace."),
    QStringLiteral("confirm_required"),
    QStringLiteral("Workflow activation changes workspace state and should be confirmed before execution."),
    false));
}

QJsonObject SkillHostService::workflowSaveToolDefinition() const
{
    return annotateCapabilityMetadata(annotatePlannerMetadata(QJsonObject{
        {"name", "studio.workflow.save"},
        {"description", "Persist the active workflow DAG back to a .mne file."},
        {"input_schema", objectSchema(QJsonObject{
             {"file", stringSchema("Workflow File",
                                   "Optional absolute save path. If omitted, the active workflow source file is updated.")}
         })},
        {"result_schema", objectSchema(QJsonObject{
             {"status", stringSchema("Status")},
             {"source_file", stringSchema("Workflow File")},
             {"graph_resource_uri", stringSchema("Graph Resource URI")}
         }, QJsonArray{"status", "source_file", "graph_resource_uri"})},
        {"capability_kind", "workflow_io"},
        {"graph_mutation", "persist_graph"},
        {"extension_id", "temporal-filter-skill"},
        {"extension_display_name", "Workflow Manager"}
    },
    false,
    QStringLiteral("high"),
    QStringLiteral("Persists workflow graph changes to disk and should only run with explicit user intent."),
    QStringLiteral("suggestion_only"),
    QStringLiteral("Workflow saves should be proposed explicitly rather than auto-executed."),
    false,
    QJsonArray{QStringLiteral("active_workflow_graph")}));
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

    const QJsonArray workflowResources = m_workflowManager.resourceDefinitions();
    for(const QJsonValue& value : workflowResources) {
        resources.append(value.toObject());
    }

    return QJsonObject{
        {"tool_name", "resources/list"},
        {"message", QString("Extension host resources: %1").arg(resources.size())},
        {"resources", resources}
    };
}

QJsonObject SkillHostService::handleResourcesRead(const QJsonObject& params) const
{
    const QString resourceUri = params.value("uri").toString().trimmed().isEmpty()
        ? params.value("id").toString().trimmed()
        : params.value("uri").toString().trimmed();

    return m_workflowManager.readResource(resourceUri);
}

QJsonObject SkillHostService::handleToolsList() const
{
    QJsonArray tools = m_registry.toolDefinitions();
    QSet<QString> toolNames;
    for(const QJsonValue& value : tools) {
        toolNames.insert(value.toObject().value("name").toString());
    }

    const QJsonArray pipelineTools = m_registry.analysisPipelineToolDefinitions();
    for(const QJsonValue& value : pipelineTools) {
        const QJsonObject tool = value.toObject();
        const QString toolName = tool.value("name").toString();
        if(!toolNames.contains(toolName)) {
            tools.append(tool);
            toolNames.insert(toolName);
        }
    }

    const QJsonArray workflowTools = m_workflowManager.toolDefinitions();
    for(const QJsonValue& value : workflowTools) {
        const QJsonObject tool = value.toObject();
        const QString toolName = tool.value("name").toString();
        if(!toolNames.contains(toolName)) {
            tools.append(tool);
            toolNames.insert(toolName);
        }
    }

    const QJsonObject workflowLoadTool = workflowLoadToolDefinition();
    if(!toolNames.contains(workflowLoadTool.value("name").toString())) {
        tools.append(workflowLoadTool);
        toolNames.insert(workflowLoadTool.value("name").toString());
    }

    const QJsonObject workflowSaveTool = workflowSaveToolDefinition();
    if(!toolNames.contains(workflowSaveTool.value("name").toString())) {
        tools.append(workflowSaveTool);
        toolNames.insert(workflowSaveTool.value("name").toString());
    }

    return QJsonObject{
        {"tool_name", "tools/list"},
        {"message", QString("Extension host tools: %1").arg(tools.size())},
        {"tools", tools}
    };
}

QJsonObject SkillHostService::handleToolCall(const QJsonObject& params)
{
    const QString toolName = params.value("name").toString();
    const QJsonObject arguments = params.value("arguments").toObject();

    if(toolName == "studio.workflow.load") {
        const QString filePath = arguments.value("file").toString().trimmed();
        if(filePath.isEmpty()) {
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "error"},
                {"message", "Workflow load requires a non-empty `file` argument."}
            };
        }

        try {
            m_workflowManager.loadAnalysisFile(filePath);
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "ok"},
                {"message", QString("Activated workflow graph from %1.").arg(QFileInfo(filePath).fileName())},
                {"source_file", filePath},
                {"graph_resource_uri", m_workflowManager.activeGraphResourceUri()},
                {"graph", m_workflowManager.activeGraph().toJson()}
            };
        } catch(const std::exception& error) {
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "error"},
                {"source_file", filePath},
                {"message", QString::fromUtf8(error.what())}
            };
        }
    }

    if(toolName == "studio.workflow.save") {
        const QString filePath = arguments.value("file").toString().trimmed();
        try {
            m_workflowManager.saveAnalysisFile(filePath);
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "ok"},
                {"message", QString("Saved workflow graph to %1.")
                                .arg(QFileInfo(m_workflowManager.activeGraphSourceFile()).fileName())},
                {"source_file", m_workflowManager.activeGraphSourceFile()},
                {"graph_resource_uri", m_workflowManager.activeGraphResourceUri()},
                {"graph", m_workflowManager.activeGraph().toJson()}
            };
        } catch(const std::exception& error) {
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "error"},
                {"source_file", filePath},
                {"message", QString::fromUtf8(error.what())}
            };
        }
    }

    if(m_workflowManager.canHandleTool(toolName)) {
        try {
            return m_workflowManager.appendNodeAndExecute(toolName, arguments);
        } catch(const std::exception& error) {
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "error"},
                {"message", QString::fromUtf8(error.what())}
            };
        }
    }

    if(toolName == "dummy3d.set_opacity") {
        const double opacity = std::max(0.0, std::min(1.0, arguments.value("opacity").toDouble(1.0)));

        QJsonArray updatedSessions;
        for(auto it = m_viewSessions.begin(); it != m_viewSessions.end(); ++it) {
            QJsonObject session = it.value();
            if(session.value("widget_type").toString() == QLatin1String("inspect_3d_surface")) {
                QJsonObject state = session.value("state").toObject();
                state.insert(QStringLiteral("opacity"), opacity);
                session.insert(QStringLiteral("state"), state);
                it.value() = session;
                updatedSessions.append(QJsonObject{
                    {"session_id", it.key()},
                    {"provider_id", session.value("provider_id").toString()},
                    {"opacity", opacity}
                });
            }
        }

        return QJsonObject{
            {"tool_name", toolName},
            {"status", "ok"},
            {"message", updatedSessions.isEmpty()
                ? QString("No 3D scene session is currently open; opacity %1 will apply to the next session.")
                      .arg(QString::number(opacity, 'f', 2))
                : QString("Set opacity to %1 across %2 active 3D scene session(s).")
                      .arg(QString::number(opacity, 'f', 2))
                      .arg(updatedSessions.size())},
            {"opacity", opacity},
            {"updated_sessions", updatedSessions}
        };
    }

    if(toolName == "fiffbrowser.reveal_active_state") {
        Q_UNUSED(arguments)

        QJsonArray browserSessions;
        for(auto it = m_viewSessions.constBegin(); it != m_viewSessions.constEnd(); ++it) {
            const QJsonObject session = it.value();
            if(session.value("extension_id").toString() == QLatin1String("fiff-browser-extension")) {
                browserSessions.append(QJsonObject{
                    {"session_id", it.key()},
                    {"file", session.value("file").toString()},
                    {"provider_id", session.value("provider_id").toString()},
                    {"state", session.value("state").toObject()}
                });
            }
        }

        if(browserSessions.isEmpty()) {
            return QJsonObject{
                {"tool_name", toolName},
                {"status", "ok"},
                {"message", "No FIFF browser session is currently open."},
                {"session_count", 0},
                {"sessions", QJsonArray{}}
            };
        }

        return QJsonObject{
            {"tool_name", toolName},
            {"status", "ok"},
            {"message", QString("Revealed state for %1 active FIFF browser session(s).").arg(browserSessions.size())},
            {"session_count", browserSessions.size()},
            {"sessions", browserSessions}
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
            const QJsonObject stateSchema = provider.stateSchema;
            const QJsonObject actionResultSchema = viewCommandResultSchema(stateSchema);
            QJsonArray actions = provider.actions;
            for(int i = 0; i < actions.size(); ++i) {
                QJsonObject action = actions.at(i).toObject();
                if(action.value("result_schema").toObject().isEmpty()) {
                    action.insert("result_schema", actionResultSchema);
                }
                actions[i] = action;
            }

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
                {"capabilities", sessionCapabilitiesForProvider(provider)},
                {"controls", provider.controls},
                {"actions", actions},
                {"state", provider.initialState},
                {"initial_state", provider.initialState},
                {"state_schema", stateSchema}
            };

            if(provider.widgetType == "embedded_raw_browser") {
                descriptor.insert("title", QFileInfo(filePath).fileName());
                descriptor.insert("message", QString("Opened FIFF browser extension session for %1.").arg(filePath));
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
    QJsonObject matchedCommandDefinition;
    QString matchedControlKey;
    bool isControlCommand = false;

    for(auto it = controls.constBegin(); it != controls.constEnd(); ++it) {
        const QJsonObject control = it.value().toObject();
        if(control.value("command").toString().trimmed() == commandName) {
            matchedCommandDefinition = control;
            matchedControlKey = it.key();
            isControlCommand = true;
            break;
        }
    }

    if(matchedCommandDefinition.isEmpty()) {
        const QJsonArray actions = descriptor.value("actions").toArray();
        for(const QJsonValue& value : actions) {
            const QJsonObject action = value.toObject();
            if(action.value("command").toString().trimmed() == commandName) {
                matchedCommandDefinition = action;
                break;
            }
        }
    }

    if(!matchedCommandDefinition.isEmpty()) {
        if(isControlCommand) {
            const QString targetArgument = matchedCommandDefinition.value("target_argument").toString().trimmed();
            const QString stateKey = matchedCommandDefinition.value("state_key").toString(targetArgument).trimmed();
            QJsonValue nextValue = arguments.value(targetArgument);
            if(matchedCommandDefinition.value("type").toString() == "number") {
                const double minimum = matchedCommandDefinition.value("minimum").toDouble(0.0);
                const double maximum = matchedCommandDefinition.value("maximum").toDouble(1.0);
                const double bounded = std::max(minimum, std::min(maximum, nextValue.toDouble(matchedCommandDefinition.value("value").toDouble())));
                nextValue = bounded;
            }
            if(!stateKey.isEmpty()) {
                state.insert(stateKey, nextValue);
            }
            QJsonObject updatedControl = matchedCommandDefinition;
            updatedControl.insert("value", nextValue);
            if(!matchedControlKey.isEmpty()) {
                controls.insert(matchedControlKey, updatedControl);
            }
            descriptor.insert("controls", controls);
        } else {
            if(matchedCommandDefinition.value("reset_to_initial_state").toBool(false)) {
                state = descriptor.value("initial_state").toObject();
            }
            const QJsonObject statePatch = matchedCommandDefinition.value("state_patch").toObject();
            for(auto it = statePatch.constBegin(); it != statePatch.constEnd(); ++it) {
                state.insert(it.key(), it.value());
            }
        }

        descriptor.insert("state", state);
        const QString message = matchedCommandDefinition.value("success_message").toString().trimmed().isEmpty()
            ? QString("Applied hosted view command %1.").arg(commandName)
            : matchedCommandDefinition.value("success_message").toString().trimmed();
        descriptor.insert("message", message);
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
            {"message", descriptor.value("message").toString()},
            {"result_schema", matchedCommandDefinition.value("result_schema").toObject()}
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
