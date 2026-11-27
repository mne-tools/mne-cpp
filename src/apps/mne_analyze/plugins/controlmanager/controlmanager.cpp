//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     controlmanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     August, 2020
 * @brief    Definition of the ControlManager class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "controlmanager.h"

#include <mne/base/log.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <mne/viz/backends/qt/viewers/scalingview.h>
#include <mne/viz/backends/qt/viewers/applytoview.h>
#include <mne/viz/backends/qt/viewers/control3dview.h>
#include <mne/viz/backends/qt/viewers/fiffrawviewsettings.h>
#ifdef MNE_VIZ_QT_3D
#include <mne/viz/backends/qt/3d/model/braintreemodel.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONTROLMANAGERPLUGIN;
using namespace mne::analyze;
using namespace mne::viz;
using namespace mne::viz::qt;

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

QMenu* ControlManager::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QDockWidget* ControlManager::getControl()
{
    auto pControlDock = std::make_unique<QDockWidget>(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());

    QScrollArea* wrappedScrollArea = new QScrollArea(pControlDock.get());
    QVBoxLayout* pLayout = new QVBoxLayout(wrappedScrollArea);
    QTabWidget* pTabWidget = new QTabWidget(wrappedScrollArea);

    const std::vector<std::string> slControlFlags{"Data", "View", "Light"};

    //Scaling
    mne::viz::qt::ScalingView* pScalingWidget = new mne::viz::qt::ScalingView("MNEANALYZE", wrappedScrollArea);
    pTabWidget->addTab(pScalingWidget, "Scaling");

    connect(pScalingWidget, &mne::viz::qt::ScalingView::scalingChanged,
            this, &ControlManager::onScalingChanged, Qt::UniqueConnection);

    m_ScalingParameters.m_mScalingMap = pScalingWidget->getScaleMap();

    //View Settings
    mne::viz::qt::FiffRawViewSettings* pFiffViewSettings = new mne::viz::qt::FiffRawViewSettings("MNEANALYZE", wrappedScrollArea);

    pTabWidget->addTab(pFiffViewSettings, "Controls");

    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::signalColorChanged,
            this, &ControlManager::onSignalColorChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::backgroundColorChanged,
            this, &ControlManager::onBackgroundColorChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::zoomChanged,
            this, &ControlManager::onZoomChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::timeWindowChanged,
            this, &ControlManager::onTimeWindowChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::distanceTimeSpacerChanged,
            this, &ControlManager::onDistanceTimeSpacerChanged, Qt::UniqueConnection);
    connect(pFiffViewSettings, &mne::viz::qt::FiffRawViewSettings::makeScreenshot,
            this, &ControlManager::onMakeScreenshot, Qt::UniqueConnection);

    m_ViewParameters.m_colorSignal = pFiffViewSettings->getSignalColor();
    m_ViewParameters.m_colorBackground = pFiffViewSettings->getBackgroundColor();
    m_ViewParameters.m_dZoomValue = pFiffViewSettings->getZoom();
    m_ViewParameters.m_iTimeWindow = pFiffViewSettings->getWindowSize();
    m_ViewParameters.m_iTimeSpacers = pFiffViewSettings->getDistanceTimeSpacer();
    m_ViewParameters.m_sImageType = "";

    //View3D Settings
    m_pControl3DView = new mne::viz::qt::Control3DView(QString("MNEANALYZE/%1").arg(this->getName()), pTabWidget, slControlFlags);

    pTabWidget->addTab(m_pControl3DView, "3D");

    connect(m_pControl3DView, &mne::viz::qt::Control3DView::sceneColorChanged,
            this, &ControlManager::onSceneColorChange);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::rotationChanged,
            this, &ControlManager::onRotationChanged);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::showCoordAxis,
            this, &ControlManager::onShowCoordAxis);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::showFullScreen,
            this, &ControlManager::onShowFullScreen);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::lightColorChanged,
            this, &ControlManager::onLightColorChanged);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::lightIntensityChanged,
            this, &ControlManager::onLightIntensityChanged);
    connect(m_pControl3DView, &mne::viz::qt::Control3DView::takeScreenshotChanged,
            this, &ControlManager::onTakeScreenshotChanged);

    m_pApplyToView = new mne::viz::qt::ApplyToView("", wrappedScrollArea);
    pLayout->addWidget(pTabWidget);
    pLayout->addWidget(m_pApplyToView);
    pLayout->addStretch();
    pControlDock->setWidget(wrappedScrollArea);

    return pControlDock.release();
}

//=============================================================================================================

QWidget* ControlManager::getView()
{
    return nullptr;
}

//=============================================================================================================

void ControlManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            if (m_pApplyToView && e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>()->getType() != ANSHAREDLIB_BEMDATA_MODEL) {
                onScalingChanged(m_ScalingParameters.m_mScalingMap);
                m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
                m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::all;
                m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
            }
            break;

#ifdef MNE_VIZ_QT_3D
        case EVENT_TYPE::SET_DATA3D_TREE_MODEL:
            init3DGui(e->getData().value<QSharedPointer<BrainTreeModel>>());
            break;
#endif
        default:
            mne::Log::warning("[ControlManager::handleEvent] received an Event that is not handled by switch-cases");
            break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ControlManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
#ifdef MNE_VIZ_QT_3D
    temp.push_back(SET_DATA3D_TREE_MODEL);
#endif

    return temp;
}

//=============================================================================================================

void ControlManager::onScalingChanged(const std::map<int, float>& scalingMap)
{
    m_ScalingParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_ScalingParameters.m_mScalingMap = scalingMap;

    m_pCommu->publishEvent(EVENT_TYPE::SCALING_MAP_CHANGED, QVariant::fromValue(m_ScalingParameters));
}

//=============================================================================================================

void ControlManager::onSignalColorChanged(const QColor& signalColor)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::signal;
    m_ViewParameters.m_colorSignal = signalColor;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onBackgroundColorChanged(const QColor& backgroundColor)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::background;
    m_ViewParameters.m_colorBackground = backgroundColor;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onZoomChanged(double dZoomValue)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::zoom;
    m_ViewParameters.m_dZoomValue = dZoomValue;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onTimeWindowChanged(int iTimeWindow)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::window;
    m_ViewParameters.m_iTimeWindow = iTimeWindow;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onDistanceTimeSpacerChanged(int iSpacerDistance)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::spacer;
    m_ViewParameters.m_iTimeSpacers = iSpacerDistance;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

void ControlManager::onMakeScreenshot(const QString& imageType)
{
    m_ViewParameters.m_sViewsToApply = m_pApplyToView->getSelectedViews();
    m_ViewParameters.m_sSettingsToApply = mne::analyze::ViewParameters::ViewSetting::screenshot;
    m_ViewParameters.m_sImageType = imageType;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW_SETTINGS_CHANGED, QVariant::fromValue(m_ViewParameters));
}

//=============================================================================================================

#ifdef MNE_VIZ_QT_3D
void ControlManager::init3DGui(QSharedPointer<BrainTreeModel> pModel)
{
    if (m_pControl3DView)
        m_pControl3DView->setModel(pModel.data());
}
#endif

//=============================================================================================================

void ControlManager::onSceneColorChange(const QColor& color)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::sceneColor;
    m_View3DParameters.m_sceneColor = color;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onRotationChanged(bool bRotationChanged)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::rotation;
    m_View3DParameters.m_bToggleRotation = bRotationChanged;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onShowCoordAxis(bool bShowCoordAxis)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::coordAxis;
    m_View3DParameters.m_bToogleCoordAxis = bShowCoordAxis;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onShowFullScreen(bool bShowFullScreen)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::fullscreen;
    m_View3DParameters.m_bToggleFullscreen = bShowFullScreen;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onLightColorChanged(const QColor& color)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::lightColor;
    m_View3DParameters.m_lightColor = color;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onLightIntensityChanged(double value)
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::lightIntensity;
    m_View3DParameters.m_dLightIntensity = value;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

void ControlManager::onTakeScreenshotChanged()
{
    m_View3DParameters.m_settingsToApply = mne::analyze::View3DParameters::View3DSetting::screenshot;

    m_pCommu->publishEvent(EVENT_TYPE::VIEW3D_SETTINGS_CHANGED, QVariant::fromValue(m_View3DParameters));
}

//=============================================================================================================

QString ControlManager::getBuildInfo()
{
    return QString(CONTROLMANAGERPLUGIN::buildDateTime()) + QString(" - ") + QString(CONTROLMANAGERPLUGIN::buildHash());
}
