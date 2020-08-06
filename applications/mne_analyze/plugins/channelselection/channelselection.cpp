//=============================================================================================================
/**
 * @file     channelselection.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu
 * @since    0.1.4
 * @date     July, 2020
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
 * @brief    Contains the definition of the ChannelSelection class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelselection.h"

#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/channelselectionview.h>

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Utils/metatypes.h>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CHANNELSELECTIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSelection::ChannelSelection()
: m_pChannelSelectionView(Q_NULLPTR)
, m_pChannelInfoModel(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_pViewLayout(Q_NULLPTR)
, m_pControlLayout(Q_NULLPTR)
, m_bIsInit(false)
{
//    m_pChannelSelectionView = QSharedPointer<DISPLIB::ChannelSelectionView>(new DISPLIB::ChannelSelectionView(QString("MNEANALYZE/CHANSELECT")));
}

//=============================================================================================================

ChannelSelection::~ChannelSelection()
{
}

//=============================================================================================================

QSharedPointer<IPlugin> ChannelSelection::clone() const
{
    QSharedPointer<ChannelSelection> pChannelSelectionClone = QSharedPointer<ChannelSelection>::create();
    return pChannelSelectionClone;
}

//=============================================================================================================

void ChannelSelection::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void ChannelSelection::unload()
{
}

//=============================================================================================================

QString ChannelSelection::getName() const
{
    return "Channel Selection";
}

//=============================================================================================================

QMenu *ChannelSelection::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *ChannelSelection::getControl()
{
    QDockWidget* pControlDockWidget = new QDockWidget(getName());
    QWidget* pControlWidget = new QWidget(pControlDockWidget);

    m_pControlLayout = new QHBoxLayout();
    pControlWidget->setLayout(m_pControlLayout);
    pControlDockWidget->setWidget(pControlWidget);

    //m_pControlLayout->addWidget(m_pChannelSelectionView->getControlWidget());

    pControlDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDockWidget->setObjectName("Channel Selection");
    pControlDockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));

//    QLabel* test = new QLabel("getControl");
//    m_pControlLayout->addWidget(test);

    return pControlDockWidget;

    //return Q_NULLPTR;
}

//=============================================================================================================

QWidget *ChannelSelection::getView()
{
    QWidget* pViewWidget = new QWidget();
    m_pViewLayout = new QHBoxLayout();
    pViewWidget->setLayout(m_pViewLayout);

    //m_pViewLayout->addWidget(m_pChannelSelectionView->getViewWidget());

    return pViewWidget;

    //return Q_NULLPTR;
}

//=============================================================================================================

void ChannelSelection::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
            break;
        default:
            qWarning() << "[ChannelSelection::handleEvent] received an Event that is not handled by switch-cases";
            break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ChannelSelection::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void ChannelSelection::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        setFiffSettings(qSharedPointerCast<FiffRawViewModel>(pNewModel)->getFiffInfo());
    }
}

//=============================================================================================================

void ChannelSelection::setFiffSettings(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    qDebug() << "ChannelSelection::setFiffSettings";

    m_pFiffInfo = pFiffInfo;

    if(m_bIsInit){
        return;
    }

    m_pChannelInfoModel = QSharedPointer<DISPLIB::ChannelInfoModel>(new DISPLIB::ChannelInfoModel(m_pFiffInfo));

    m_pChannelSelectionView = QSharedPointer<DISPLIB::ChannelSelectionView>(new DISPLIB::ChannelSelectionView(QString("MNEANALYZE/CHANSELECT"),
                                                                           Q_NULLPTR,
                                                                           m_pChannelInfoModel,
                                                                           Qt::Window));

    m_pViewLayout->addWidget(m_pChannelSelectionView->getViewWidget());
    m_pControlLayout->addWidget(m_pChannelSelectionView->getControlWidget());

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::loadedLayoutMap,
            m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::layoutChanged, Qt::UniqueConnection);

    connect(m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::setCurrentlyMappedFiffChannels, Qt::UniqueConnection);

    //Slots for event loop
    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::showSelectedChannelsOnly,
            this, &ChannelSelection::onShowSelectedChannelsOnly, Qt::UniqueConnection);

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::selectionChanged,
            this, &ChannelSelection::onSelectionChanged, Qt::UniqueConnection);

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::loadedLayoutMap,
            this, &ChannelSelection::onLoadedLayoutMap, Qt::UniqueConnection);

    connect(m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::channelsMappedToLayout,
            this, &ChannelSelection::onChannelsMappedToLayout, Qt::UniqueConnection);


//    QVariant data;
//    data.setValue(m_pChannelSelectionView.data());
//    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_INDICES, data);

    m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

    m_pChannelSelectionView->updateDataView();

//    QVariant data;
//    data.setValue(m_pChannelInfoModel);
//    m_pCommu->publishEvent(EVENT_TYPE::SET_CHANNEL_SELECTION, data);

    m_bIsInit = true;
}

//=============================================================================================================

void ChannelSelection::onShowSelectedChannelsOnly(const QStringList&  selectedChannels)
{
    QList<int> selectedChannelsIndexes;

    for(int i = 0; i<selectedChannels.size(); i++)
        selectedChannelsIndexes<<m_pChannelInfoModel->getIndexFromOrigChName(selectedChannels.at(i));

    QVariant data;
    data.setValue(selectedChannelsIndexes);
    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_INDICES, data);

}

//=============================================================================================================

#include <disp/viewers/helpers/selectionsceneitem.h>
void ChannelSelection::onSelectionChanged(const QList<QGraphicsItem*>& selectedChannelItems)
{
    DISPLIB::SelectionSceneItem* test = static_cast<DISPLIB::SelectionSceneItem*>(selectedChannelItems.first());
    QListIterator<QGraphicsItem*> i(selectedChannelItems);

    m_listItemList.clear();

    while (i.hasNext()) {
        DISPLIB::SelectionSceneItem* selectionSceneItemTemp = static_cast<DISPLIB::SelectionSceneItem*>(i.next());
        QSharedPointer<DISPLIB::SelItem> newItem;
        newItem.create();

        newItem->m_sChannelName = selectionSceneItemTemp->m_sChannelName;
        newItem->m_iChannelNumber = selectionSceneItemTemp->m_iChannelNumber;
        newItem->m_iChannelKind = selectionSceneItemTemp->m_iChannelKind;
        newItem->m_iChannelUnit = selectionSceneItemTemp->m_iChannelUnit;
        newItem->m_qpChannelPosition = selectionSceneItemTemp->m_qpChannelPosition;

        m_listItemList.append(newItem);
    }
    QVariant data;
    data.setValue(m_listItemList);
    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_ITEMS, data);
}

//=============================================================================================================

void ChannelSelection::onLoadedLayoutMap(const QMap<QString,QPointF> &layoutMap)
{
    QVariant data;
    data.setValue(layoutMap);
    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_MAP, data);
}

//=============================================================================================================

void ChannelSelection::onChannelsMappedToLayout(const QStringList &mappedLayoutChNames)
{
    std::cout<<"CHAN NAMES:" << mappedLayoutChNames.first().toStdString() << std::endl;
    QVariant data;
    data.setValue(mappedLayoutChNames);
    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_CHANNELS, data);
}
