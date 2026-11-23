//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     averaging.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.3
 * @date     May, 2020
 * @brief    Averaging class defintion.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "averaging.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <mne/disp/viewers/helpers/channelinfomodel.h>
#include <mne/disp/viewers/helpers/evokedsetmodel.h>
#include <mne/disp/viewers/averagingsettingsview.h>
#include <mne/disp/viewers/modalityselectionview.h>
#include <mne/disp/viewers/channelselectionview.h>
#include <mne/disp/viewers/averageselectionview.h>
#include <mne/disp/viewers/fiffrawviewsettings.h>
#include <mne/disp/viewers/averagelayoutview.h>
#include <mne/disp/viewers/butterflyview.h>
#include <mne/disp/viewers/scalingview.h>
#include <mne/disp/viewers/helpers/selectionsceneitem.h>
#include <mne/dsp/filterkernel.h>
#include <mne/dsp/rt/rt_filter.h>
#include <mne/mne/mne_epoch_data_list.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_info.h>
#include <events/eventmanager.h>
#include <mne/base/thread_pool.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QTabWidget>
#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;
using namespace mne::analyze;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pFiffRawModel(nullptr)
, m_pTriggerList(nullptr)
, m_pFiffInfo(nullptr)
, m_pCommu(nullptr)
, m_pAveragingSettingsView(nullptr)
, m_fBaselineFromS(0)
, m_fBaselineToS(0)
, m_fPreStim(0)
, m_fPostStim(0)
, m_fTriggerThreshold(0.5)
, m_bBaseline(0)
, m_bRejection(0)
, m_bLoaded(0)
, m_bPerformFiltering(false)
, m_bAutoRecompute(false)
, m_bSavingAverage(false)
{
    m_pEvokedModel = std::make_shared<mne::disp::EvokedSetModel>();
}

//=============================================================================================================

Averaging::~Averaging()
{
    if (m_Future.valid()) {
        m_Future.wait();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Averaging::clone() const
{
    return QSharedPointer<AbstractPlugin>(new Averaging);
}

//=============================================================================================================

void Averaging::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void Averaging::unload()
{
}

//=============================================================================================================

QString Averaging::getName() const
{
    return "Averaging";
}

//=============================================================================================================

QMenu* Averaging::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QWidget* Averaging::getView()
{
    QWidget* pAveragingViewWidget = new QWidget();
    QTabWidget* pTabView = new QTabWidget(pAveragingViewWidget);
    QVBoxLayout* pAveragingViewLayout = new QVBoxLayout();

    m_pButterflyView = new mne::disp::ButterflyView("", pTabView);
    m_pAverageLayoutView = new mne::disp::AverageLayoutView("", pTabView);

    connect(this, &Averaging::showSelectedChannels,
            m_pButterflyView.data(), &mne::disp::ButterflyView::showSelectedChannels, Qt::UniqueConnection);

    connect(this, &Averaging::showAllChannels,
            m_pButterflyView.data(), &mne::disp::ButterflyView::showAllChannels, Qt::UniqueConnection);

    connect(this, &Averaging::channelSelectionManagerChanged,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::channelSelectionChanged, Qt::UniqueConnection);

    m_pButterflyView->setObjectName("butterflyview");
    m_pAverageLayoutView->setObjectName("layoutview");
    pAveragingViewWidget->setObjectName("AvgView");

    pTabView->addTab(m_pButterflyView, "Butterfly View");
    pTabView->addTab(m_pAverageLayoutView, "2D Layout View");

    pAveragingViewLayout->addWidget(pTabView);
    pAveragingViewWidget->setLayout(pAveragingViewLayout);

    pAveragingViewWidget->setMinimumSize(256, 256);
    pAveragingViewWidget->setFocusPolicy(Qt::TabFocus);
    pAveragingViewWidget->setAttribute(Qt::WA_DeleteOnClose, false);

    return pAveragingViewWidget;
}

//=============================================================================================================

QDockWidget* Averaging::getControl()
{
    QDockWidget* pControl = new QDockWidget(getName());
    QScrollArea* pWidget = new QScrollArea();

    m_pLayout = new QVBoxLayout;
    m_pTabView = new QTabWidget();

    //Average Settings View
    m_pAveragingSettingsView = new mne::disp::AveragingSettingsView(QString("MNEANALYZE/%1").arg(this->getName()));

    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeNumAverages,
            this, &Averaging::onChangeNumAverages, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeBaselineFrom,
            this, &Averaging::onChangeBaselineFrom, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeBaselineTo,
            this, &Averaging::onChangeBaselineTo, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changePostStim,
            this, &Averaging::onChangePostStim, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changePreStim,
            this, &Averaging::onChangePreStim, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeBaselineActive,
            this, &Averaging::onChangeBaselineActive, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::resetAverage,
            this, &Averaging::onResetAverage, Qt::UniqueConnection);
    //    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeStimChannel,
    //            this, &Averaging::onChangeStimChannel);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::calculateAverage,
            this, &Averaging::onComputeButtonClicked, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::changeDropActive,
            this, &Averaging::onRejectionChecked, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &mne::disp::AveragingSettingsView::setAutoCompute,
            this, &Averaging::setAutoCompute, Qt::UniqueConnection);

    m_pAveragingSettingsView->setProcessingMode(mne::disp::AbstractView::ProcessingMode::Offline);
    m_pAveragingSettingsView->setSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Fixed);

    pWidget->setSizePolicy(QSizePolicy::Expanding,
                           QSizePolicy::Preferred);
    m_pTabView->setSizePolicy(QSizePolicy::Expanding,
                              QSizePolicy::Preferred);

    m_pTabView->addTab(m_pAveragingSettingsView, "Parameters");

    m_pLayout->addWidget(m_pTabView);
    m_pLayout->addStretch();
    pWidget->setLayout(m_pLayout);

    pControl->setWidget(pWidget);
    pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControl->setObjectName("Averaging");
    pControl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));
    return pControl;
}

//=============================================================================================================

void Averaging::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        case EVENT_TYPE::FILTER_ACTIVE_CHANGED:
            m_bPerformFiltering = e->getData().toBool();
            break;
        case EVENT_TYPE::FILTER_DESIGN_CHANGED:
            m_filterKernel = e->getData().value<FilterKernel>();
            break;
        case EVENT_TYPE::EVENTS_UPDATED:
            if (m_bAutoRecompute) {
                computeAverage();
            }
            break;
        case EVENT_TYPE::CHANNEL_SELECTION_ITEMS:
            setChannelSelection(e->getData());
            break;
        case EVENT_TYPE::SCALING_MAP_CHANGED:
            setScalingMap(e->getData());
            break;
        case EVENT_TYPE::VIEW_SETTINGS_CHANGED:
            setViewSettings(e->getData().value<mne::analyze::ViewParameters>());
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        default:
            Log::warning("[Averaging::handleEvent] Received an Event that is not handled by switch cases.");
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> Averaging::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(FILTER_ACTIVE_CHANGED);
    temp.push_back(FILTER_DESIGN_CHANGED);
    temp.push_back(EVENTS_UPDATED);
    temp.push_back(CHANNEL_SELECTION_ITEMS);
    temp.push_back(SCALING_MAP_CHANGED);
    temp.push_back(VIEW_SETTINGS_CHANGED);
    temp.push_back(MODEL_REMOVED);

    return temp;
}

//=============================================================================================================

void Averaging::onModelChanged(QSharedPointer<mne::analyze::AbstractModel> pNewModel)
{
    if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if (m_pFiffRawModel) {
            if (m_pFiffRawModel == pNewModel) {
                Log::info("[Averaging::onModelChanged] New model is the same as old model");
                return;
            }
        }
        auto pModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        if (auto info = pModel->getFiffInfo()) {
            m_pFiffRawModel = pModel;
            loadFullGui(info);
        }
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        loadFullGui(qSharedPointerCast<AveragingDataModel>(pNewModel)->getFiffInfo());
        onNewAveragingModel(qSharedPointerCast<AveragingDataModel>(pNewModel));
    }
}

//=============================================================================================================

void Averaging::onChangeNumAverages(qint32 numAve)
{
    Q_UNUSED(numAve)
}

//=============================================================================================================

void Averaging::onChangeBaselineFrom(qint32 fromMS)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_fBaselineFromS = static_cast<float>(fromMS) / 1000.f;
}

//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMS)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_fBaselineToS = static_cast<float>(toMS) / 1000.f;
}

//=============================================================================================================

void Averaging::onChangePreStim(qint32 mseconds)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_fPreStim = -(static_cast<float>(mseconds) / 1000);
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_fPostStim = (static_cast<float>(mseconds) / 1000);
}

//=============================================================================================================

void Averaging::onChangeBaselineActive(bool state)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_bBaseline = state;
}

//=============================================================================================================

void Averaging::onResetAverage(bool state)
{
    Q_UNUSED(state)
}

//=============================================================================================================

void Averaging::onComputeButtonClicked(bool bChecked)
{
    Q_UNUSED(bChecked);
    m_bSavingAverage = true;
    computeAverage();
}

//=============================================================================================================

void Averaging::computeAverage()
{
    if (!m_pFiffRawModel || (m_pFiffRawModel->getEventModel()->rowCount() < 2)) {
        //qWarning() << "No model loaded. Cannot calculate average.";
        return;
    }

    if (m_Future.valid()) {
        // qWarning() << "Averaging computation already taking place.";
        return;
        //m_Future.wait();
    }

    triggerLoadingStart("Calculating average...");

    m_Future = mne::ThreadPool::global().submit([this, pThis = QPointer<Averaging>(this)] {
        std::shared_ptr<mne::FiffEvokedSet> pEvokedSet = this->averageCalculation(*this->m_pFiffRawModel->getFiffIO()->raw.front().get(),
                                                                                  m_pFiffRawModel->getEventModel()->getEventMatrix(),
                                                                                  m_filterKernel,
                                                                                  *m_pFiffRawModel->getFiffInfo());
        if (pThis) {
            QMetaObject::invokeMethod(pThis, &Averaging::createNewAverage, Qt::QueuedConnection);
        }
        return pEvokedSet;
    });
}

//=============================================================================================================

std::shared_ptr<mne::FiffEvokedSet> Averaging::averageCalculation(const mne::FiffRawData& FiffRaw,
                                                                  MatrixXi matEvents,
                                                                  mne::FilterKernel filterKernel,
                                                                  mne::FiffInfo fiffInfo)
{
    std::map<std::string, double> mapReject;
    mapReject.emplace("eog", 300e-06);

    if (matEvents.rows() < 2) {
        return nullptr;
    }

    // One evoked response per trigger code of the selected events, like mne.Epochs per event_id
    QList<int> eventCodes;
    for (Eigen::Index i = 0; i < matEvents.rows(); ++i) {
        if (matEvents(i, 2) != 0 && !eventCodes.contains(matEvents(i, 2)))
            eventCodes.append(matEvents(i, 2));
    }
    std::sort(eventCodes.begin(), eventCodes.end());

    std::shared_ptr<mne::FiffEvokedSet> pFiffEvokedSet = std::make_shared<mne::FiffEvokedSet>();
    pFiffEvokedSet->info = fiffInfo;

    QMutexLocker lock(&m_ParameterMutex);
    for (const int eventCode : eventCodes) {
        mne::FiffEvoked evoked = m_bPerformFiltering
            ? mne::computeFilteredAverage(FiffRaw, matEvents, m_fPreStim, m_fPostStim, eventCode, m_bBaseline,
                                          m_fBaselineFromS, m_fBaselineToS, mapReject, filterKernel)
            : MNEEpochDataList::computeAverage(FiffRaw, matEvents, m_fPreStim, m_fPostStim, eventCode, m_bBaseline,
                                               m_fBaselineFromS, m_fBaselineToS, mapReject);
        if (evoked.empty())
            continue;
        evoked.comment = QString::number(eventCode).toStdString();
        if (m_bBaseline) {
            evoked.baseline.first = m_fBaselineFromS;
            evoked.baseline.second = m_fBaselineToS;
        }
        pFiffEvokedSet->evoked.push_back(evoked);
    }

    if (pFiffEvokedSet->evoked.empty())
        return nullptr;
    return pFiffEvokedSet;
}

//=============================================================================================================

void Averaging::createNewAverage()
{
    std::shared_ptr<mne::FiffEvokedSet> pEvokedSet = m_Future.get();

    if (pEvokedSet) {
        if (m_bSavingAverage) {
            QSharedPointer<mne::analyze::AveragingDataModel> pNewAvgModel = QSharedPointer<mne::analyze::AveragingDataModel>(new mne::analyze::AveragingDataModel(pEvokedSet));

            m_pAnalyzeData->addModel<mne::analyze::AveragingDataModel>(pNewAvgModel,
                                                                       "Average - " + QDateTime::currentDateTime().toString());
        } else if (m_bAutoRecompute) {
            m_pEvokedModel->setEvokedSet(pEvokedSet);
            updateEvokedSetModel();
        }
    } else {
        Log::info("[Averaging::createNewAverage] Unable to compute average.");
    }

    m_bSavingAverage = false;
    triggerLoadingEnd("Calculating average...");
}

//=============================================================================================================

void Averaging::loadFullGui(std::shared_ptr<mne::FiffInfo> pInfo)
{
    m_pFiffInfo = pInfo;
    m_pAverageLayoutView->setFiffInfo(m_pFiffInfo);

    if (m_bLoaded) {
        return;
    }

    //Init Models

    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    //Modality selection
    mne::disp::ModalitySelectionView* pModalitySelectionView = new mne::disp::ModalitySelectionView(m_pFiffInfo->chs,
                                                                                                    QString("MNEANALYZE/AVERAGING"));
    pModalitySelectionView->setObjectName("group_tab_View_Modalities");

    connect(pModalitySelectionView, &mne::disp::ModalitySelectionView::modalitiesChanged,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setModalityMap, Qt::UniqueConnection);

    m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

    // Quick control average selection
    mne::disp::AverageSelectionView* pAverageSelectionView = new mne::disp::AverageSelectionView(QString("MNEANALYZE/AVERAGING"));
    pAverageSelectionView->setObjectName("group_tab_View_Selection");

    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageActivationMap,
            pAverageSelectionView, &mne::disp::AverageSelectionView::setAverageActivation, Qt::UniqueConnection);
    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageColorMap,
            pAverageSelectionView, &mne::disp::AverageSelectionView::setAverageColor, Qt::UniqueConnection);

    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageColorMap,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setAverageColor, Qt::UniqueConnection);
    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageActivationMap,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &mne::disp::AverageSelectionView::newAverageActivationMap,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &mne::disp::AverageSelectionView::newAverageColorMap,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setAverageColor, Qt::UniqueConnection);

    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageColorMap,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::setAverageColor, Qt::UniqueConnection);
    connect(m_pEvokedModel.get(), &mne::disp::EvokedSetModel::newAverageActivationMap,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &mne::disp::AverageSelectionView::newAverageActivationMap,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &mne::disp::AverageSelectionView::newAverageColorMap,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::setAverageColor, Qt::UniqueConnection);

    m_pEvokedModel->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pEvokedModel->setAverageColor(pAverageSelectionView->getAverageColor());

    m_pButterflyView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pButterflyView->setAverageColor(pAverageSelectionView->getAverageColor());

    m_pAverageLayoutView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pAverageLayoutView->setAverageColor(pAverageSelectionView->getAverageColor());

    //Data customization:
    mne::disp::FiffRawViewSettings* pChannelDataSettingsView = new mne::disp::FiffRawViewSettings(QString("MNESCAN/RTESW"));
    pChannelDataSettingsView->setWidgetList({"screenshot", "backgroundColor"});
    pChannelDataSettingsView->setObjectName("group_tab_View_General");

    connect(pChannelDataSettingsView, &mne::disp::FiffRawViewSettings::backgroundColorChanged,
            m_pAverageLayoutView.data(), &mne::disp::AverageLayoutView::setBackgroundColor, Qt::UniqueConnection);

    connect(pChannelDataSettingsView, &mne::disp::FiffRawViewSettings::backgroundColorChanged,
            m_pButterflyView.data(), &mne::disp::ButterflyView::setBackgroundColor, Qt::UniqueConnection);

    connect(pChannelDataSettingsView, &mne::disp::FiffRawViewSettings::makeScreenshot,
            this, &Averaging::onMakeScreenshot, Qt::UniqueConnection);

    m_pAverageLayoutView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
    m_pButterflyView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());

    m_pTabView->addTab(pModalitySelectionView, "Modality");

    //Update saved params
    m_fBaselineFromS = static_cast<float>(m_pAveragingSettingsView->getBaselineFromSeconds()) / 1000.f;
    m_fBaselineToS = static_cast<float>(m_pAveragingSettingsView->getBaselineToSeconds()) / 1000.f;

    m_fPreStim = -(static_cast<float>(m_pAveragingSettingsView->getPreStimMSeconds()) / 1000.f);
    m_fPostStim = static_cast<float>(m_pAveragingSettingsView->getPostStimMSeconds()) / 1000.f;

    m_bAutoRecompute = m_pAveragingSettingsView->getAutoComputeStatus();

    m_bLoaded = true;
}

//=============================================================================================================

void Averaging::onRejectionChecked(bool bState)
{
    m_bRejection = bState;
}

//=============================================================================================================

void Averaging::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if (!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName;

    if (imageType.contains("SVG")) {
        fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.svg").arg(sDate).arg(sTime);
    } else if (imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.png").arg(sDate).arg(sTime);
    }

    m_pButterflyView->takeScreenshot(fileName);
}

//=============================================================================================================

void Averaging::setChannelSelection(const QVariant& data)
{
    if (std::ranges::contains(data.value<mne::disp::SelectionItem*>()->m_sViewsToApply, "layoutview")) {
        emit channelSelectionManagerChanged(data);
    }
    if (std::ranges::contains(data.value<mne::disp::SelectionItem*>()->m_sViewsToApply, "butterflyview")) {
        if (data.value<mne::disp::SelectionItem*>()->m_bShowAll) {
            emit showAllChannels();
        } else {
            emit showSelectedChannels(data.value<mne::disp::SelectionItem*>()->m_iChannelNumber);
        }
    }
}

//=============================================================================================================

void Averaging::setScalingMap(const QVariant& data)
{
    if (!m_bLoaded) {
        return;
    }

    if (std::ranges::contains(data.value<mne::analyze::ScalingParameters>().m_sViewsToApply, "layoutview")) {
        m_pAverageLayoutView->setScaleMap(data.value<mne::analyze::ScalingParameters>().m_mScalingMap);
    }
    if (std::ranges::contains(data.value<mne::analyze::ScalingParameters>().m_sViewsToApply, "butterflyview")) {
        m_pButterflyView->setScaleMap(data.value<mne::analyze::ScalingParameters>().m_mScalingMap);
    }
}

//=============================================================================================================

void Averaging::setViewSettings(mne::analyze::ViewParameters viewParams)
{
    if (std::ranges::contains(viewParams.m_sViewsToApply, "layoutview")) {
        if (viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::all || viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::background) {
            m_pAverageLayoutView->setBackgroundColor(viewParams.m_colorBackground);
            m_pAverageLayoutView->update();
        }
    }

    if (std::ranges::contains(viewParams.m_sViewsToApply, "butterflyview")) {
        if (viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::all || viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::background) {
            m_pButterflyView->setBackgroundColor(viewParams.m_colorBackground);
            m_pButterflyView->update();
        }
        if (viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::all || viewParams.m_sSettingsToApply == mne::analyze::ViewParameters::signal) {
            m_pButterflyView->setSingleAverageColor(viewParams.m_colorSignal);
            m_pButterflyView->update();
        }
    }
}

//=============================================================================================================

void Averaging::onNewAveragingModel(QSharedPointer<mne::analyze::AveragingDataModel> pAveragingModel)
{
    m_pEvokedModel->setEvokedSet(pAveragingModel->data(QModelIndex()).value<std::shared_ptr<mne::FiffEvokedSet>>());

    updateEvokedSetModel();
}

//=============================================================================================================

void Averaging::triggerLoadingStart(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_START, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void Averaging::triggerLoadingEnd(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_END, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void Averaging::onModelRemoved(QSharedPointer<mne::analyze::AbstractModel> pRemovedModel)
{
    //Butterfly view
    if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        if (m_pButterflyView->getEvokedSetModel()->getEvokedSet().get() == qSharedPointerCast<AveragingDataModel>(pRemovedModel)->getEvokedSet().get()) {
            m_pButterflyView->clearView();
        }
        if (m_pAverageLayoutView->getEvokedSetModel()->getEvokedSet().get() == qSharedPointerCast<AveragingDataModel>(pRemovedModel)->getEvokedSet().get()) {
            m_pAverageLayoutView->clearView();
        }
    }
}

//=============================================================================================================

void Averaging::setAutoCompute(bool bShouldAutoCompute)
{
    m_bAutoRecompute = bShouldAutoCompute;
}

//=============================================================================================================

void Averaging::updateEvokedSetModel()
{
    m_pButterflyView->setEvokedSetModel(m_pEvokedModel);
    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    m_pButterflyView->showAllChannels();
    m_pButterflyView->dataUpdate();
    m_pButterflyView->updateView();
    m_pAverageLayoutView->updateData();
}

//=============================================================================================================

QString Averaging::getBuildInfo()
{
    return QString(AVERAGINGPLUGIN::buildDateTime()) + QString(" - ") + QString(AVERAGINGPLUGIN::buildHash());
}
