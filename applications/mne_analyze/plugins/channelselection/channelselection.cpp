//=============================================================================================================
/**
 * @file     channelselection.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu
 * @since    0.1.5
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

#include <disp/viewers/helpers/selectionsceneitem.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/applytoview.h>

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
: m_pSelectionItem(new DISPLIB::SelectionItem())
, m_pChannelSelectionView(Q_NULLPTR)
, m_pChannelInfoModel(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_pViewLayout(Q_NULLPTR)
, m_pControlLayout(Q_NULLPTR)
, m_bIsInit(false)
{
}

//=============================================================================================================

ChannelSelection::~ChannelSelection()
{
    delete m_pSelectionItem;
    delete m_pChannelSelectionView;
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> ChannelSelection::clone() const
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

//=========================================virtual QString getBuildDateTime();====================================================================

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

    m_pControlLayout = new QVBoxLayout();
    pControlWidget->setLayout(m_pControlLayout);
    pControlDockWidget->setWidget(pControlWidget);

    pControlDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDockWidget->setObjectName("Channel Selection");
    pControlDockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));

    QLabel* pTempLabel = new QLabel("No File Loaded");
    m_pControlLayout->addWidget(pTempLabel);

    return pControlDockWidget;
}

//=============================================================================================================

QWidget *ChannelSelection::getView()
{
    QWidget* pViewWidget = new QWidget();
    m_pViewLayout = new QHBoxLayout();
    pViewWidget->setLayout(m_pViewLayout);

    return pViewWidget;
}

//=============================================================================================================

void ChannelSelection::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        if(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >()->getType() != ANSHAREDLIB_BEMDATA_MODEL) {
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        }
        break;
    case EVENT_TYPE::MODEL_REMOVED:
        onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
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
    temp.push_back(MODEL_REMOVED);

    return temp;
}

//=============================================================================================================

void ChannelSelection::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(auto info = qSharedPointerCast<FiffRawViewModel>(pNewModel)->getFiffInfo()){
            setFiffSettings(info);
        }
    }
}

//=============================================================================================================

void ChannelSelection::setFiffSettings(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;

    if(m_bIsInit){
        m_pChannelInfoModel->setFiffInfo(m_pFiffInfo);
        return;
    }

    //First time set up only
    m_pControlLayout->takeAt(0)->widget()->hide();

    m_pChannelInfoModel = QSharedPointer<DISPLIB::ChannelInfoModel>(new DISPLIB::ChannelInfoModel(m_pFiffInfo));

    m_pChannelSelectionView = new DISPLIB::ChannelSelectionView(QString("MNEANALYZE/CHANSELECT"),
                                                                Q_NULLPTR,
                                                                m_pChannelInfoModel,
                                                                Qt::Window);

    m_pViewLayout->addWidget(m_pChannelSelectionView->getViewWidget());
    m_pControlLayout->addWidget(m_pChannelSelectionView->getControlWidget());

    m_pApplyToView = new DISPLIB::ApplyToView("",m_pControlLayout->widget());

    m_pControlLayout->addWidget(m_pApplyToView);

    connect(m_pChannelSelectionView, &DISPLIB::ChannelSelectionView::loadedLayoutMap,
            m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::layoutChanged, Qt::UniqueConnection);

    connect(m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView, &DISPLIB::ChannelSelectionView::setCurrentlyMappedFiffChannels, Qt::UniqueConnection);

    //Slots for event loop

    connect(m_pChannelSelectionView, &DISPLIB::ChannelSelectionView::selectionChanged,
            this, &ChannelSelection::onSelectionChanged, Qt::UniqueConnection);

    m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

    m_pChannelSelectionView->updateDataView();

    m_bIsInit = true;
}

//=============================================================================================================

void ChannelSelection::onSelectionChanged(const QList<QGraphicsItem*>& selectedChannelItems)
{
    QListIterator<QGraphicsItem*> i(selectedChannelItems);

    m_pSelectionItem->m_sChannelName.clear();
    m_pSelectionItem->m_iChannelNumber.clear();
    m_pSelectionItem->m_iChannelKind.clear();
    m_pSelectionItem->m_iChannelUnit.clear();
    m_pSelectionItem->m_qpChannelPosition.clear();
    m_pSelectionItem->m_sViewsToApply.clear();

    while(i.hasNext()){
        DISPLIB::SelectionSceneItem* selectionSceneItemTemp = static_cast<DISPLIB::SelectionSceneItem*>(i.next());

        m_pSelectionItem->m_sChannelName.append(selectionSceneItemTemp->m_sChannelName);
        m_pSelectionItem->m_iChannelNumber.append(m_pChannelInfoModel->getIndexFromOrigChName(selectionSceneItemTemp->m_sChannelName.remove(' ')));
        m_pSelectionItem->m_iChannelKind.append(selectionSceneItemTemp->m_iChannelKind);
        m_pSelectionItem->m_iChannelUnit.append(selectionSceneItemTemp->m_iChannelUnit);
        m_pSelectionItem->m_qpChannelPosition.append(selectionSceneItemTemp->m_qpChannelPosition);
    }

    m_pSelectionItem->m_bShowAll = m_pChannelSelectionView->isSelectionEmpty();

    m_pSelectionItem->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_ITEMS, QVariant::fromValue(/*static_cast<void*>(*/m_pSelectionItem/*)*/));
}

//=============================================================================================================

void ChannelSelection::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if(m_pAnalyzeData->getModelsByType(ANSHAREDLIB_FIFFRAW_MODEL).size() == 0 && m_pAnalyzeData->getModelsByType(ANSHAREDLIB_AVERAGING_MODEL).size() == 0){
        m_pChannelSelectionView->clearView();
    }
}


//=============================================================================================================

QString ChannelSelection::getBuildDateTime()
{
    return QString(BUILDINFO::timestamp());
}
