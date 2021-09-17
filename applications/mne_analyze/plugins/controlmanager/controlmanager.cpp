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
#include <disp/viewers/control3dview.h>
#include <disp/viewers/fiffrawviewsettings.h>

#ifndef WASMBUILD
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/delegate/data3Dtreedelegate.h>
#endif

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
{
}

//=============================================================================================================

ControlManager::~ControlManager()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> ControlManager::clone() const
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
    return "Settings";
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

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";

    //Scaling
    DISPLIB::ScalingView* pScalingWidget = new DISPLIB::ScalingView("MNEANALYZE", wrappedScrollArea);
    pTabWidget->addTab(pScalingWidget, "Scaling");

    connect(pScalingWidget, &DISPLIB::ScalingView::scalingChanged,
            this, &ControlManager::onScalingChanged, Qt::UniqueConnection);

    m_ScalingParameters.m_mScalingMap = pScalingWidget->getScaleMap();
    m_ScalingParameters.m_mScalingMap.detach();

    //View Settings
    DISPLIB::FiffRawViewSettings* pFiffViewSettings = new DISPLIB::FiffRawViewSettings("MNEANALYZE", wrappedScrollArea);

    pTabWidget->addTab(pFiffViewSettings, "Controls");

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

    m_ViewParameters.m_colorSignal = pFiffViewSettings->getSignalColor();
    m_ViewParameters.m_colorBackground = pFiffViewSettings->getBackgroundColor();
    m_ViewParameters.m_dZoomValue = pFiffViewSettings->getZoom();
    m_ViewParameters.m_iTimeWindow = pFiffViewSettings->getWindowSize();
    m_ViewParameters.m_iTimeSpacers = pFiffViewSettings->getDistanceTimeSpacer();
    m_ViewParameters.m_sImageType = "";

    #ifndef WASMBUILD
    //View3D Settings
    m_pControl3DView = new DISPLIB::Control3DView(QString("MNEANALYZE/%1").arg(this->getName()), Q_NULLPTR, slControlFlags);
    DISP3DLIB::Data3DTreeDelegate* pData3DTreeDelegate = new DISP3DLIB::Data3DTreeDelegate(this);

    pTabWidget->addTab(m_pControl3DView, "3D");

    m_pControl3DView->setDelegate(pData3DTreeDelegate);

    connect(m_pControl3DView, &DISPLIB::Control3DView::sceneColorChanged,
            this, &ControlManager::onSceneColorChange);
    connect(m_pControl3DView, &DISPLIB::Control3DView::rotationChanged,
            this, &ControlManager::onRotationChanged);
    connect(m_pControl3DView, &DISPLIB::Control3DView::showCoordAxis,
            this, &ControlManager::onShowCoordAxis);
    connect(m_pControl3DView, &DISPLIB::Control3DView::showFullScreen,
            this, &ControlManager::onShowFullScreen);
    connect(m_pControl3DView, &DISPLIB::Control3DView::lightColorChanged,
            this, &ControlManager::onLightColorChanged);
    connect(m_pControl3DView, &DISPLIB::Control3DView::lightIntensityChanged,
            this, &ControlManager::onLightIntensityChanged);
    connect(m_pControl3DView, &DISPLIB::Control3DView::takeScreenshotChanged,
            this, &ControlManager::onTakeScreenshotChanged);
    #endif

    m_pApplyToView = new DISPLIB::ApplyToView();
    pLayout->addWidget(pTabWidget);
    pLayout->addWidget(m_pApplyToView);
    pLayout->addStretch();
    wrappedScrollArea->setLayout(pLayout);
    pControlDock->setWidget(wrappedScrollArea);

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
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        if(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >()->getType() != ANSHAREDLIB_BEMDATA_MODEL) {
            onScalingChanged(m_ScalingParameters.m_mScalingMap);
            m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
            m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::all;
            m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
        }
        break;

    case EVENT_TYPE::SET_DATA3D_TREE_MODEL:
        #ifndef WASMBUILD
        init3DGui(e->getData().value<QSharedPointer<DISP3DLIB::Data3DTreeModel>>());
        #endif
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
    temp.push_back(SET_DATA3D_TREE_MODEL);

    return temp;
}

//=============================================================================================================

void ControlManager::onScalingChanged(const QMap<qint32, float> &scalingMap)
{
    m_ScalingParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_ScalingParameters.m_mScalingMap = scalingMap;
    m_ScalingParameters.m_mScalingMap.detach();

    m_pCommu->publishEvent(EVENT_TYPE::SCALING_MAP_CHANGED, QVariant::fromValue(m_ScalingParameters));
}

//=============================================================================================================

void ControlManager::onSignalColorChanged(const QColor& signalColor)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::signal;
    m_ViewParameters.m_colorSignal = signalColor;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onBackgroundColorChanged(const QColor& backgroundColor)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::background;
    m_ViewParameters.m_colorBackground = backgroundColor;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onZoomChanged(double dZoomValue)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::zoom;
    m_ViewParameters.m_dZoomValue = dZoomValue;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onTimeWindowChanged(int iTimeWindow)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::window;
    m_ViewParameters.m_iTimeWindow = iTimeWindow;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onDistanceTimeSpacerChanged(int iSpacerDistance)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::spacer;
    m_ViewParameters.m_iTimeSpacers = iSpacerDistance;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onMakeScreenshot(const QString& imageType)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = ANSHAREDLIB::ViewParameters::ViewSetting::screenshot;
    m_ViewParameters.m_sImageType = imageType;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

#ifndef WASMBUILD
void ControlManager::init3DGui(QSharedPointer<DISP3DLIB::Data3DTreeModel> pModel)
{
    m_pControl3DView->setModel(pModel.data());
}
#endif

//=============================================================================================================

void ControlManager::onSceneColorChange(const QColor &color)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::sceneColor;
    m_View3DParameters.m_sceneColor = color;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onRotationChanged(bool bRotationChanged)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::rotation;
    m_View3DParameters.m_bToggleRotation = bRotationChanged;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onShowCoordAxis(bool bShowCoordAxis)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::coordAxis;
    m_View3DParameters.m_bToogleCoordAxis = bShowCoordAxis;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onShowFullScreen(bool bShowFullScreen)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::fullscreen;
    m_View3DParameters.m_bToggleFullscreen = bShowFullScreen;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onLightColorChanged(const QColor &color)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::lightColor;
    m_View3DParameters.m_lightColor = color;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onLightIntensityChanged(double value)
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::lightIntensity;
    m_View3DParameters.m_dLightIntensity = value;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onTakeScreenshotChanged()
{
    m_View3DParameters.m_settingsToApply = ANSHAREDLIB::View3DParameters::View3DSetting::screenshot;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

QString ControlManager::getBuildInfo()
{
    return QString(CONTROLMANAGERPLUGIN::buildDateTime()) + QString(" - ")  + QString(CONTROLMANAGERPLUGIN::buildHash());
}
