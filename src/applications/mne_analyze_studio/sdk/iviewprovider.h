//=============================================================================================================
/**
 * @file     iviewprovider.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the SDK interface used by pluggable view providers.
 */

#ifndef MNE_ANALYZE_STUDIO_IVIEWPROVIDER_H
#define MNE_ANALYZE_STUDIO_IVIEWPROVIDER_H

#include "extensionmanifest.h"

#include <QJsonObject>
#include <QString>

class QWidget;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Abstract interface implemented by extension-hosted view providers.
 */
class IViewProvider
{
public:
    virtual ~IViewProvider() = default;

    virtual QString providerId() const = 0;
    virtual ViewProviderContribution contribution() const = 0;
    virtual bool canRender(const QString& filePath, const QJsonObject& metadata = QJsonObject()) const = 0;
    virtual QWidget* createView(QWidget* parent,
                                const QString& filePath,
                                const QJsonObject& metadata = QJsonObject()) = 0;
};

} // namespace MNEANALYESTUDIO

#endif // MNE_ANALYZE_STUDIO_IVIEWPROVIDER_H
