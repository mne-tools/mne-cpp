//=============================================================================================================
/**
 * @file     iresultrendererwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the extension-owned result renderer widget contract.
 */

#ifndef MNE_ANALYZE_STUDIO_IRESULTRENDERERWIDGET_H
#define MNE_ANALYZE_STUDIO_IRESULTRENDERERWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Interface implemented by extension-owned widgets that render structured tool results.
 */
class IResultRendererWidget
{
public:
    virtual ~IResultRendererWidget() = default;

    virtual void setResult(const QString& toolName, const QJsonObject& result) = 0;
    virtual QString toolName() const = 0;
    virtual QJsonObject result() const = 0;
    virtual void setResultHistory(const QJsonArray& history)
    {
        Q_UNUSED(history)
    }

    virtual void setRuntimeContext(const QJsonObject& context)
    {
        Q_UNUSED(context)
    }
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_IRESULTRENDERERWIDGET_H
