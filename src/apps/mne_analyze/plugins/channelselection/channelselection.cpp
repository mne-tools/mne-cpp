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

#include <mne/base/log.h>
#include "channelselection.h"

#include <mne/disp/viewers/helpers/selectionsceneitem.h>
#include <mne/disp/viewers/helpers/channelinfomodel.h>
#include <mne/disp/viewers/channelselectionview.h>
#include <mne/disp/viewers/applytoview.h>
#include <anShared/Management/communicator.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Management/analyzedata.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CHANNELSELECTIONPLUGIN;
using namespace mne::analyze;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSelection::ChannelSelection()
: m_pSelectionItem(std::make_unique<mne::disp::SelectionItem>())
, m_pChannelInfoModel(nullptr)
, m_pFiffInfo(nullptr)
, m_pViewLayout(nullptr)
, m_pControlLayout(nullptr)
, m_bIsInit(false)
{
}

//=============================================================================================================

ChannelSelection::~ChannelSelection() = default;

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

QMenu* ChannelSelection::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QDockWidget* ChannelSelection::getControl()
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

QWidget* ChannelSelection::getView()
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
            if (e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>()->getType() != ANSHAREDLIB_BEMDATA_MODEL) {
                onModelChanged(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            }
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        default:
            mne::Log::warning("[ChannelSelection::handleEvent] received an Event that is not handled by switch-cases");
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

void ChannelSelection::onModelChanged(QSharedPointer<mne::analyze::AbstractModel> pNewModel)
{
    if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if (auto info = qSharedPointerCast<FiffRawViewModel>(pNewModel)->getFiffInfo()) {
            setFiffSettings(info);
        }
    }
}

//=============================================================================================================

void ChannelSelection::setFiffSettings(std::shared_ptr<mne::FiffInfo> pFiffInfo)
{
    m_pFiffInfo = std::move(pFiffInfo);

    if (m_bIsInit) {
        m_pChannelInfoModel->setFiffInfo(m_pFiffInfo);
        return;
    }

    //First time set up only
    m_pControlLayout->takeAt(0)->widget()->hide();

    m_pChannelInfoModel = std::make_shared<mne::disp::ChannelInfoModel>(m_pFiffInfo);

    m_pChannelSelectionView = std::make_unique<mne::disp::ChannelSelectionView>(QString("MNEANALYZE/CHANSELECT"),
                                                                                nullptr,
                                                                                m_pChannelInfoModel,
                                                                                Qt::Window);

    m_pViewLayout->addWidget(m_pChannelSelectionView->getViewWidget());
    m_pControlLayout->addWidget(m_pChannelSelectionView->getControlWidget());

    m_pApplyToView = new mne::disp::ApplyToView("", m_pControlLayout->widget());

    m_pControlLayout->addWidget(m_pApplyToView);

    connect(m_pChannelSelectionView.get(), &mne::disp::ChannelSelectionView::loadedLayoutMap,
            m_pChannelInfoModel.get(), &mne::disp::ChannelInfoModel::layoutChanged, Qt::UniqueConnection);

    connect(m_pChannelInfoModel.get(), &mne::disp::ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView.get(), &mne::disp::ChannelSelectionView::setCurrentlyMappedFiffChannels, Qt::UniqueConnection);

    //Slots for event loop

    connect(m_pChannelSelectionView.get(), &mne::disp::ChannelSelectionView::selectionChanged,
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

    while (i.hasNext()) {
        mne::disp::SelectionSceneItem* selectionSceneItemTemp = static_cast<mne::disp::SelectionSceneItem*>(i.next());

        m_pSelectionItem->m_sChannelName.append(selectionSceneItemTemp->m_sChannelName);
        m_pSelectionItem->m_iChannelNumber.append(m_pChannelInfoModel->getIndexFromOrigChName(selectionSceneItemTemp->m_sChannelName));
        m_pSelectionItem->m_iChannelKind.append(selectionSceneItemTemp->m_iChannelKind);
        m_pSelectionItem->m_iChannelUnit.append(selectionSceneItemTemp->m_iChannelUnit);
        m_pSelectionItem->m_qpChannelPosition.append(selectionSceneItemTemp->m_qpChannelPosition);
    }

    m_pSelectionItem->m_bShowAll = m_pChannelSelectionView->isSelectionEmpty();

    m_pSelectionItem->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pCommu->publishEvent(EVENT_TYPE::CHANNEL_SELECTION_ITEMS, QVariant::fromValue(m_pSelectionItem.get()));
}

//=============================================================================================================

void ChannelSelection::onModelRemoved([[maybe_unused]] QSharedPointer<mne::analyze::AbstractModel> pRemovedModel)
{
    // The view is only created with the first recording
    if (!m_bIsInit) {
        return;
    }
    if (m_pAnalyzeData->getModelsByType(ANSHAREDLIB_FIFFRAW_MODEL).size() == 0 && m_pAnalyzeData->getModelsByType(ANSHAREDLIB_AVERAGING_MODEL).size() == 0) {
        m_pChannelSelectionView->clearView();
    }
}

//=============================================================================================================

QString ChannelSelection::getBuildInfo()
{
    return QString(CHANNELSELECTIONPLUGIN::buildDateTime()) + QString(" - ") + QString(CHANNELSELECTIONPLUGIN::buildHash());
}
