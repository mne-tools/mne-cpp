//=============================================================================================================
/**
 * @file     datamanager.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the DataManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamanager.h"
#include <disp/viewers/bidsview.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DATAMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataManager::DataManager()
{
    m_iOrder = -10;
}

//=============================================================================================================

DataManager::~DataManager()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> DataManager::clone() const
{
    QSharedPointer<DataManager> pDataManagerClone = QSharedPointer<DataManager>::create();
    return pDataManagerClone;
}

//=============================================================================================================

void DataManager::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void DataManager::unload()
{
}

//=============================================================================================================

QString DataManager::getName() const
{
    return "Data";
}

//=============================================================================================================

QMenu *DataManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *DataManager::getControl()
{
    DISPLIB::BidsView* pDataManagerBidsView = new DISPLIB::BidsView;

    pDataManagerBidsView->setModel(m_pAnalyzeData->getDataModel());

    connect(pDataManagerBidsView, &DISPLIB::BidsView::selectedModelChanged,
            this, &DataManager::onCurrentlySelectedModelChanged, Qt::UniqueConnection);

    connect(pDataManagerBidsView, &DISPLIB::BidsView::selectedItemChanged,
            this, &DataManager::onCurrentItemChanged, Qt::UniqueConnection);

    connect(pDataManagerBidsView, &DISPLIB::BidsView::removeItem,
            this, &DataManager::onRemoveItem, Qt::UniqueConnection);

    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setWidget(pDataManagerBidsView);
    pControlDock->setObjectName(getName());

    return pControlDock;
}

//=============================================================================================================

QWidget *DataManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void DataManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    default:
        qWarning() << "[DataManager::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> DataManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;

    return temp;
}

//=============================================================================================================

void DataManager::onCurrentlySelectedModelChanged(const QVariant& data)
{
    m_pCommu->publishEvent(EVENT_TYPE::SELECTED_MODEL_CHANGED, data);
}

//=============================================================================================================

void DataManager::onRemoveItem(const QModelIndex& index)
{
    m_pAnalyzeData->removeModel(index);
}

//=============================================================================================================

void DataManager::onCurrentItemChanged(const QModelIndex &pIndex)
{
    m_pAnalyzeData->newSelection(pIndex);
}

//=============================================================================================================

QString DataManager::getBuildInfo()
{
    return QString(DATAMANAGERPLUGIN::buildDateTime()) + QString(" - ")  + QString(DATAMANAGERPLUGIN::buildHash());
}
