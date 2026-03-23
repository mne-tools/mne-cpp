//=============================================================================================================
/**
 * @file     editortabwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the editor tab widget that exposes custom tab bar installation.
 */

#ifndef MNE_ANALYZE_STUDIO_EDITORTABWIDGET_H
#define MNE_ANALYZE_STUDIO_EDITORTABWIDGET_H

#include <QTabWidget>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Small QTabWidget helper that exposes setTabBar for the center editor area.
 */
class EditorTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit EditorTabWidget(QWidget* parent = nullptr)
    : QTabWidget(parent)
    {
    }

    using QTabWidget::setTabBar;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_EDITORTABWIDGET_H
