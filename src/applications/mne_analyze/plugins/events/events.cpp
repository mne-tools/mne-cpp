//=============================================================================================================
/**
 * @file     events.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.0
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Lorenz Esch, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Definition of the Events class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events.h"

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EVENTSPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Events::Events()
{

}

//=============================================================================================================

Events::~Events()
{

}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Events::clone() const
{
    QSharedPointer<Events> pEventsClone(new Events);
    return pEventsClone;
}

//=============================================================================================================

void Events::init()
{
    m_pCommu = QSharedPointer<ANSHAREDLIB::Communicator>(new ANSHAREDLIB::Communicator(this));
}

//=============================================================================================================

void Events::unload()
{

}

//=============================================================================================================

QString Events::getName() const
{
    return "Events";
}

//=============================================================================================================

QMenu *Events::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *Events::getControl()
{
    EventView* pEventView = new EventView();
    pEventView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                       QSizePolicy::Preferred));

    connect(pEventView, &EventView::triggerRedraw,
            this, &Events::onTriggerRedraw, Qt::UniqueConnection);

    connect(pEventView, &EventView::eventsUpdated,
            this, &Events::onEventsUpdated, Qt::UniqueConnection);

    connect(pEventView, &EventView::activeEventsChecked,
            this, &Events::toggleDisplayEvent, Qt::UniqueConnection);

    connect(pEventView, &EventView::jumpToSelected,
            this, &Events::onJumpToSelected, Qt::UniqueConnection);

    connect(this, &Events::newEventAvailable,
            pEventView, &EventView::addEventToModel, Qt::UniqueConnection);

    connect(this, &Events::disconnectFromModel,
            pEventView, &EventView::disconnectFromModel, Qt::UniqueConnection);

    connect(this, &Events::newEventModelAvailable,
            pEventView, &EventView::setModel, Qt::UniqueConnection);

    connect(this, &Events::newFiffRawViewModel,
            pEventView, &EventView::onNewFiffRawViewModel, Qt::UniqueConnection);

    connect(pEventView, &EventView::loadingStart,
            this, &Events::triggerLoadingStart, Qt::DirectConnection);

    connect(pEventView, &EventView::loadingEnd,
            this, &Events::triggerLoadingEnd, Qt::DirectConnection);

    connect(this, &Events::clearView,
            pEventView, &EventView::clearView, Qt::UniqueConnection);

    QDockWidget* pControl = new QDockWidget(getName());
    pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    pControl->setWidget(pEventView);
    pControl->setObjectName(getName());
    pControl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));

    return pControl;
}

//=============================================================================================================

QWidget *Events::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void Events::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::NEW_EVENT_ADDED:
        emit newEventAvailable(e->getData().toInt());
        onTriggerRedraw();
        break;
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    case EVENT_TYPE::MODEL_REMOVED:
        onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
        break;
    default:
        qWarning() << "[Events::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> Events::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(NEW_EVENT_ADDED);
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(MODEL_REMOVED);

    return temp;
}

//=============================================================================================================

void Events::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        emit disconnectFromModel();
        FiffRawViewModel::SPtr pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);

        if(pFiffRawModel->hasEventModel()){
            emit newEventModelAvailable(pFiffRawModel->getEventModel());
        } else {
            QSharedPointer<EventModel> pEventModel = QSharedPointer<EventModel>::create(pFiffRawModel);
            if (pFiffRawModel->isRealtime()){
                pEventModel->setSharedMemory(true);
            }
            emit newEventModelAvailable(pEventModel);
            m_pAnalyzeData->addModel<ANSHAREDLIB::EventModel>(pEventModel,
                                                                   "Events");
        }
        emit newFiffRawViewModel(pFiffRawModel);
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_EVENT_MODEL) {
        emit disconnectFromModel();
        EventModel::SPtr pEventModel = qSharedPointerCast<EventModel>(pNewModel);
        emit newEventModelAvailable(pEventModel);
    }
}

//=============================================================================================================

void Events::toggleDisplayEvent(int iToggle)
{
    QVariant data;
    data.setValue(iToggle);
    m_pCommu->publishEvent(EVENT_TYPE::TRIGGER_ACTIVE_CHANGED, data);
}

//=============================================================================================================

void Events::onTriggerRedraw()
{
    m_pCommu->publishEvent(TRIGGER_REDRAW);
}

//=============================================================================================================

void Events::onJumpToSelected()
{
    m_pCommu->publishEvent(TRIGGER_VIEWER_MOVE);
}

//=============================================================================================================

void Events::onEventsUpdated()
{
    m_pCommu->publishEvent(EVENTS_UPDATED);
}

//=============================================================================================================

void Events::triggerLoadingStart(const QString& sMessage)
{
    m_pCommu->publishEvent(LOADING_START, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void Events::triggerLoadingEnd(const QString& sMessage)
{
    m_pCommu->publishEvent(LOADING_END, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void Events::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_EVENT_MODEL || pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        emit clearView(pRemovedModel);
    }
}

//=============================================================================================================

QString Events::getBuildInfo()
{
    return QString(EVENTSPLUGIN::buildDateTime()) + QString(" - ")  + QString(EVENTSPLUGIN::buildHash());
}
