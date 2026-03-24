//=============================================================================================================
/**
 * @file     extensionviewfactoryregistry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the shared hosted view factory registry.
 */

#include "extensionviewfactoryregistry.h"

#include "iextensionviewfactory.h"

#include <QHash>

using namespace MNEANALYZESTUDIO;

namespace
{

QHash<QString, const IExtensionViewFactory*>& factoryMap()
{
    static QHash<QString, const IExtensionViewFactory*> s_factories;
    return s_factories;
}

}

ExtensionViewFactoryRegistry& ExtensionViewFactoryRegistry::instance()
{
    static ExtensionViewFactoryRegistry s_registry;
    return s_registry;
}

void ExtensionViewFactoryRegistry::registerFactory(const IExtensionViewFactory* factory)
{
    if(!factory || factory->widgetType().trimmed().isEmpty()) {
        return;
    }

    factoryMap().insert(factory->widgetType().trimmed(), factory);
}

const IExtensionViewFactory* ExtensionViewFactoryRegistry::factoryForWidgetType(const QString& widgetType) const
{
    return factoryMap().value(widgetType.trimmed(), nullptr);
}
