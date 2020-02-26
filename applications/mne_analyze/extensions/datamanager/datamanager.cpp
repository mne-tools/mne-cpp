//=============================================================================================================
/**
 * @file     datamanager.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     August, 2018
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
 * @brief    Definition of the DataManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamanager.h"
#include "FormFiles/datamanagerview.h"
#include <anShared/Management/analyzedata.h>
#include "anShared/Utils/metatypes.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DATAMANAGEREXTENSION;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataManager::DataManager()
: m_pControlDock(Q_NULLPTR)
, m_pDataManagerView(Q_NULLPTR)
{

}

//=============================================================================================================

DataManager::~DataManager()
{
    delete m_pDataManagerView;
}

//=============================================================================================================

QSharedPointer<IExtension> DataManager::clone() const
{
    QSharedPointer<DataManager> pDataViewerClone = QSharedPointer<DataManager>::create();
    return pDataViewerClone;
}

//=============================================================================================================

void DataManager::init()
{
    m_pDataManagerView = new DataManagerView;

    updateListWidget();

    connect(m_pAnalyzeData.data(), &AnalyzeData::newModelAvailable,
            this, &DataManager::updateListWidget);
    connect(m_pAnalyzeData.data(), &AnalyzeData::modelPathChanged,
            this, &DataManager::updateListWidget);
    connect(m_pAnalyzeData.data(), &AnalyzeData::modelRemoved,
            this, &DataManager::updateListWidget);

}

//=============================================================================================================

void DataManager::unload()
{
}

//=============================================================================================================

QString DataManager::getName() const
{
    return "DataManager";
}

//=============================================================================================================

QMenu *DataManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *DataManager::getControl()
{
    if(!m_pControlDock) {
        m_pControlDock = new QDockWidget(tr("Data Manager"));
        m_pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControlDock->setWidget(m_pDataManagerView);
    }

    return m_pControlDock;
}

//=============================================================================================================

QWidget *DataManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void DataManager::handleEvent(QSharedPointer<Event> e)
{
    qDebug() << "[DataManager::handleEvent] received an Event that is not handled by switch-cases";
}

//=============================================================================================================

QVector<EVENT_TYPE> DataManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(EXTENSION_INIT_FINISHED);
    return temp;
}

//=============================================================================================================

void DataManager::updateListWidget()
{
    m_pDataManagerView->clearList();

    QList<QSharedPointer<AbstractModel>> lModels = m_pAnalyzeData->getModels();

    //add all model names to the listView
    for(QSharedPointer<AbstractModel> pModel: lModels) {

        if(pModel->getType() == MODEL_TYPE::ANSHAREDLIB_QENTITYLIST_MODEL) {
            continue;
        }

        QListWidgetItem* tempListItem = new QListWidgetItem;
        tempListItem->setText(pModel->getModelName());
        tempListItem->setToolTip(pModel->getModelPath());
        m_pDataManagerView->addListItem(tempListItem);
    }
}

//=============================================================================================================
