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
#include <anShared/Management/analyzedata.h>
#include <anShared/Utils/metatypes.h>
#include <anShared/Management/communicator.h>
#include "fiffrawview.h"
#include "fiffrawviewdelegate.h"
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/scalingview.h>
#include "../dataloader/dataloader.h"
#include <anShared/Model/annotationmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSpacerItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawDataViewer::RawDataViewer()
: m_iVisibleBlocks(10)
, m_iBufferBlocks(10)
, m_pRawDelegate(QSharedPointer<FiffRawViewDelegate>::create())
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
}

//=============================================================================================================

void RawDataViewer::onModelChanged(QSharedPointer<AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pRawModel) {
            if(m_pRawModel == pNewModel) {
                return;
            }
        }

        m_pRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        m_pFiffRawView->initMVCSettings(m_pRawModel, m_pRawDelegate);

        setUpControls();
    }
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
    if(!m_pControlDock) {
        m_pControlDock = new QDockWidget(getName());
        m_pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        m_pLayout = new QVBoxLayout;
        m_pContainer = new QWidget;

        QLabel* tempLabel = new QLabel("No file loaded.");
        m_pControlDock->setWidget(tempLabel);
    }

    return m_pControlDock;
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
        m_pFiffRawView->updateView();
        break;
    case TRIGGER_VIEWER_MOVE:
        m_pFiffRawView->updateScrollPosition();
        break;
    case SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
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
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void RawDataViewer::setUpControls()
{
    QLabel* tempLabel = new QLabel("Loading file");
    m_pControlDock->setWidget(tempLabel);

    disconnectOld();

    delete m_pLayout;
    delete m_pContainer;
    m_pLayout = new QVBoxLayout;
    m_pContainer = new QWidget;

    //Scaling Widget
    DISPLIB::ScalingView* scalingWidget = new DISPLIB::ScalingView("Test", m_pRawModel->getFiffInfo()->chs);
    connect(scalingWidget, &DISPLIB::ScalingView::scalingChanged,
            m_pFiffRawView.data(), &FiffRawView::setScalingMap);

    //View Widget
    DISPLIB::FiffRawViewSettings* viewWidget = new DISPLIB::FiffRawViewSettings("Test");
    viewWidget->setWidgetList();
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::signalColorChanged,
            m_pFiffRawView.data(), &FiffRawView::setSignalColor);
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
            m_pFiffRawView.data(), &FiffRawView::setBackgroundColor);
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::zoomChanged,
            m_pFiffRawView.data(), &FiffRawView::setZoom);
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::timeWindowChanged,
            m_pFiffRawView.data(), &FiffRawView::setWindowSize);
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::distanceTimeSpacerChanged,
            m_pFiffRawView.data(), &FiffRawView::setDistanceTimeSpacer);
    connect(viewWidget, &DISPLIB::FiffRawViewSettings::makeScreenshot,
            m_pFiffRawView.data(), &FiffRawView::onMakeScreenshot);

    //Init view widget (preserved between session)
    m_pFiffRawView->setWindowSize(viewWidget->getWindowSize());
    m_pFiffRawView->setSignalColor(viewWidget->getSignalColor());
    m_pFiffRawView->setBackgroundColor(viewWidget->getBackgroundColor());
    m_pFiffRawView->setZoom(viewWidget->getZoom());
    m_pFiffRawView->setDistanceTimeSpacer(viewWidget->getDistanceTimeSpacer());

    connect(m_pFiffRawView.data(), &FiffRawView::sendSamplePos,
            this, &RawDataViewer::onSendSamplePos);

    //Set up layout w/ control widgets
    m_pLayout->addWidget(scalingWidget);
    m_pLayout->addWidget(viewWidget);
    m_pLayout->addStretch();

    //Make it all visible to the user
    m_pControlDock->setWidget(m_pContainer);
    m_pContainer->setLayout(m_pLayout);
    m_pContainer->show();

    delete tempLabel;
}

//=============================================================================================================

void RawDataViewer::onSendSamplePos(int iSample)
{
    QVariant data;
    data.setValue(iSample);

    m_pCommu->publishEvent(EVENT_TYPE::NEW_ANNOTATION_ADDED, data);
}

//=============================================================================================================

void RawDataViewer::disconnectOld()
{
    disconnect(m_pFiffRawView.data(), &FiffRawView::sendSamplePos,
            this, &RawDataViewer::onSendSamplePos);

//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::signalColorChanged,
//            m_pFiffRawView.data(), &FiffRawView::setSignalColor);
//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
//            m_pFiffRawView.data(), &FiffRawView::setBackgroundColor);
//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::zoomChanged,
//            m_pFiffRawView.data(), &FiffRawView::setZoom);
//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::timeWindowChanged,
//            m_pFiffRawView.data(), &FiffRawView::setWindowSize);
//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::distanceTimeSpacerChanged,
//            m_pFiffRawView.data(), &FiffRawView::setDistanceTimeSpacer);
//    connect(viewWidget, &DISPLIB::FiffRawViewSettings::makeScreenshot,
//            m_pFiffRawView.data(), &FiffRawView::onMakeScreenshot);
}
