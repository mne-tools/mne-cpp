//=============================================================================================================
/**
 * @file     manifestparser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares manifest parsing helpers for lean-core studio extensions.
 */

#ifndef MNE_ANALYZE_STUDIO_MANIFESTPARSER_H
#define MNE_ANALYZE_STUDIO_MANIFESTPARSER_H

#include "studio_core_global.h"

#include <extensionmanifest.h>

#include <QObject>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Helper that reads extension manifests from disk into typed SDK structures.
 */
class STUDIOCORESHARED_EXPORT ManifestParser : public QObject
{
    Q_OBJECT

public:
    explicit ManifestParser(QObject* parent = nullptr);

    ExtensionManifest parseFile(const QString& manifestFilePath, QString* errorMessage = nullptr) const;
};

} // namespace MNEANALYESTUDIO

#endif // MNE_ANALYZE_STUDIO_MANIFESTPARSER_H
