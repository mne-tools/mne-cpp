//=============================================================================================================
/**
 * @file     annotationmanager.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the AnnotationManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmanager.h"

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

using namespace ANNOTATIONMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationManager::AnnotationManager()
{

}

//=============================================================================================================

AnnotationManager::~AnnotationManager()
{

}

//=============================================================================================================

QSharedPointer<IPlugin> AnnotationManager::clone() const
{
    QSharedPointer<AnnotationManager> pAnnotationManagerClone(new AnnotationManager);
    return pAnnotationManagerClone;
}

//=============================================================================================================

void AnnotationManager::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void AnnotationManager::unload()
{

}

//=============================================================================================================

QString AnnotationManager::getName() const
{
    return "Events";
}

//=============================================================================================================

QMenu *AnnotationManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *AnnotationManager::getControl()
{
    AnnotationSettingsView* pAnnotationSettingsView = new AnnotationSettingsView();
    pAnnotationSettingsView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                       QSizePolicy::Preferred));

    connect(pAnnotationSettingsView, &AnnotationSettingsView::triggerRedraw,
            this, &AnnotationManager::onTriggerRedraw, Qt::UniqueConnection);

    connect(pAnnotationSettingsView, &AnnotationSettingsView::groupsUpdated,
            this, &AnnotationManager::onGroupsUpdated, Qt::UniqueConnection);

    connect(pAnnotationSettingsView, &AnnotationSettingsView::activeEventsChecked,
            this, &AnnotationManager::toggleDisplayEvent, Qt::UniqueConnection);

    connect(pAnnotationSettingsView, &AnnotationSettingsView::jumpToSelected,
            this, &AnnotationManager::onJumpToSelected, Qt::UniqueConnection);

    connect(this, &AnnotationManager::newAnnotationAvailable,
            pAnnotationSettingsView, &AnnotationSettingsView::addAnnotationToModel, Qt::UniqueConnection);

    connect(this, &AnnotationManager::disconnectFromModel,
            pAnnotationSettingsView, &AnnotationSettingsView::disconnectFromModel, Qt::UniqueConnection);

    connect(this, &AnnotationManager::newAnnotationModelAvailable,
            pAnnotationSettingsView, &AnnotationSettingsView::setModel, Qt::UniqueConnection);

    connect(m_pAnalyzeData.data(), &AnalyzeData::modelIsEmpty,
            pAnnotationSettingsView, &AnnotationSettingsView::reset, Qt::UniqueConnection);

    connect(this, &AnnotationManager::newFiffRawViewModel,
            pAnnotationSettingsView, &AnnotationSettingsView::onNewFiffRawViewModel, Qt::UniqueConnection);

    connect(pAnnotationSettingsView, &AnnotationSettingsView::loadingStart,
            this, &AnnotationManager::triggerLoadingStart, Qt::DirectConnection);

    connect(pAnnotationSettingsView, & AnnotationSettingsView::loadingEnd,
            this, &AnnotationManager::triggerLoadingEnd, Qt::DirectConnection);

    QDockWidget* pControl = new QDockWidget(getName());
    pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    pControl->setWidget(pAnnotationSettingsView);
    pControl->setObjectName(getName());
    pControl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));

    return pControl;
}

//=============================================================================================================

QWidget *AnnotationManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void AnnotationManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::NEW_ANNOTATION_ADDED:
            emit newAnnotationAvailable(e->getData().toInt());
            onTriggerRedraw();
            break;
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            if(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >()->getType() == ANSHAREDLIB_FIFFRAW_MODEL) {
                onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
            }
            break;
        default:
            qWarning() << "[AnnotationManager::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> AnnotationManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(NEW_ANNOTATION_ADDED);
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void AnnotationManager::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        emit disconnectFromModel();
        FiffRawViewModel::SPtr pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);

        emit newAnnotationModelAvailable(pFiffRawModel->getAnnotationModel());
        emit newFiffRawViewModel(pFiffRawModel);
    }
}

//=============================================================================================================

void AnnotationManager::toggleDisplayEvent(const int& iToggle)
{
    QVariant data;
    data.setValue(iToggle);
    m_pCommu->publishEvent(EVENT_TYPE::TRIGGER_ACTIVE_CHANGED, data);
}

//=============================================================================================================

void AnnotationManager::onTriggerRedraw()
{
    m_pCommu->publishEvent(TRIGGER_REDRAW);
}

//=============================================================================================================

void AnnotationManager::onJumpToSelected()
{
    m_pCommu->publishEvent(TRIGGER_VIEWER_MOVE);
}

//=============================================================================================================

void AnnotationManager::onGroupsUpdated()
{
    m_pCommu->publishEvent(EVENT_GROUPS_UPDATED);
}

//=============================================================================================================

void AnnotationManager::triggerLoadingStart(const QString& sMessage)
{
    m_pCommu->publishEvent(LOADING_START, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void AnnotationManager::triggerLoadingEnd(const QString& sMessage)
{
    m_pCommu->publishEvent(LOADING_END, QVariant::fromValue(sMessage));
}
