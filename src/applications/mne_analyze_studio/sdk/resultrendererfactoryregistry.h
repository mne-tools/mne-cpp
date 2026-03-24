//=============================================================================================================
/**
 * @file     resultrendererfactoryregistry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the shared result renderer factory registry used by the workbench and extensions.
 */

#ifndef MNE_ANALYZE_STUDIO_RESULTRENDERERFACTORYREGISTRY_H
#define MNE_ANALYZE_STUDIO_RESULTRENDERERFACTORYREGISTRY_H

#include <QString>

namespace MNEANALYZESTUDIO
{

class IResultRendererFactory;

/**
 * @brief Shared registry for extension-owned result renderer factories.
 */
class ResultRendererFactoryRegistry
{
public:
    static ResultRendererFactoryRegistry& instance();

    void registerFactory(const IResultRendererFactory* factory);
    const IResultRendererFactory* factoryForToolName(const QString& toolName) const;

private:
    ResultRendererFactoryRegistry() = default;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_RESULTRENDERERFACTORYREGISTRY_H
