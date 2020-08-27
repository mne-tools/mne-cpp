//=============================================================================================================
/**
 * @file     controlmanager.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the ControlManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "controlmanager.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>

#include <disp/viewers/scalingview.h>
#include <disp/viewers/applytoview.h>
#include <disp/viewers/fiffrawviewsettings.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONTROLMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ControlManager::ControlManager()
: m_pScalingParameters(new ANSHAREDLIB::ScalingParameters)
, m_pViewParmeters(new ANSHAREDLIB::ViewParameters)
{
}

//=============================================================================================================

ControlManager::~ControlManager()
{
    if(m_pScalingParameters){
        delete m_pScalingParameters;
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> ControlManager::clone() const
{
    QSharedPointer<ControlManager> pControlManagerClone = QSharedPointer<ControlManager>::create();
    return pControlManagerClone;
}

//=============================================================================================================

void ControlManager::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void ControlManager::unload()
{
}

//=============================================================================================================

QString ControlManager::getName() const
{
    return "Scaling";
}

//=============================================================================================================

QMenu *ControlManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *ControlManager::getControl()
{
    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());
    QTabWidget* pTabWidget = new QTabWidget();

    QScrollArea* wrappedScrollArea = new QScrollArea(pControlDock);
    QVBoxLayout* pLayout = new QVBoxLayout;

    DISPLIB::ScalingView* pScalingWidget = new DISPLIB::ScalingView("MNEANALYZE", wrappedScrollArea);
    DISPLIB::FiffRawViewSettings* pFiffViewSettings = new DISPLIB::FiffRawViewSettings("MNEANALYZE", wrappedScrollArea);
    m_pApplyToView = new DISPLIB::ApplyToView();

    pTabWidget->addTab(pScalingWidget, "Scaling");
    pTabWidget->addTab(pFiffViewSettings, "Controls");

    pLayout->addWidget(pTabWidget);
    pLayout->addWidget(m_pApplyToView);
    pLayout->addStretch();
    wrappedScrollArea->setLayout(pLayout);
    pControlDock->setWidget(wrappedScrollArea);

    connect(pScalingWidget, &DISPLIB::ScalingView::scalingChanged,
            this, &ControlManager::onScalingChanged, Qt::UniqueConnection);

    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::signalColorChanged,
            this, &ControlManager::onSignalColorChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
            this, &ControlManager::onBackgroundColorChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::zoomChanged,
            this, &ControlManager::onZoomChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::timeWindowChanged,
            this, &ControlManager::onTimeWindowChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::distanceTimeSpacerChanged,
            this, &ControlManager::onDistanceTimeSpacerChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &DISPLIB::FiffRawViewSettings::makeScreenshot,
            this, &ControlManager::onMakeScreenshot, Qt::UniqueConnection);


    m_pScalingParameters->m_mScalingMap = pScalingWidget->getScaleMap();
    m_pScalingParameters->m_mScalingMap.detach();

    m_pViewParmeters->m_colorSignal = pFiffViewSettings->getSignalColor();
    m_pViewParmeters->m_colorBackground = pFiffViewSettings->getBackgroundColor();
    m_pViewParmeters->m_dZoomValue = pFiffViewSettings->getZoom();
    m_pViewParmeters->m_iTimeWindow = pFiffViewSettings->getWindowSize();
    m_pViewParmeters->m_iTimeSpacers = pFiffViewSettings->getDistanceTimeSpacer();
    m_pViewParmeters->m_sImageType = "";

    return pControlDock;
}

//=============================================================================================================

QWidget *ControlManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void ControlManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case SELECTED_MODEL_CHANGED:
        onScalingChanged(m_pScalingParameters->m_mScalingMap);
        break;
    default:
        qWarning() << "[ControlManager::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ControlManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void ControlManager::onScalingChanged(const QMap<qint32, float> &scalingMap)
{
    m_pScalingParameters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pScalingParameters->m_mScalingMap = scalingMap;
    m_pScalingParameters->m_mScalingMap.detach();

    m_pCommu->publishEvent(EVENT_TYPE::SCALING_MAP_CHANGED, QVariant::fromValue(m_pScalingParameters));
}

//=============================================================================================================
void ControlManager::onSignalColorChanged(const QColor& signalColor)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}

//=============================================================================================================
void ControlManager::onBackgroundColorChanged(const QColor& backgroundColor)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}

//=============================================================================================================
void ControlManager::onZoomChanged(double value)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}

//=============================================================================================================
void ControlManager::onTimeWindowChanged(int value)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}

//=============================================================================================================
void ControlManager::onDistanceTimeSpacerChanged(int value)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}

//=============================================================================================================
void ControlManager::onMakeScreenshot(const QString& imageType)
{
    m_pViewParmeters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_pViewParmeters));
}
