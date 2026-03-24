//=============================================================================================================
/**
 * @file     extensionviewfactoryregistry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the shared hosted view factory registry used by the workbench and extensions.
 */

#ifndef MNE_ANALYZE_STUDIO_EXTENSIONVIEWFACTORYREGISTRY_H
#define MNE_ANALYZE_STUDIO_EXTENSIONVIEWFACTORYREGISTRY_H

#include <QString>

namespace MNEANALYZESTUDIO
{

class IExtensionViewFactory;

/**
 * @brief Shared registry for hosted extension widget factories.
 */
class ExtensionViewFactoryRegistry
{
public:
    static ExtensionViewFactoryRegistry& instance();

    void registerFactory(const IExtensionViewFactory* factory);
    const IExtensionViewFactory* factoryForWidgetType(const QString& widgetType) const;

private:
    ExtensionViewFactoryRegistry() = default;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_EXTENSIONVIEWFACTORYREGISTRY_H
