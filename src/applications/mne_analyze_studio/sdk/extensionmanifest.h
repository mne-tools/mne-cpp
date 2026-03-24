//=============================================================================================================
/**
 * @file     extensionmanifest.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares manifest data structures for lean-core studio extensions.
 */

#ifndef MNE_ANALYZE_STUDIO_EXTENSIONMANIFEST_H
#define MNE_ANALYZE_STUDIO_EXTENSIONMANIFEST_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace MNEANALYZESTUDIO
{

struct ToolContribution
{
    QString name;
    QString description;
    QJsonObject inputSchema;
    QJsonObject resultSchema;
};

struct ViewProviderContribution
{
    QString id;
    QString displayName;
    QString widgetType;
    QString slot;
    QStringList fileExtensions;
    bool supportsSceneMerging = false;
    QJsonObject controls;
    QJsonArray actions;
    QJsonObject stateSchema;
    QJsonObject initialState;
};

struct ResultRendererContribution
{
    QString id;
    QString displayName;
    QString widgetType;
    QStringList toolNames;
    QJsonArray controls;
    QJsonArray actions;
    QJsonObject runtimeContextSchema;
    QJsonObject historySchema;
};

struct AnalysisPipelineContribution
{
    QString id;
    QString displayName;
    QString description;
    QJsonObject inputSchema;
    QJsonObject outputSchema;
    QJsonArray steps;
    QJsonArray followUpActions;
};

struct UiContribution
{
    struct SettingsTabContribution
    {
        QString id;
        QString title;
        QString description;
        QJsonArray fields;
        QJsonArray actions;
    };

    QStringList sidebarItems;
    QStringList menuItems;
    QVector<SettingsTabContribution> settingsTabs;
};

struct ExtensionManifest
{
    QString id;
    QString displayName;
    QString version;
    QString rootPath;
    QString entryPoint;
    QVector<ViewProviderContribution> viewProviders;
    QVector<ResultRendererContribution> resultRenderers;
    QVector<AnalysisPipelineContribution> analysisPipelines;
    QVector<ToolContribution> tools;
    UiContribution ui;
    QJsonObject rawManifest;

    bool isValid() const
    {
        return !id.trimmed().isEmpty();
    }
};

} // namespace MNEANALYESTUDIO

#endif // MNE_ANALYZE_STUDIO_EXTENSIONMANIFEST_H
