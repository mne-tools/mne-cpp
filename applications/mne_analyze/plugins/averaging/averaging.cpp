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
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/annotationmodel.h>

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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QTabWidget>

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
, m_bBasline(0)
, m_bRejection(0)
, m_bLoaded(0)
, m_bPerformFiltering(false)
, m_iCurrentGroup(9999)
{
}

//=============================================================================================================

Averaging::~Averaging()
{
}

//=============================================================================================================

QSharedPointer<IPlugin> Averaging::clone() const
{
    QSharedPointer<Averaging> pAveragingClone(new Averaging);
    return pAveragingClone;
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
    QWidget* pWidget = new QWidget();

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

    m_pAveragingSettingsView->setProcessingMode(DISPLIB::AbstractView::ProcessingMode::Offline);
    m_pAveragingSettingsView->setSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Minimum);

    QGroupBox* pGBox = new QGroupBox();
    QVBoxLayout* pVBLayout = new QVBoxLayout();

    pGBox->setSizePolicy(QSizePolicy::Expanding,
                         QSizePolicy::Minimum);
    pWidget->setSizePolicy(QSizePolicy::Expanding,
                           QSizePolicy::Minimum);

    pGBox->setLayout(pVBLayout);

    m_pLayout->addWidget(m_pAveragingSettingsView);

    pWidget->setLayout(m_pLayout);

    m_pTabView->addTab(pWidget, "Parameters");

    pControl->setWidget(m_pTabView);
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
        case FILTER_ACTIVE_CHANGED:
            m_bPerformFiltering = e->getData().toBool();
            break;
        case FILTER_DESIGN_CHANGED:
            m_filterKernel = e->getData().value<FilterKernel>();
            break;
        case EVENT_GROUPS_UPDATED:
            updateGroups();
            break;
        case CHANNEL_SELECTION_ITEMS:
            setChannelSelection(e->getData());
            break;
        case SCALING_MAP_CHANGED:
            setScalingMap(e->getData());
            break;
        case VIEW_SETTINGS_CHANGED:
            setViewSettings(e->getData().value<ANSHAREDLIB::ViewParameters>());
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
    temp.push_back(EVENT_GROUPS_UPDATED);
    temp.push_back(CHANNEL_SELECTION_ITEMS);
    temp.push_back(SCALING_MAP_CHANGED);
    temp.push_back(VIEW_SETTINGS_CHANGED);

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
        m_pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        loadFullGui();
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
    m_fBaselineFromS = static_cast<float>(fromMS) / 1000.f;
}

//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMS)
{
    m_fBaselineToS = static_cast<float>(toMS) / 1000.f;
}

//=============================================================================================================

void Averaging::onChangePreStim(qint32 mseconds)
{
    m_fPreStim =  -(static_cast<float>(mseconds)/1000);
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{
    m_fPostStim = (static_cast<float>(mseconds)/1000);
}

//=============================================================================================================

void Averaging::onChangeBaselineActive(bool state)
{
    m_bBasline = state;
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
    computeAverage();
}

//=============================================================================================================

void Averaging::computeAverage()
{
    if(!m_pFiffRawModel){
        qWarning() << "No model loaded. Cannot calculate average";
        return;
    }

    clearAveraging();

    MatrixXi matEvents;
    QMap<QString,double> mapReject;

    m_pFiffEvoked = QSharedPointer<FIFFLIB::FiffEvoked>(new FIFFLIB::FiffEvoked());
    int iType = 1; //hardwired for now, change later to type
    mapReject.insert("eog", 300e-06);

    FIFFLIB::FiffRawData* pFiffRaw = this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data();

    matEvents = m_pFiffRawModel->getAnnotationModel()->getAnnotationMatrix(m_iCurrentGroup);

    if(matEvents.size() < 6){
        qWarning() << "Not enough data points to calculate average.";
        return;
    }

    if(m_bPerformFiltering) {
        *m_pFiffEvoked = RTPROCESSINGLIB::computeFilteredAverage(*pFiffRaw,
                                                                 matEvents,
                                                                 m_fPreStim,
                                                                 m_fPostStim,
                                                                 iType,
                                                                 m_bBasline,
                                                                 m_fBaselineFromS,
                                                                 m_fBaselineToS,
                                                                 mapReject,
                                                                 m_filterKernel);
    } else {
        *m_pFiffEvoked = RTPROCESSINGLIB::computeAverage(*pFiffRaw,
                                                         matEvents,
                                                         m_fPreStim,
                                                         m_fPostStim,
                                                         iType,
                                                         m_bBasline,
                                                         m_fBaselineFromS,
                                                         m_fBaselineToS,
                                                         mapReject);
    }

    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());
    m_pFiffEvokedSet->evoked.append(*(m_pFiffEvoked.data()));
    m_pFiffEvokedSet->info = *(m_pFiffRawModel->getFiffInfo().data());

    if(m_bBasline){
        m_pFiffEvokedSet->evoked[0].baseline.first = m_fBaselineFromS;
        m_pFiffEvokedSet->evoked[0].baseline.second = m_fBaselineToS;
    }

    m_pEvokedModel->setEvokedSet(m_pFiffEvokedSet);

    m_pButterflyView->setEvokedSetModel(m_pEvokedModel);
    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    m_pButterflyView->dataUpdate();
    m_pButterflyView->updateView();
    m_pAverageLayoutView->updateData();

    qInfo() << "[Averaging::computeAverage] Average computed.";
}

//=============================================================================================================

void Averaging::loadFullGui()
{
    m_pFiffInfo = m_pFiffRawModel->getFiffInfo();
    m_pAverageLayoutView->setFiffInfo(m_pFiffInfo);

    if(m_bLoaded) {
        return;
    }

    //Init Models
    m_pEvokedModel = QSharedPointer<DISPLIB::EvokedSetModel>(new DISPLIB::EvokedSetModel());
    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());

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

    connect(m_pAveragingSettingsView, &DISPLIB::AveragingSettingsView::changeGroupSelect,
            this, &Averaging::onChangeGroupSelect, Qt::UniqueConnection);

    onChangeGroupSelect(m_pAveragingSettingsView->getCurrentSelectGroup());
    m_pAverageLayoutView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
    m_pButterflyView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());

    m_pTabView->addTab(pModalitySelectionView, "Modality");

    //Update saved params
    m_fBaselineFromS = static_cast<float>(m_pAveragingSettingsView->getBaselineFromSeconds())/1000.f;
    m_fBaselineToS = static_cast<float>(m_pAveragingSettingsView->getBaselineToSeconds())/1000.f;

    m_fPreStim = -(static_cast<float>(m_pAveragingSettingsView->getPreStimMSeconds())/1000.f);
    m_fPostStim = static_cast<float>(m_pAveragingSettingsView->getPostStimMSeconds())/1000.f;

    m_bLoaded = true;
}

//=============================================================================================================

void Averaging::clearAveraging()
{
    m_pFiffEvokedSet->evoked.clear();
    m_pFiffEvokedSet.clear();
    m_pFiffEvoked.clear();
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

void Averaging::updateGroups()
{
    m_pAveragingSettingsView->clearSelectionGroup();
    for(int i = 0; i < m_pFiffRawModel->getAnnotationModel()->getHubSize(); i++){
        m_pAveragingSettingsView->addSelectionGroup(m_pFiffRawModel->getAnnotationModel()->getGroupNameFromList(i));
    }
}

//=============================================================================================================

void Averaging::onChangeGroupSelect(const QString &text)
{
    m_iCurrentGroup = m_pFiffRawModel->getAnnotationModel()->getIndexFromName(text);
}

//=============================================================================================================

void Averaging::setChannelSelection(const QVariant &data)
{
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("layoutview")){
        emit channelSelectionManagerChanged(data);
    }
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("butterflyview")){
        emit showSelectedChannels(data.value<DISPLIB::SelectionItem*>()->m_iChannelNumber);
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
