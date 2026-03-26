//=============================================================================================================
/**
 * @file     viewproviderregistry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the registry used by the lean core to resolve pluggable view providers.
 */

#ifndef MNE_ANALYZE_STUDIO_VIEWPROVIDERREGISTRY_H
#define MNE_ANALYZE_STUDIO_VIEWPROVIDERREGISTRY_H

#include "studio_core_global.h"

#include <extensionmanifest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QVector>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Registry that discovers extension manifests and resolves pluggable view providers.
 */
class STUDIOCORESHARED_EXPORT ViewProviderRegistry : public QObject
{
    Q_OBJECT

public:
    explicit ViewProviderRegistry(QObject* parent = nullptr);

    bool loadFromDirectory(const QString& directoryPath, QString* errorMessage = nullptr);
    void setDisabledExtensionIds(const QStringList& extensionIds);
    QStringList disabledExtensionIds() const;
    bool isExtensionEnabled(const QString& extensionId) const;
    QVector<ExtensionManifest> manifests() const;
    QVector<ExtensionManifest> allManifests() const;
    QJsonArray toolDefinitions() const;
    QJsonArray analysisPipelineToolDefinitions() const;
    QJsonArray resultRendererDefinitions() const;
    QJsonArray analysisPipelineDefinitions() const;
    QJsonObject providerForFile(const QString& filePath, const QJsonObject& metadata = QJsonObject()) const;

private:
    QVector<ExtensionManifest> m_manifests;
    QStringList m_disabledExtensionIds;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_VIEWPROVIDERREGISTRY_H
