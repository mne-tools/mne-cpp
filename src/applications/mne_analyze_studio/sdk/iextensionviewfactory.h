//=============================================================================================================
/**
 * @file     iextensionviewfactory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the extension-provided hosted view factory interface.
 */

#ifndef MNE_ANALYZE_STUDIO_IEXTENSIONVIEWFACTORY_H
#define MNE_ANALYZE_STUDIO_IEXTENSIONVIEWFACTORY_H

#include <QString>

class QWidget;
class QJsonObject;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Factory interface used by extension libraries to create hosted workbench widgets.
 */
class IExtensionViewFactory
{
public:
    virtual ~IExtensionViewFactory() = default;

    virtual QString widgetType() const = 0;
    virtual QWidget* createView(const QJsonObject& sessionDescriptor, QWidget* parent = nullptr) const = 0;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_IEXTENSIONVIEWFACTORY_H
