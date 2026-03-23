//=============================================================================================================
/**
 * @file     editortabbar.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the custom editor tab bar used by the studio center view.
 */

#ifndef MNE_ANALYZE_STUDIO_EDITORTABBAR_H
#define MNE_ANALYZE_STUDIO_EDITORTABBAR_H

#include <QTabBar>

class QToolButton;

namespace MNEANALYZESTUDIO
{

/**
 * @brief Tab bar that keeps close buttons on the right side for every editor tab.
 */
class EditorTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit EditorTabBar(QWidget* parent = nullptr);

signals:
    void closeButtonClicked(int index);

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    void ensureCloseButton(int index);
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_EDITORTABBAR_H
