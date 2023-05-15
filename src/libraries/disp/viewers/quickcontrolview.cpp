//=============================================================================================================
/**
 * @file     quickcontrolview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the QuickControlView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_quickcontrolview.h"

#include "quickcontrolview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGroupBox>
#include <QTabWidget>
#include <QSettings>
#include <QApplication>
#include <QScreen>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QuickControlView::QuickControlView(const QString &sSettingsPath,
                                   const QString& name,
                                   Qt::WindowFlags flags,
                                   QWidget *parent,
                                   bool bDraggable)
: DraggableFramelessWidget(parent, flags, false, bDraggable, true)
, m_sName(name)
, m_pUi(new Ui::QuickControlViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);
    m_pUi->m_pTabWidget->setTabBarAutoHide(true);
    m_pUi->m_pTabWidget->setMovable(true);
    m_pUi->m_pTabWidget->setTabPosition(QTabWidget::West);
    this->setWindowTitle("Quick Control");

    if(!(windowFlags() & Qt::CustomizeWindowHint)) {
        m_pUi->m_pushButton_close->hide();
    }

    //Init opacity slider
    connect(m_pUi->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &QuickControlView::onOpacityChange);

    //Init and connect close button
    connect(m_pUi->m_pushButton_close, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlView::hide);

    this->adjustSize();

    loadSettings();
}

//=============================================================================================================

QuickControlView::~QuickControlView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void QuickControlView::clear()
{
    while(m_pUi->m_pTabWidget->count() != 0) {
        QWidget* pTabWidget = m_pUi->m_pTabWidget->widget(m_pUi->m_pTabWidget->count()-1);
        m_pUi->m_pTabWidget->removeTab(m_pUi->m_pTabWidget->count()-1);
        delete pTabWidget;
    }
}

//=============================================================================================================

QVBoxLayout* QuickControlView::findTabWidgetLayout(const QString& sTabName)
{
    QVBoxLayout* pTabWidgetLayout = Q_NULLPTR;

    if(QWidget* pTabWidget = m_pUi->m_pTabWidget->findChild<QWidget *>(sTabName + "TabWidget")) {
        // Tab widget already exisits. Get the grid layout and return it.
        pTabWidgetLayout = qobject_cast<QVBoxLayout *>(pTabWidget->layout());
    } else {
        // Tab widget does not exist yet. Create it and return grid lyout.
        pTabWidget = new QWidget();
        pTabWidget->setObjectName(sTabName + "TabWidget");
        pTabWidgetLayout = new QVBoxLayout();
        pTabWidgetLayout->setContentsMargins(4,2,4,4);
        pTabWidget->setLayout(pTabWidgetLayout);
        m_pUi->m_pTabWidget->insertTab(0,pTabWidget, sTabName);
    }

    return pTabWidgetLayout;
}

//=============================================================================================================

void QuickControlView::addWidgets(const QList<QWidget*>& lWidgets,
                                  const QString& sTabName,
                                  bool bAddToEnd)
{
    for(int i = 0; i < lWidgets.size(); ++i) {
        QString sObjectName = lWidgets.at(i)->objectName();

        if(sObjectName.contains("widget_", Qt::CaseInsensitive)) {
            this->addWidget(lWidgets.at(i), sTabName, bAddToEnd);
        }

        if(sObjectName.contains("group_", Qt::CaseInsensitive)) {
            if(sObjectName.contains("group_tab_", Qt::CaseInsensitive)) {
                sObjectName.remove("group_tab_");
                QStringList sList = sObjectName.split("_");

                if(sList.size() >= 2) {
                    this->addGroupBoxWithTabs(lWidgets.at(i), sList.at(0), sList.at(1), sTabName, bAddToEnd);
                } else {
                    this->addGroupBoxWithTabs(lWidgets.at(i), "", sObjectName, sTabName, bAddToEnd);
                }
            } else {
                sObjectName.remove("group_");
                this->addGroupBox(lWidgets.at(i), sObjectName, sTabName, bAddToEnd);
            }
        }
    }
}

//=============================================================================================================

void QuickControlView::addWidget(QWidget* pWidget,
                                 const QString& sTabName,
                                 bool bAddToEnd)
{
    if(QVBoxLayout* pTabWidgetLayout = findTabWidgetLayout(sTabName)) {
        int iPos = bAddToEnd ? -1 : 0;
        pTabWidgetLayout->insertWidget(iPos,
                                       pWidget,
                                       0);
    }
}

//=============================================================================================================

void QuickControlView::addGroupBox(QWidget* pWidget,
                                   const QString& sGroupBoxName,
                                   const QString& sTabName,
                                   bool bAddToEnd)
{
    if(QVBoxLayout* pTabWidgetLayout = findTabWidgetLayout(sTabName)) {
        int iPos = bAddToEnd ? -1 : 0;
        QGroupBox* pGroupBox = new QGroupBox(sGroupBoxName);
        pGroupBox->setObjectName(sGroupBoxName);

        QVBoxLayout *pVBox = new QVBoxLayout;

        pVBox->setContentsMargins(0,0,0,0);
        pVBox->addWidget(pWidget);
        pGroupBox->setLayout(pVBox);

        pTabWidgetLayout->insertWidget(iPos,
                                       pGroupBox,
                                       0);
    }
}

//=============================================================================================================

void QuickControlView::addGroupBoxWithTabs(QWidget* pWidget,
                                           const QString& sGroupBoxName,
                                           const QString& sTabNameGroupBox,
                                           const QString& sTabName,
                                           bool bAddToEnd)
{
    if(QVBoxLayout* pTabWidgetLayout = findTabWidgetLayout(sTabName)) {
        QGroupBox* pGroupBox = pTabWidgetLayout->parentWidget()->findChild<QGroupBox *>(sGroupBoxName);

        if(!pGroupBox) {
            int iPos = bAddToEnd ? -1 : 0;
            pGroupBox = new QGroupBox(sGroupBoxName);
            pGroupBox->setObjectName(sGroupBoxName);

            pTabWidgetLayout->insertWidget(iPos,
                                           pGroupBox,
                                           0);

            QVBoxLayout *pVBox = new QVBoxLayout;
            QTabWidget* pTabWidget = new QTabWidget;
            pTabWidget->setTabBarAutoHide(false);
            pTabWidget->setMovable(true);
            pTabWidget->setObjectName(sGroupBoxName + "TabWidget");

            pTabWidget->addTab(pWidget, sTabNameGroupBox);
            pVBox->setContentsMargins(4,2,4,4);
            pVBox->addWidget(pTabWidget);
            pGroupBox->setLayout(pVBox);
        } else {
            QTabWidget* pTabWidget = pGroupBox->findChild<QTabWidget *>(sGroupBoxName + "TabWidget");

            if(pTabWidget) {
                pTabWidget->addTab(pWidget, sTabNameGroupBox);
            }
        }
    }
}

//=============================================================================================================

void QuickControlView::setOpacityValue(int opactiy)
{
    m_pUi->m_horizontalSlider_opacity->setValue(opactiy);

    onOpacityChange(opactiy);
}

//=============================================================================================================

int QuickControlView::getOpacityValue()
{
    return m_pUi->m_horizontalSlider_opacity->value();
}

//=============================================================================================================

void QuickControlView::setVisiblityHideOpacityClose(bool bVisibility)
{
    m_pUi->m_pushButton_close->setVisible(bVisibility);
    m_pUi->m_horizontalSlider_opacity->setVisible(bVisibility);
    m_pUi->m_label_opacity->setVisible(bVisibility);
}

//=============================================================================================================

void QuickControlView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/QuickControlView/ViewOpacity"), getOpacityValue());
    settings.setValue(m_sSettingsPath + QString("/QuickControlView/ViewPos"), this->pos());
}

//=============================================================================================================

void QuickControlView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    setOpacityValue(settings.value(m_sSettingsPath + QString("/QuickControlView/ViewOpacity"), 100).toInt());

    QPoint pos = settings.value(m_sSettingsPath + QString("/QuickControlView/ViewPos"), QPoint(100,100)).toPoint();

    QRect screenRect = QApplication::primaryScreen()->geometry();
    if(!screenRect.contains(pos) && QGuiApplication::screens().size() == 1) {
        move(QPoint(100,100));
    } else {
        move(pos);
    }
}

//=============================================================================================================

void QuickControlView::onOpacityChange(qint32 value)
{
    if(value <= 0) {
        this->setWindowOpacity(1);
    } else {
        this->setWindowOpacity(1/(100.0/value));
    }
}
