//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     channelselection.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     July, 2020
 * @brief    Contains the definition of the ChannelSelection class.
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

void ChannelSelection::onModelRemoved([[maybe_unused]] QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if(m_pAnalyzeData->getModelsByType(ANSHAREDLIB_FIFFRAW_MODEL).size() == 0 && m_pAnalyzeData->getModelsByType(ANSHAREDLIB_AVERAGING_MODEL).size() == 0){
        m_pChannelSelectionView->clearView();
    }
}

//=============================================================================================================

QString ChannelSelection::getBuildInfo()
{
    return QString(CHANNELSELECTIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(CHANNELSELECTIONPLUGIN::buildHash());
}
