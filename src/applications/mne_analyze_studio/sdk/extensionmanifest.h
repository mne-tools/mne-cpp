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
};

struct UiContribution
{
    struct SettingsTabContribution
    {
        QString id;
        QString title;
        QString description;
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
