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

#include <rtprocessing/helpers/filterkernel.h>

#include <disp/viewers/scalingview.h>

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
    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());

    QScrollArea* wrappedScrollArea = new QScrollArea(pControlDock);
    QVBoxLayout* pLayout = new QVBoxLayout;

    //Scaling Widget
    QLabel* title_scaling = new QLabel();
    title_scaling->setTextFormat(Qt::RichText);
    title_scaling->setText("<b>Channel Scaling</b>");

    m_pScalingWidget = new ScalingView("MNEANALYZE", wrappedScrollArea);
    connect(this, &RawDataViewer::guiModeChanged,
            m_pScalingWidget.data(), &ScalingView::setGuiMode);
    pLayout->addWidget(title_scaling);
    pLayout->addWidget(m_pScalingWidget);

    //View Widget
    QLabel* title_viewsettings = new QLabel();
    title_viewsettings->setTextFormat(Qt::RichText);
    title_viewsettings->setText("<b>View Settings</b>");

    m_pSettingsViewWidget = new FiffRawViewSettings("MNEANALYZE", wrappedScrollArea);
    connect(this, &RawDataViewer::guiModeChanged,
            m_pSettingsViewWidget.data(), &FiffRawViewSettings::setGuiMode);
    pLayout->addWidget(title_viewsettings);
    pLayout->addWidget(m_pSettingsViewWidget);

    QSpacerItem* endSpacer = new QSpacerItem(1,
                                             1,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::Expanding);
    pLayout->addSpacerItem(endSpacer);
    wrappedScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                                 QSizePolicy::Preferred));

    wrappedScrollArea->setLayout(pLayout);

    pControlDock->setWidget(wrappedScrollArea);
    pControlDock->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Preferred));

    return pControlDock;
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
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
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
    if(m_pScalingWidget && m_pSettingsViewWidget) {
        // Setup scaling widget
        connect(m_pScalingWidget.data(), &DISPLIB::ScalingView::scalingChanged,
                m_pFiffRawView.data(), &FiffRawView::setScalingMap, Qt::UniqueConnection);

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
        m_pFiffRawView->setScalingMap(m_pScalingWidget->getScaleMap());
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
