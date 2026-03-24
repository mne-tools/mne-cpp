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

#include "manifestparser.h"

#include <QDirIterator>
#include <QFileInfo>

using namespace MNEANALYZESTUDIO;

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

    QDirIterator it(directoryPath,
                    QStringList() << "manifest.json",
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while(it.hasNext()) {
        const QString manifestPath = it.next();
        QString parseError;
        const ExtensionManifest manifest = parser.parseFile(manifestPath, &parseError);
        if(manifest.isValid()) {
            m_manifests.append(manifest);
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
            tools.append(QJsonObject{
                {"name", tool.name},
                {"description", tool.description},
                {"input_schema", tool.inputSchema},
                {"result_schema", tool.resultSchema},
                {"extension_id", manifest.id},
                {"extension_display_name", manifest.displayName}
            });
        }
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
