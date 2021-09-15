//=============================================================================================================
/**
 * @file     averaging.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.3
 * @date     May, 2020
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
 * @brief    Averaging class defintion.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/averagingsettingsview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/scalingview.h>

#include <disp/viewers/helpers/selectionsceneitem.h>

#include <rtprocessing/helpers/filterkernel.h>
#include <rtprocessing/averaging.h>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>

#include <events/eventmanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QTabWidget>
#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;
using namespace ANSHAREDLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pFiffRawModel(Q_NULLPTR)
, m_pTriggerList(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_pCommu(Q_NULLPTR)
, m_pAveragingSettingsView(Q_NULLPTR)
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
    m_pEvokedModel = QSharedPointer<DISPLIB::EvokedSetModel>(new DISPLIB::EvokedSetModel());
}

//=============================================================================================================

Averaging::~Averaging()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Averaging::clone() const
{
    return QSharedPointer<AbstractPlugin> (new Averaging);
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

QMenu *Averaging::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *Averaging::getView()
{
    QWidget* pAveragingViewWidget = new QWidget();
    QTabWidget* pTabView = new QTabWidget(pAveragingViewWidget);
    QVBoxLayout* pAveragingViewLayout = new QVBoxLayout();

    m_pButterflyView = new DISPLIB::ButterflyView("", pTabView);
    m_pAverageLayoutView = new DISPLIB::AverageLayoutView("", pTabView);

    connect(this, &Averaging::showSelectedChannels,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::showSelectedChannels, Qt::UniqueConnection);

    connect(this, &Averaging::showAllChannels,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::showAllChannels, Qt::UniqueConnection);

    connect(this, &Averaging::channelSelectionManagerChanged,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::channelSelectionChanged, Qt::UniqueConnection);

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
    m_pAveragingSettingsView = new DISPLIB::AveragingSettingsView(QString("MNEANALYZE/%1").arg(this->getName()));

    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeNumAverages,
            this, &Averaging::onChangeNumAverages, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeBaselineFrom,
            this, &Averaging::onChangeBaselineFrom, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeBaselineTo,
            this, &Averaging::onChangeBaselineTo, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changePostStim,
            this, &Averaging::onChangePostStim, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changePreStim,
            this, &Averaging::onChangePreStim, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeBaselineActive,
            this, &Averaging::onChangeBaselineActive, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::resetAverage,
            this, &Averaging::onResetAverage, Qt::UniqueConnection);
//    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeStimChannel,
//            this, &Averaging::onChangeStimChannel);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::calculateAverage,
            this, &Averaging::onComputeButtonClicked, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeDropActive,
            this, &Averaging::onRejectionChecked, Qt::UniqueConnection);
    connect(&m_FutureWatcher, &QFutureWatcher<QMap<double,QList<int>>>::finished,
            this, &Averaging::createNewAverage, Qt::UniqueConnection);
    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::setAutoCompute,
            this, &Averaging::setAutoCompute, Qt::UniqueConnection);

    m_pAveragingSettingsView->setProcessingMode(DISPLIB::AbstractView::ProcessingMode::Offline);
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
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
            break;
        case EVENT_TYPE::FILTER_ACTIVE_CHANGED:
            m_bPerformFiltering = e->getData().toBool();
            break;
        case EVENT_TYPE::FILTER_DESIGN_CHANGED:
            m_filterKernel = e->getData().value<FilterKernel>();
            break;
        case EVENT_TYPE::EVENTS_UPDATED:
            if(m_bAutoRecompute){
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
            setViewSettings(e->getData().value<ANSHAREDLIB::ViewParameters>());
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
            break;
        default:
            qWarning() << "[Averaging::handleEvent] Received an Event that is not handled by switch cases.";
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

void Averaging::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pFiffRawModel) {
            if(m_pFiffRawModel == pNewModel) {
                qInfo() << "[Averaging::onModelChanged] New model is the same as old model";
                return;
            }
        }
        auto pModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        if(auto info = pModel->getFiffInfo()){
            m_pFiffRawModel = pModel;
            loadFullGui(info);
        }
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
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
    m_fPreStim =  -(static_cast<float>(mseconds)/1000);
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{
    QMutexLocker lock(&m_ParameterMutex);
    m_fPostStim = (static_cast<float>(mseconds)/1000);
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
    if(!m_pFiffRawModel || (m_pFiffRawModel->getEventModel()->rowCount() < 2)){
        //qWarning() << "No model loaded. Cannot calculate average.";
        return;
    }

    if (m_FutureWatcher.isRunning()){
       // qWarning() << "Averaging computation already taking place.";
        return;
        //m_FutureWatcher.waitForFinished();
    }

    triggerLoadingStart("Calculating average...");

    m_Future = QtConcurrent::run(this,
                                 &Averaging::averageCalculation,
                                 *this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data(),
                                 m_pFiffRawModel->getEventModel()->getEventMatrix(),
                                 m_filterKernel,
                                 *m_pFiffRawModel->getFiffInfo());
    m_FutureWatcher.setFuture(m_Future);
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffEvokedSet> Averaging::averageCalculation(FIFFLIB::FiffRawData FiffRaw,
                                                                     MatrixXi matEvents,
                                                                     RTPROCESSINGLIB::FilterKernel filterKernel,
                                                                     FIFFLIB::FiffInfo fiffInfo)
{
    QMap<QString,double> mapReject;

    int iType = 1; //hardwired for now, change later to type
    mapReject.insert("eog", 300e-06);

    if(matEvents.size() < 6){
        //qWarning() << "[Averaging::averageCalacualtion] Not enough data points to calculate average.";
        return Q_NULLPTR;
    }

    QSharedPointer<FIFFLIB::FiffEvoked> pFiffEvoked = QSharedPointer<FIFFLIB::FiffEvoked>(new FIFFLIB::FiffEvoked());

    if(m_bPerformFiltering) {
        QMutexLocker lock(&m_ParameterMutex);
        *pFiffEvoked = RTPROCESSINGLIB::computeFilteredAverage(FiffRaw,
                                                               matEvents,
                                                               m_fPreStim,
                                                               m_fPostStim,
                                                               iType,
                                                               m_bBaseline,
                                                               m_fBaselineFromS,
                                                               m_fBaselineToS,
                                                               mapReject,
                                                               filterKernel);
    } else {
        QMutexLocker lock(&m_ParameterMutex);
        *pFiffEvoked = RTPROCESSINGLIB::computeAverage(FiffRaw,
                                                       matEvents,
                                                       m_fPreStim,
                                                       m_fPostStim,
                                                       iType,
                                                       m_bBaseline,
                                                       m_fBaselineFromS,
                                                       m_fBaselineToS,
                                                       mapReject);
    }

    QSharedPointer<FIFFLIB::FiffEvokedSet> pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());

    pFiffEvokedSet->evoked.append(*(pFiffEvoked.data()));
    pFiffEvokedSet->info = fiffInfo;

    QMutexLocker lock(&m_ParameterMutex);

    if(m_bBaseline){
        pFiffEvokedSet->evoked[0].baseline.first = m_fBaselineFromS;
        pFiffEvokedSet->evoked[0].baseline.second = m_fBaselineToS;
    }

    return pFiffEvokedSet;
}

//=============================================================================================================

void Averaging::createNewAverage()
{
    QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet = m_Future.result();

    if(pEvokedSet){
        if(m_bSavingAverage){
            QSharedPointer<ANSHAREDLIB::AveragingDataModel> pNewAvgModel = QSharedPointer<ANSHAREDLIB::AveragingDataModel>(new ANSHAREDLIB::AveragingDataModel(pEvokedSet));

            m_pAnalyzeData->addModel<ANSHAREDLIB::AveragingDataModel>(pNewAvgModel,
                                                                      "Average - " + QDateTime::currentDateTime().toString());
        } else if(m_bAutoRecompute){
            m_pEvokedModel->setEvokedSet(pEvokedSet);
            updateEvokedSetModel();
        }
    } else {
        qInfo() << "[Averaging::createNewAverage] Unable to compute average.";
    }

    m_bSavingAverage = false;
    triggerLoadingEnd("Calculating average...");
}

//=============================================================================================================

void Averaging::loadFullGui(QSharedPointer<FIFFLIB::FiffInfo> pInfo)
{
    m_pFiffInfo = pInfo;
    m_pAverageLayoutView->setFiffInfo(m_pFiffInfo);

    if(m_bLoaded) {
        return;
    }

    //Init Models

    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    //Modality selection
    DISPLIB::ModalitySelectionView* pModalitySelectionView = new DISPLIB::ModalitySelectionView(m_pFiffInfo->chs,
                                                                              QString("MNEANALYZE/AVERAGING"));
    pModalitySelectionView->setObjectName("group_tab_View_Modalities");

    connect(pModalitySelectionView, &DISPLIB::ModalitySelectionView::modalitiesChanged,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setModalityMap, Qt::UniqueConnection);

    m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

    // Quick control average selection
    DISPLIB::AverageSelectionView* pAverageSelectionView = new DISPLIB::AverageSelectionView(QString("MNEANALYZE/AVERAGING"));
    pAverageSelectionView->setObjectName("group_tab_View_Selection");

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            pAverageSelectionView, &DISPLIB::AverageSelectionView::setAverageActivation, Qt::UniqueConnection);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            pAverageSelectionView, &DISPLIB::AverageSelectionView::setAverageColor, Qt::UniqueConnection);

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageColor, Qt::UniqueConnection);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageActivationMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageColorMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageColor, Qt::UniqueConnection);

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageColor, Qt::UniqueConnection);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageActivationMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageActivation, Qt::UniqueConnection);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageColorMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageColor, Qt::UniqueConnection);

    m_pEvokedModel->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pEvokedModel->setAverageColor(pAverageSelectionView->getAverageColor());

    m_pButterflyView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pButterflyView->setAverageColor(pAverageSelectionView->getAverageColor());

    m_pAverageLayoutView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pAverageLayoutView->setAverageColor(pAverageSelectionView->getAverageColor());

    //Data customization:
    DISPLIB::FiffRawViewSettings* pChannelDataSettingsView = new DISPLIB::FiffRawViewSettings(QString("MNESCAN/RTESW"));
    pChannelDataSettingsView->setWidgetList(QStringList() << "screenshot" << "backgroundColor");
    pChannelDataSettingsView->setObjectName("group_tab_View_General");

    connect(pChannelDataSettingsView, &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setBackgroundColor, Qt::UniqueConnection);

    connect(pChannelDataSettingsView, &DISPLIB::FiffRawViewSettings::backgroundColorChanged,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setBackgroundColor, Qt::UniqueConnection);

    connect(pChannelDataSettingsView, &DISPLIB::FiffRawViewSettings::makeScreenshot,
            this, &Averaging::onMakeScreenshot, Qt::UniqueConnection);

    m_pAverageLayoutView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
    m_pButterflyView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());

    m_pTabView->addTab(pModalitySelectionView, "Modality");

    //Update saved params
    m_fBaselineFromS = static_cast<float>(m_pAveragingSettingsView->getBaselineFromSeconds())/1000.f;
    m_fBaselineToS = static_cast<float>(m_pAveragingSettingsView->getBaselineToSeconds())/1000.f;

    m_fPreStim = -(static_cast<float>(m_pAveragingSettingsView->getPreStimMSeconds())/1000.f);
    m_fPostStim = static_cast<float>(m_pAveragingSettingsView->getPostStimMSeconds())/1000.f;

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

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName;

    if(imageType.contains("SVG")) {
        fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.svg").arg(sDate).arg(sTime);
    } else if(imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.png").arg(sDate).arg(sTime);
    }

    m_pButterflyView->takeScreenshot(fileName);
}

//=============================================================================================================

void Averaging::setChannelSelection(const QVariant &data)
{
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("layoutview")){
        emit channelSelectionManagerChanged(data);
    }
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("butterflyview")){
        if(data.value<DISPLIB::SelectionItem*>()->m_bShowAll){
            emit showAllChannels();
        } else {
            emit showSelectedChannels(data.value<DISPLIB::SelectionItem*>()->m_iChannelNumber);
        }
    }
}

//=============================================================================================================

void Averaging::setScalingMap(const QVariant &data)
{
    if(!m_bLoaded){
        return;
    }

    if(data.value<ANSHAREDLIB::ScalingParameters>().m_sViewsToApply.contains("layoutview")){
        m_pAverageLayoutView->setScaleMap(data.value<ANSHAREDLIB::ScalingParameters>().m_mScalingMap);
    }
    if(data.value<ANSHAREDLIB::ScalingParameters>().m_sViewsToApply.contains("butterflyview")){
        m_pButterflyView->setScaleMap(data.value<ANSHAREDLIB::ScalingParameters>().m_mScalingMap);
    }

}

//=============================================================================================================

void Averaging::setViewSettings(ANSHAREDLIB::ViewParameters viewParams)
{
    if(viewParams.m_sViewsToApply.contains("layoutview")){
        if (viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::all || viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::background){
            m_pAverageLayoutView->setBackgroundColor(viewParams.m_colorBackground);
            m_pAverageLayoutView->update();
        }
    }

    if(viewParams.m_sViewsToApply.contains("butterflyview")){
        if (viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::all || viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::background){
            m_pButterflyView->setBackgroundColor(viewParams.m_colorBackground);
            m_pButterflyView->update();
        }
        if (viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::all || viewParams.m_sSettingsToApply == ANSHAREDLIB::ViewParameters::signal){
            m_pButterflyView->setSingleAverageColor(viewParams.m_colorSignal);
            m_pButterflyView->update();
        }
    }
}

//=============================================================================================================

void Averaging::onNewAveragingModel(QSharedPointer<ANSHAREDLIB::AveragingDataModel> pAveragingModel)
{
    m_pEvokedModel->setEvokedSet(pAveragingModel->data(QModelIndex()).value<QSharedPointer<FIFFLIB::FiffEvokedSet>>());

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

void Averaging::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    //Butterfly view
    if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        if(m_pButterflyView->getEvokedSetModel()->getEvokedSet().data() == qSharedPointerCast<AveragingDataModel>(pRemovedModel)->getEvokedSet().data()) {
            m_pButterflyView->clearView();
        }
        if(m_pAverageLayoutView->getEvokedSetModel()->getEvokedSet().data() == qSharedPointerCast<AveragingDataModel>(pRemovedModel)->getEvokedSet().data()) {
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

QString Averaging::getBuildDateTime()
{
    return QString(BUILDINFO::dateTime());
}
