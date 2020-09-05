//=============================================================================================================
/**
 * @file     rawdataviewer.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Definition of the RawDataViewer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer.h"
#include "fiffrawview.h"
#include "fiffrawviewdelegate.h"
#include "../dataloader/dataloader.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Utils/metatypes.h>
#include <anShared/Management/communicator.h>
#include <anShared/Model/annotationmodel.h>

#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/helpers/selectionsceneitem.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSpacerItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawDataViewer::RawDataViewer()
: m_iVisibleBlocks(10)
, m_iBufferBlocks(10)
, m_pFiffRawView(Q_NULLPTR)
{
}

//=============================================================================================================

RawDataViewer::~RawDataViewer()
{
}

//=============================================================================================================

QSharedPointer<IPlugin> RawDataViewer::clone() const
{
    QSharedPointer<RawDataViewer> pRawDataViewerClone = QSharedPointer<RawDataViewer>::create();
    return pRawDataViewerClone;
}

//=============================================================================================================

void RawDataViewer::init()
{
    m_pCommu = new Communicator(this);

    // Create viewer
    m_pFiffRawView = new FiffRawView();
    m_pFiffRawView->setMinimumSize(256, 256);
    m_pFiffRawView->setFocusPolicy(Qt::TabFocus);
    m_pFiffRawView->setAttribute(Qt::WA_DeleteOnClose, false);

    connect(m_pAnalyzeData.data(), &AnalyzeData::modelIsEmpty,
            this, &RawDataViewer::onModelIsEmpty);
}

//=============================================================================================================

void RawDataViewer::unload()
{
}

//=============================================================================================================

QString RawDataViewer::getName() const
{
    return "Signal Viewer";
}

//=============================================================================================================

QMenu *RawDataViewer::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *RawDataViewer::getControl()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *RawDataViewer::getView()
{
    return m_pFiffRawView;
}

//=============================================================================================================

void RawDataViewer::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case TRIGGER_REDRAW:
        if(m_pFiffRawView) {
            m_pFiffRawView->updateView();
        }
        break;
    case TRIGGER_VIEWER_MOVE:
        m_pFiffRawView->updateScrollPositionToAnnotation();
        break;
    case TRIGGER_ACTIVE_CHANGED:
        m_pFiffRawView->getModel()->toggleDispAnnotation(e->getData().toInt());
        m_pFiffRawView->updateView();
        break;
    case SELECTED_MODEL_CHANGED:
        if(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >()->getType() == ANSHAREDLIB_FIFFRAW_MODEL) {
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        }
        break;
    case FILTER_CHANNEL_TYPE_CHANGED:
        m_pFiffRawView->setFilterChannelType(e->getData().toString());
        break;
    case FILTER_ACTIVE_CHANGED:
        m_pFiffRawView->setFilterActive(e->getData().toBool());
        break;
    case FILTER_DESIGN_CHANGED:
        m_pFiffRawView->setFilter(e->getData().value<FilterKernel>());
        break;
    case CHANNEL_SELECTION_ITEMS:
        if (e->getData().value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("signalview")){
            if(e->getData().value<DISPLIB::SelectionItem*>()->m_bShowAll){
                m_pFiffRawView->showAllChannels();
            } else {
                m_pFiffRawView->showSelectedChannelsOnly(e->getData().value<DISPLIB::SelectionItem*>()->m_iChannelNumber);
            }
        }
        break;
    case SCALING_MAP_CHANGED:
        if(e->getData().value<ANSHAREDLIB::ScalingParameters>().m_sViewsToApply.contains("signalview")){
            m_pFiffRawView->setScalingMap(e->getData().value<ANSHAREDLIB::ScalingParameters>().m_mScalingMap);
        }
        break;
    case VIEW_SETTINGS_CHANGED:
        if(e->getData().value<ANSHAREDLIB::ViewParameters>().m_sViewsToApply.contains("signalview")){
            updateViewParameters(e->getData().value<ANSHAREDLIB::ViewParameters>());
        }
        break;
    default:
        qWarning() << "[RawDataViewer::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> RawDataViewer::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp = {};
    temp.push_back(TRIGGER_REDRAW);
    temp.push_back(TRIGGER_VIEWER_MOVE);
    temp.push_back(TRIGGER_ACTIVE_CHANGED);
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(FILTER_CHANNEL_TYPE_CHANGED);
    temp.push_back(FILTER_ACTIVE_CHANGED);
    temp.push_back(FILTER_DESIGN_CHANGED);
    temp.push_back(CHANNEL_SELECTION_ITEMS);
    temp.push_back(SCALING_MAP_CHANGED);
    temp.push_back(VIEW_SETTINGS_CHANGED);

    return temp;
}

//=============================================================================================================

void RawDataViewer::onModelIsEmpty()
{
    m_pFiffRawView->reset();
}

//=============================================================================================================

void RawDataViewer::onModelChanged(QSharedPointer<AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pFiffRawView->getModel() == pNewModel) {
            return;
        }

        if(!m_pFiffRawView->getDelegate()) {
            m_pFiffRawView->setDelegate(QSharedPointer<FiffRawViewDelegate>::create());
        }

        m_pFiffRawView->setModel(qSharedPointerCast<FiffRawViewModel>(pNewModel));

        updateControls();
    }
}

//=============================================================================================================

void RawDataViewer::updateControls()
{
    if(m_pSettingsViewWidget) {
        // Setup view settings widget
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::signalColorChanged,
                m_pFiffRawView.data(), &FiffRawView::setSignalColor, Qt::UniqueConnection);
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
                m_pFiffRawView.data(), &FiffRawView::setBackgroundColor, Qt::UniqueConnection);
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::zoomChanged,
                m_pFiffRawView.data(), &FiffRawView::setZoom, Qt::UniqueConnection);
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::timeWindowChanged,
                m_pFiffRawView.data(), &FiffRawView::setWindowSize, Qt::UniqueConnection);
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::distanceTimeSpacerChanged,
                m_pFiffRawView.data(), &FiffRawView::setDistanceTimeSpacer, Qt::UniqueConnection);
        connect(m_pSettingsViewWidget.data(), &DISPLIB::FiffRawViewSettings::makeScreenshot,
                m_pFiffRawView.data(), &FiffRawView::onMakeScreenshot, Qt::UniqueConnection);

        // Preserve settings between different file sessions
        m_pFiffRawView->setWindowSize(m_pSettingsViewWidget->getWindowSize());
        m_pFiffRawView->setSignalColor(m_pSettingsViewWidget->getSignalColor());
        m_pFiffRawView->setBackgroundColor(m_pSettingsViewWidget->getBackgroundColor());
        m_pFiffRawView->setZoom(m_pSettingsViewWidget->getZoom());
        m_pFiffRawView->setDistanceTimeSpacer(m_pSettingsViewWidget->getDistanceTimeSpacer());

        connect(m_pFiffRawView.data(), &FiffRawView::sendSamplePos,
                this, &RawDataViewer::onSendSamplePos, Qt::UniqueConnection);
    }
}

//=============================================================================================================

void RawDataViewer::onSendSamplePos(int iSample)
{
    QVariant data;
    data.setValue(iSample);

    m_pCommu->publishEvent(EVENT_TYPE::NEW_ANNOTATION_ADDED, data);
}

//=============================================================================================================

void RawDataViewer::updateViewParameters(ANSHAREDLIB::ViewParameters viewParameters)
{
    switch (viewParameters.m_sSettingsToApply){
        case ANSHAREDLIB::ViewParameters::ViewSetting::signal:
            m_pFiffRawView->setSignalColor(viewParameters.m_colorSignal);
            m_pFiffRawView->updateView();
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::background:
            m_pFiffRawView->setBackgroundColor(viewParameters.m_colorBackground);
            m_pFiffRawView->updateView();
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::zoom:
            m_pFiffRawView->setZoom(viewParameters.m_dZoomValue);
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::window:
            m_pFiffRawView->setWindowSize(viewParameters.m_iTimeWindow);
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::spacer:
            m_pFiffRawView->setDistanceTimeSpacer(viewParameters.m_iTimeSpacers);
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::screenshot:
            m_pFiffRawView->onMakeScreenshot(viewParameters.m_sImageType);
            break;
        case ANSHAREDLIB::ViewParameters::ViewSetting::all:
            m_pFiffRawView->setSignalColor(viewParameters.m_colorSignal);
            m_pFiffRawView->setBackgroundColor(viewParameters.m_colorBackground);
            m_pFiffRawView->setZoom(viewParameters.m_dZoomValue);
            m_pFiffRawView->setWindowSize(viewParameters.m_iTimeWindow);
            m_pFiffRawView->setDistanceTimeSpacer(viewParameters.m_iTimeSpacers);
            m_pFiffRawView->onMakeScreenshot(viewParameters.m_sImageType);
        break;
        default:
            qDebug() << "Unknown setting";
    }
}
