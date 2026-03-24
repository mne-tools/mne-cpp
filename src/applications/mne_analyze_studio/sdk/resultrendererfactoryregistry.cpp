//=============================================================================================================
/**
 * @file     resultrendererfactoryregistry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the shared result renderer factory registry.
 */

#include "resultrendererfactoryregistry.h"

#include "iresultrendererfactory.h"

#include <QHash>

using namespace MNEANALYZESTUDIO;

namespace
{

QHash<QString, const IResultRendererFactory*>& factoryMap()
{
    static QHash<QString, const IResultRendererFactory*> s_factories;
    return s_factories;
}

}

ResultRendererFactoryRegistry& ResultRendererFactoryRegistry::instance()
{
    static ResultRendererFactoryRegistry s_registry;
    return s_registry;
}

void ResultRendererFactoryRegistry::registerFactory(const IResultRendererFactory* factory)
{
    if(!factory) {
        return;
    }

    const QStringList toolNames = factory->supportedToolNames();
    for(const QString& toolName : toolNames) {
        const QString normalizedToolName = toolName.trimmed();
        if(!normalizedToolName.isEmpty()) {
            factoryMap().insert(normalizedToolName, factory);
        }
    }
}

const IResultRendererFactory* ResultRendererFactoryRegistry::factoryForToolName(const QString& toolName) const
{
    return factoryMap().value(toolName.trimmed(), nullptr);
}
