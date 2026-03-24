//=============================================================================================================
/**
 * @file     iresultrendererfactory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the extension-provided result renderer factory interface.
 */

#ifndef MNE_ANALYZE_STUDIO_IRESULTRENDERERFACTORY_H
#define MNE_ANALYZE_STUDIO_IRESULTRENDERERFACTORY_H

#include <QString>
#include <QStringList>

class QWidget;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Factory interface used by extension libraries to create result renderer widgets.
 */
class IResultRendererFactory
{
public:
    virtual ~IResultRendererFactory() = default;

    virtual QString rendererId() const = 0;
    virtual QString widgetType() const = 0;
    virtual QStringList supportedToolNames() const = 0;
    virtual QWidget* createRenderer(QWidget* parent = nullptr) const = 0;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_IRESULTRENDERERFACTORY_H
