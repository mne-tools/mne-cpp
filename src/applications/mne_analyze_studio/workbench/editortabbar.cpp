//=============================================================================================================
/**
 * @file     editortabbar.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the custom editor tab bar used by the studio center view.
 */

#include "editortabbar.h"

#include <QStyle>
#include <QToolButton>

using namespace MNEANALYZESTUDIO;

EditorTabBar::EditorTabBar(QWidget* parent)
: QTabBar(parent)
{
    setMovable(true);
    setExpanding(false);
    setDocumentMode(true);
    setUsesScrollButtons(true);
}

void EditorTabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);
    ensureCloseButton(index);
}

void EditorTabBar::tabRemoved(int index)
{
    QTabBar::tabRemoved(index);

    for(int i = 0; i < count(); ++i) {
        ensureCloseButton(i);
    }
}

void EditorTabBar::ensureCloseButton(int index)
{
    if(index < 0 || index >= count()) {
        return;
    }

    QWidget* existingLeftButton = tabButton(index, QTabBar::LeftSide);
    if(existingLeftButton) {
        existingLeftButton->hide();
        setTabButton(index, QTabBar::LeftSide, nullptr);
        existingLeftButton->deleteLater();
    }

    QToolButton* closeButton = qobject_cast<QToolButton*>(tabButton(index, QTabBar::RightSide));
    if(!closeButton) {
        closeButton = new QToolButton(this);
        closeButton->setAutoRaise(true);
        closeButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
        closeButton->setToolTip(tr("Close"));
        setTabButton(index, QTabBar::RightSide, closeButton);

        connect(closeButton, &QToolButton::clicked, this, [this, closeButton]() {
            for(int tabIndex = 0; tabIndex < count(); ++tabIndex) {
                if(tabButton(tabIndex, QTabBar::RightSide) == closeButton) {
                    emit closeButtonClicked(tabIndex);
                    return;
                }
            }
        });
    }
}
