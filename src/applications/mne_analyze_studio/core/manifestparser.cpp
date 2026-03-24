//=============================================================================================================
/**
 * @file     manifestparser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements manifest parsing helpers for lean-core studio extensions.
 */

#include "manifestparser.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

using namespace MNEANALYZESTUDIO;

ManifestParser::ManifestParser(QObject* parent)
: QObject(parent)
{
}

ExtensionManifest ManifestParser::parseFile(const QString& manifestFilePath, QString* errorMessage) const
{
    ExtensionManifest manifest;

    QFile file(manifestFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if(errorMessage) {
            *errorMessage = QString("Could not open manifest: %1").arg(manifestFilePath);
        }
        return manifest;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if(error.error != QJsonParseError::NoError || !document.isObject()) {
        if(errorMessage) {
            *errorMessage = QString("Invalid manifest JSON in %1: %2").arg(manifestFilePath, error.errorString());
        }
        return manifest;
    }

    const QJsonObject root = document.object();
    manifest.id = root.value("id").toString().trimmed();
    manifest.displayName = root.value("display_name").toString(root.value("name").toString(manifest.id));
    manifest.version = root.value("version").toString();
    manifest.entryPoint = root.value("entry_point").toString();
    manifest.rootPath = QFileInfo(manifestFilePath).absolutePath();
    manifest.rawManifest = root;

    const QJsonObject contributes = root.value("contributes").toObject();
    const QJsonArray viewProviders = contributes.value("view_providers").toArray();
    for(const QJsonValue& value : viewProviders) {
        const QJsonObject object = value.toObject();
        ViewProviderContribution contribution;
        contribution.id = object.value("id").toString();
        contribution.displayName = object.value("display_name").toString(contribution.id);
        contribution.widgetType = object.value("widget_type").toString("placeholder");
        contribution.slot = object.value("slot").toString("center");
        contribution.supportsSceneMerging = object.value("supports_scene_merging").toBool(false);
        const QJsonArray fileExtensions = object.value("file_extensions").toArray();
        for(const QJsonValue& extensionValue : fileExtensions) {
            contribution.fileExtensions.append(extensionValue.toString().trimmed().toLower());
        }
        manifest.viewProviders.append(contribution);
    }

    const QJsonArray tools = contributes.value("tools").toArray();
    for(const QJsonValue& value : tools) {
        const QJsonObject object = value.toObject();
        ToolContribution tool;
        tool.name = object.value("name").toString();
        tool.description = object.value("description").toString();
        tool.inputSchema = object.value("input_schema").toObject();
        tool.resultSchema = object.value("result_schema").toObject();
        manifest.tools.append(tool);
    }

    const QJsonObject ui = contributes.value("ui").toObject();
    const QJsonArray sidebarItems = ui.value("sidebar_items").toArray();
    for(const QJsonValue& value : sidebarItems) {
        manifest.ui.sidebarItems.append(value.toString());
    }
    const QJsonArray menuItems = ui.value("menu_items").toArray();
    for(const QJsonValue& value : menuItems) {
        manifest.ui.menuItems.append(value.toString());
    }
    const QJsonArray settingsTabs = ui.value("settings_tabs").toArray();
    for(const QJsonValue& value : settingsTabs) {
        const QJsonObject object = value.toObject();
        UiContribution::SettingsTabContribution tab;
        tab.id = object.value("id").toString().trimmed();
        tab.title = object.value("title").toString(tab.id);
        tab.description = object.value("description").toString().trimmed();
        if(!tab.id.isEmpty()) {
            manifest.ui.settingsTabs.append(tab);
        }
    }

    if(!manifest.isValid() && errorMessage) {
        *errorMessage = QString("Manifest in %1 is missing an id.").arg(manifestFilePath);
    }

    return manifest;
}
