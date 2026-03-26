//=============================================================================================================
/**
 * @file     capabilitycatalog.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares helpers for building a normalized Studio capability catalog.
 */

#ifndef MNE_ANALYZE_STUDIO_CAPABILITYCATALOG_H
#define MNE_ANALYZE_STUDIO_CAPABILITYCATALOG_H

#include "studio_core_global.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace MNEANALYZESTUDIO
{

struct CapabilityCatalogSource
{
    QString sourceId;
    QString sourceDisplayName;
    QJsonArray tools;
};

STUDIOCORESHARED_EXPORT QJsonArray buildCapabilityCatalog(const QVector<CapabilityCatalogSource>& sources,
                                                          QStringList* warnings = nullptr);
STUDIOCORESHARED_EXPORT QJsonObject capabilityFromCatalog(const QJsonArray& catalog, const QString& key);
STUDIOCORESHARED_EXPORT QJsonArray capabilitiesFromCatalogBySource(const QJsonArray& catalog,
                                                                   const QString& sourceId);

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_CAPABILITYCATALOG_H
