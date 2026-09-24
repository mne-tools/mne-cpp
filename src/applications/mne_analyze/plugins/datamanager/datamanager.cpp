//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     datamanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2018
 * @brief    Definition of the DataManager class.
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
    // This plugin subscribes to no events, so anything arriving here is
    // unexpected. A switch with only a default label is what MSVC reports as
    // C4065, so warn directly instead.
    qWarning() << "[DataManager::handleEvent] received an Event that is not handled:"
               << e->getType();
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
