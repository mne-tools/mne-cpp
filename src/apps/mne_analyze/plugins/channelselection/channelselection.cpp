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

#include <mne/viz/backends/qt/viewers/helpers/selectionitem.h>
#include <mne/viz/backends/qt/viewers/helpers/channelinfomodel.h>
#include <mne/viz/backends/qt/viewers/channelselectionview.h>
#include <mne/viz/backends/qt/viewers/applytoview.h>
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
: m_pSelectionItem(std::make_unique<mne::viz::qt::SelectionItem>())
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
    auto pControlDockWidget = std::make_unique<QDockWidget>(getName());
    QWidget* pControlWidget = new QWidget(pControlDockWidget.get());

    m_pControlLayout = new QVBoxLayout(pControlWidget);
    pControlDockWidget->setWidget(pControlWidget);

    pControlDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDockWidget->setObjectName("Channel Selection");
    pControlDockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                  QSizePolicy::Preferred));

    QLabel* pTempLabel = new QLabel("No File Loaded", pControlWidget);
    m_pControlLayout->addWidget(pTempLabel);

    return pControlDockWidget.release();
}

//=============================================================================================================

QWidget* ChannelSelection::getView()
{
    auto pViewWidget = std::make_unique<QWidget>();
    m_pViewLayout = new QHBoxLayout(pViewWidget.get());

    return pViewWidget.release();
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
    std::unique_ptr<QLayoutItem>(m_pControlLayout->takeAt(0))->widget()->hide();

    m_pChannelInfoModel = std::make_shared<mne::viz::qt::ChannelInfoModel>(m_pFiffInfo);

    m_pChannelSelectionView = std::make_unique<mne::viz::qt::ChannelSelectionView>(QString("MNEANALYZE/CHANSELECT"),
                                                                                   nullptr,
                                                                                   m_pChannelInfoModel,
                                                                                   Qt::Window);

    m_pViewLayout->addWidget(m_pChannelSelectionView->getViewWidget());
    m_pControlLayout->addWidget(m_pChannelSelectionView->getControlWidget());

    m_pApplyToView = new mne::viz::qt::ApplyToView("", m_pControlLayout->parentWidget());

    m_pControlLayout->addWidget(m_pApplyToView);

    connect(m_pChannelSelectionView.get(), &mne::viz::qt::ChannelSelectionView::loadedLayoutMap,
            m_pChannelInfoModel.get(), &mne::viz::qt::ChannelInfoModel::layoutChanged, Qt::UniqueConnection);

    connect(m_pChannelInfoModel.get(), &mne::viz::qt::ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView.get(), &mne::viz::qt::ChannelSelectionView::setCurrentlyMappedFiffChannels, Qt::UniqueConnection);

    //Slots for event loop

    connect(m_pChannelSelectionView.get(), &mne::viz::qt::ChannelSelectionView::selectionChanged,
            this, &ChannelSelection::onSelectionChanged, Qt::UniqueConnection);

    m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

    m_pChannelSelectionView->updateDataView();

    m_bIsInit = true;
}

//=============================================================================================================

void ChannelSelection::onSelectionChanged(const mne::viz::qt::SelectionItem& selection)
{
    *m_pSelectionItem = selection;
    m_pSelectionItem->m_sViewsToApply.clear();
    // The view numbers the sensors of its layout; the other plugins need the rows of the loaded data
    m_pSelectionItem->m_iChannelNumber.clear();
    for (const std::string& name : selection.m_sChannelName) {
        m_pSelectionItem->m_iChannelNumber.push_back(m_pChannelInfoModel->getIndexFromOrigChName(name));
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
