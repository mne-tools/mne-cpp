//=============================================================================================================
/**
 * @file     viewproviderregistry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the registry used by the lean core to resolve pluggable view providers.
 */

#include "viewproviderregistry.h"

#include "capabilityutils.h"
#include "manifestparser.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QHash>

using namespace MNEANALYZESTUDIO;

namespace
{

QJsonObject objectSchema(const QJsonObject& properties,
                         const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QJsonObject stringSchema(const QString& title,
                         const QJsonArray& values = QJsonArray(),
                         const QString& defaultValue = QString())
{
    QJsonObject schema{
        {"type", "string"},
        {"title", title}
    };

    if(!values.isEmpty()) {
        schema.insert("enum", values);
    }

    if(!defaultValue.isEmpty()) {
        schema.insert("default", defaultValue);
    }

    return schema;
}

QJsonObject integerSchema(const QString& title,
                          int minimum,
                          int maximum,
                          int defaultValue)
{
    return QJsonObject{
        {"type", "integer"},
        {"title", title},
        {"minimum", minimum},
        {"maximum", maximum},
        {"default", defaultValue}
    };
}

} // namespace

ViewProviderRegistry::ViewProviderRegistry(QObject* parent)
: QObject(parent)
{
}

bool ViewProviderRegistry::loadFromDirectory(const QString& directoryPath, QString* errorMessage)
{
    m_manifests.clear();

    QFileInfo info(directoryPath);
    if(!info.exists() || !info.isDir()) {
        if(errorMessage) {
            *errorMessage = QString("Extension directory not found: %1").arg(directoryPath);
        }
        return false;
    }

    ManifestParser parser;
    QStringList errors;
    QHash<QString, QString> manifestOrigins;
    QHash<QString, QString> providerOrigins;
    QHash<QString, QString> toolOrigins;
    QHash<QString, QString> rendererOrigins;
    QHash<QString, QString> pipelineOrigins;

    QDirIterator it(directoryPath,
                    QStringList() << "manifest.json",
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while(it.hasNext()) {
        const QString manifestPath = it.next();
        QString parseError;
        const ExtensionManifest manifest = parser.parseFile(manifestPath, &parseError);
        if(manifest.isValid()) {
            QStringList manifestErrors;
            const QString manifestLabel = QString("%1 (%2)").arg(manifest.id, manifestPath);

            const auto noteDuplicate = [&manifestErrors, &manifestLabel](const QString& kind,
                                                                         const QString& id,
                                                                         const QString& existingOrigin) {
                manifestErrors.append(QString("Duplicate %1 `%2` in %3; already declared by %4.")
                                          .arg(kind, id, manifestLabel, existingOrigin));
            };

            const auto checkLocalDuplicate = [&manifestErrors, &manifestLabel](QHash<QString, QString>& localOrigins,
                                                                               const QString& kind,
                                                                               const QString& id) {
                const QString trimmedId = id.trimmed();
                if(trimmedId.isEmpty()) {
                    return;
                }

                if(localOrigins.contains(trimmedId)) {
                    manifestErrors.append(QString("Duplicate %1 `%2` declared twice inside %3.")
                                              .arg(kind, trimmedId, manifestLabel));
                    return;
                }

                localOrigins.insert(trimmedId, manifestLabel);
            };

            if(manifestOrigins.contains(manifest.id)) {
                noteDuplicate(QStringLiteral("extension id"), manifest.id, manifestOrigins.value(manifest.id));
            }

            QHash<QString, QString> localProviderOrigins;
            QHash<QString, QString> localToolOrigins;
            QHash<QString, QString> localRendererOrigins;
            QHash<QString, QString> localPipelineOrigins;

            for(const ViewProviderContribution& provider : manifest.viewProviders) {
                const QString providerId = provider.id.trimmed();
                checkLocalDuplicate(localProviderOrigins, QStringLiteral("view provider id"), providerId);
                if(!providerId.isEmpty() && providerOrigins.contains(providerId)) {
                    noteDuplicate(QStringLiteral("view provider id"), providerId, providerOrigins.value(providerId));
                }
            }

            for(const ToolContribution& tool : manifest.tools) {
                const QString toolName = tool.name.trimmed();
                checkLocalDuplicate(localToolOrigins, QStringLiteral("tool name"), toolName);
                if(!toolName.isEmpty() && toolOrigins.contains(toolName)) {
                    noteDuplicate(QStringLiteral("tool name"), toolName, toolOrigins.value(toolName));
                }
            }

            for(const ResultRendererContribution& renderer : manifest.resultRenderers) {
                const QString rendererId = renderer.id.trimmed();
                checkLocalDuplicate(localRendererOrigins, QStringLiteral("result renderer id"), rendererId);
                if(!rendererId.isEmpty() && rendererOrigins.contains(rendererId)) {
                    noteDuplicate(QStringLiteral("result renderer id"), rendererId, rendererOrigins.value(rendererId));
                }
            }

            for(const AnalysisPipelineContribution& pipeline : manifest.analysisPipelines) {
                const QString pipelineId = pipeline.id.trimmed();
                checkLocalDuplicate(localPipelineOrigins, QStringLiteral("analysis pipeline id"), pipelineId);
                if(!pipelineId.isEmpty() && pipelineOrigins.contains(pipelineId)) {
                    noteDuplicate(QStringLiteral("analysis pipeline id"), pipelineId, pipelineOrigins.value(pipelineId));
                }
            }

            if(!manifestErrors.isEmpty()) {
                errors.append(manifestErrors.join(" | "));
                continue;
            }

            m_manifests.append(manifest);

            manifestOrigins.insert(manifest.id, manifestLabel);
            for(const ViewProviderContribution& provider : manifest.viewProviders) {
                const QString providerId = provider.id.trimmed();
                if(!providerId.isEmpty()) {
                    providerOrigins.insert(providerId, manifestLabel);
                }
            }
            for(const ToolContribution& tool : manifest.tools) {
                const QString toolName = tool.name.trimmed();
                if(!toolName.isEmpty()) {
                    toolOrigins.insert(toolName, manifestLabel);
                }
            }
            for(const ResultRendererContribution& renderer : manifest.resultRenderers) {
                const QString rendererId = renderer.id.trimmed();
                if(!rendererId.isEmpty()) {
                    rendererOrigins.insert(rendererId, manifestLabel);
                }
            }
            for(const AnalysisPipelineContribution& pipeline : manifest.analysisPipelines) {
                const QString pipelineId = pipeline.id.trimmed();
                if(!pipelineId.isEmpty()) {
                    pipelineOrigins.insert(pipelineId, manifestLabel);
                }
            }
        } else if(!parseError.isEmpty()) {
            errors.append(parseError);
        }
    }

    if(errorMessage && !errors.isEmpty()) {
        *errorMessage = errors.join(" | ");
    }

    return !m_manifests.isEmpty();
}

void ViewProviderRegistry::setDisabledExtensionIds(const QStringList& extensionIds)
{
    m_disabledExtensionIds = extensionIds;
}

QStringList ViewProviderRegistry::disabledExtensionIds() const
{
    return m_disabledExtensionIds;
}

bool ViewProviderRegistry::isExtensionEnabled(const QString& extensionId) const
{
    return !m_disabledExtensionIds.contains(extensionId.trimmed());
}

QVector<ExtensionManifest> ViewProviderRegistry::manifests() const
{
    QVector<ExtensionManifest> enabledManifests;
    for(const ExtensionManifest& manifest : m_manifests) {
        if(isExtensionEnabled(manifest.id)) {
            enabledManifests.append(manifest);
        }
    }

    return enabledManifests;
}

QVector<ExtensionManifest> ViewProviderRegistry::allManifests() const
{
    return m_manifests;
}

QJsonArray ViewProviderRegistry::toolDefinitions() const
{
    QJsonArray tools;
    for(const ExtensionManifest& manifest : manifests()) {
        for(const ToolContribution& tool : manifest.tools) {
            tools.append(annotatePlannerMetadata(QJsonObject{
                {"name", tool.name},
                {"description", tool.description},
                {"input_schema", tool.inputSchema},
                {"result_schema", tool.resultSchema},
                {"capability_id", QStringLiteral("tool:%1").arg(tool.name.trimmed())},
                {"capability_kind", QStringLiteral("extension_tool")},
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName}
            },
            false,
            QStringLiteral("medium"),
            QStringLiteral("Extension-contributed tools may have side effects that require explicit user intent."),
            QStringLiteral("suggestion_only"),
            QStringLiteral("Extension-contributed tools should be suggested rather than auto-executed."),
            false));
        }
    }

    return tools;
}

QJsonArray ViewProviderRegistry::analysisPipelineToolDefinitions() const
{
    QJsonArray tools;
    const QJsonArray pipelines = analysisPipelineDefinitions();
    for(const QJsonValue& value : pipelines) {
        const QJsonObject pipeline = value.toObject();
        const QString pipelineId = pipeline.value("id").toString().trimmed();
        if(pipelineId.isEmpty()) {
            continue;
        }

        const QString displayName = pipeline.value("display_name").toString(pipelineId);
        const QJsonObject inputSchema = pipeline.value("input_schema").toObject();
        const QJsonObject expectedOutputSchema = pipeline.value("output_schema").toObject();

        tools.append(annotateCapabilityMetadata(annotatePlannerMetadata(QJsonObject{
            {"name", pipelineRunAliasToolName(pipelineId)},
            {"display_name", displayName},
            {"description", pipeline.value("description").toString().trimmed().isEmpty()
                                ? QString("Run the analysis pipeline `%1`.").arg(displayName)
                                : pipeline.value("description").toString().trimmed()},
            {"input_schema", inputSchema.isEmpty() ? objectSchema(QJsonObject()) : inputSchema},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"queued", "error"})},
                 {"pipeline_id", stringSchema("Pipeline ID", QJsonArray(), pipelineId)},
                 {"run_id", stringSchema("Run ID")},
                 {"queued_steps", integerSchema("Queued Steps", 0, 1000, pipeline.value("steps").toArray().size())}
             }, QJsonArray{"status", "pipeline_id", "queued_steps"})},
            {"pipeline_id", pipelineId},
            {"pipeline_run_tool", QStringLiteral("studio.pipeline.run")},
            {"steps", pipeline.value("steps").toArray()},
            {"follow_up_actions", pipeline.value("follow_up_actions").toArray()},
            {"expected_output_schema", expectedOutputSchema},
            {"extension_id", pipeline.value("extension_id").toString()},
            {"extension_display_name", pipeline.value("extension_display_name").toString()},
            {"capability_kind", QStringLiteral("analysis_pipeline")},
            {"capability_id", QStringLiteral("pipeline:%1").arg(pipelineId)},
            {"capability_aliases", QJsonArray::fromStringList(QStringList()
                 << pipelineId
                 << QStringLiteral("studio.pipeline.run"))}
        },
        true,
        QStringLiteral("medium"),
        QStringLiteral("Runs a manifest-declared analysis pipeline against the active dataset."),
        QStringLiteral("confirm_required"),
        QStringLiteral("Pipeline execution changes workspace state and should be surfaced before execution."),
        false,
        QJsonArray{QStringLiteral("analysis_pipeline_contract"), QStringLiteral("active_raw_browser")},
        QJsonObject{{"planner_validate_analysis_pipeline", true}})));
    }

    return tools;
}

QJsonArray ViewProviderRegistry::resultRendererDefinitions() const
{
    QJsonArray renderers;
    for(const ExtensionManifest& manifest : manifests()) {
        for(const ResultRendererContribution& renderer : manifest.resultRenderers) {
            renderers.append(QJsonObject{
                {"id", renderer.id},
                {"display_name", renderer.displayName},
                {"widget_type", renderer.widgetType},
                {"tool_names", QJsonArray::fromStringList(renderer.toolNames)},
                {"controls", renderer.controls},
                {"actions", renderer.actions},
                {"runtime_context_schema", renderer.runtimeContextSchema},
                {"history_schema", renderer.historySchema},
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName}
            });
        }
    }

    return renderers;
}

QJsonArray ViewProviderRegistry::analysisPipelineDefinitions() const
{
    QJsonArray pipelines;
    for(const ExtensionManifest& manifest : manifests()) {
        for(const AnalysisPipelineContribution& pipeline : manifest.analysisPipelines) {
            pipelines.append(QJsonObject{
                {"id", pipeline.id},
                {"display_name", pipeline.displayName},
                {"description", pipeline.description},
                {"input_schema", pipeline.inputSchema},
                {"output_schema", pipeline.outputSchema},
                {"steps", pipeline.steps},
                {"follow_up_actions", pipeline.followUpActions},
                {"capability_id", QStringLiteral("pipeline:%1").arg(pipeline.id)},
                {"capability_kind", QStringLiteral("analysis_pipeline")},
                {"capability_aliases", QJsonArray::fromStringList(QStringList()
                     << pipeline.id
                     << "studio.pipeline.run")},
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName}
            });
        }
    }

    return pipelines;
}

QJsonObject ViewProviderRegistry::providerForFile(const QString& filePath, const QJsonObject& metadata) const
{
    Q_UNUSED(metadata)

    const QString extension = QString(".%1").arg(QFileInfo(filePath).suffix().toLower());
    for(const ExtensionManifest& manifest : manifests()) {
        for(const ViewProviderContribution& provider : manifest.viewProviders) {
            if(provider.fileExtensions.contains(extension)) {
                return QJsonObject{
                    {"extension_id", manifest.id},
                    {"extension_display_name", manifest.displayName},
                    {"provider_id", provider.id},
                    {"provider_display_name", provider.displayName},
                    {"widget_type", provider.widgetType},
                    {"slot", provider.slot},
                    {"supports_scene_merging", provider.supportsSceneMerging}
                };
            }
        }
    }

    return QJsonObject();
}
