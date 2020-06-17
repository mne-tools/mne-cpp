//=============================================================================================================
/**
 * @file     averaging.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.2
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
 * @brief    Definition of the Averaging class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"

#include <iostream>

#include <anShared/Management/communicator.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/annotationmodel.h>

#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/averagingsettingsview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/scalingview.h>


#include <mne/mne_epoch_data_list.h>
#include <mne/mne_epoch_data.h>

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

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pCommu(Q_NULLPTR)
, m_pFiffRawModel(Q_NULLPTR)
, m_pTriggerList(Q_NULLPTR)
, m_pAveragingSettingsView(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_fBaselineFrom(0)
, m_fBaselineTo(0)
, m_fPreStim(0)
, m_fPostStim(0)
, m_fTriggerThreshold(0.5)
, m_bUseAnn(1)
, m_bBasline(0)
, m_bRejection(0)
, m_pAnnCheck(Q_NULLPTR)
, m_pStimCheck(Q_NULLPTR)
, m_pCheckRejection(Q_NULLPTR)
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
    m_pButterflyView = new DISPLIB::ButterflyView();
    m_pAverageLayoutView = new DISPLIB::AverageLayoutView();

    QTabWidget* pTabView = new QTabWidget();
    QWidget* testWidget = new QWidget();
    QVBoxLayout* testLayout = new QVBoxLayout();

    pTabView->addTab(m_pButterflyView, "Butterfly View");
    pTabView->addTab(m_pAverageLayoutView, "2D Layout View");

    testLayout->addWidget(pTabView);
    testWidget->setLayout(testLayout);

    testWidget->setMinimumSize(256, 256);
    testWidget->setFocusPolicy(Qt::TabFocus);
    testWidget->setAttribute(Qt::WA_DeleteOnClose, false);

    return testWidget;
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

    m_pAveragingSettingsView->setProcessingMode(DISPLIB::AbstractView::ProcessingMode::Offline);

    // Buttons
    QPushButton* pButton = new QPushButton();
    pButton->setText("COMPUTE");
    m_pAnnCheck = new QRadioButton("Average with annotations");
    m_pStimCheck = new QRadioButton("Average with stim channels");

    m_pCheckRejection = new QCheckBox("Drop Rejected");

    connect(m_pCheckRejection, &QCheckBox::toggled,
            this, &Averaging::onRejectionChecked);

    connect(m_pAnnCheck, &QRadioButton::toggled,
            this, &Averaging::onCheckBoxStateChanged);
    connect(m_pStimCheck, &QRadioButton::toggled,
            this, &Averaging::onCheckBoxStateChanged);
    connect(pButton, &QPushButton::clicked,
            this, &Averaging::onComputeButtonClicked);

    //Sets Default state
    m_pAnnCheck->click();
    //m_pStimCheck->click();

    QGroupBox* pGBox = new QGroupBox();
    QVBoxLayout* pVBLayout = new QVBoxLayout();

    pGBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    pVBLayout->addWidget(m_pAnnCheck);
    pVBLayout->addWidget(m_pStimCheck);
    pVBLayout->addWidget(pButton);

    pGBox->setLayout(pVBLayout);

    m_pLayout->addWidget(m_pAveragingSettingsView);
//    m_pLayout->addWidget(m_pAnnCheck);
//    m_pLayout->addWidget(m_pStimCheck);
    m_pLayout->addWidget(m_pCheckRejection);
    m_pLayout->addWidget(pButton);

    pWidget->setLayout(m_pLayout);

    m_pTabView->addTab(pWidget, "Parameters");

    pControl->setWidget(m_pTabView);
    pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
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
        default:
            qWarning() << "[Averaging::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> Averaging::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

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
        loadFullGUI();
    }
}

//=============================================================================================================

void Averaging::onChangeNumAverages(qint32 numAve)
{
    Q_UNUSED(numAve)
}

//=============================================================================================================

void Averaging::onChangeBaselineFrom(qint32 fromMSeconds)
{
    m_fBaselineFrom = static_cast<float>(fromMSeconds) / 1000.f;
}


//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMSeconds)
{
    m_fBaselineTo = static_cast<float>(toMSeconds) / 1000.f;
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
    clearAveraging();

    if(!m_pFiffRawModel){
        qWarning() << "No model loaded. Cannot calculate average";
        return;
    }

    if(m_pFiffRawModel->getAnnotationModel()->getNumberOfAnnotations() < 2){
        qWarning() << "Not enough data points to calculate average.";
        return;
    }

    MatrixXi matEvents;
    QMap<QString,double> mapReject;
    MNELIB::MNEEpochDataList lstEpochDataList;

    m_pFiffEvoked = QSharedPointer<FIFFLIB::FiffEvoked>(new FIFFLIB::FiffEvoked());
    int iType = 1; //hardwired for now, change later to type
    mapReject.insert("eog", 300e-06);

    FIFFLIB::FiffRawData* pFiffRaw = this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data();

    if (m_bUseAnn){
        matEvents = m_pFiffRawModel->getAnnotationModel()->getAnnotationMatrix();
    } else {
        //NOT IMPLEMENTED
    }

    lstEpochDataList = MNELIB::MNEEpochDataList::readEpochs(*pFiffRaw,
                                                          matEvents,
                                                          m_fPreStim,
                                                          m_fPostStim,
                                                          iType,
                                                          mapReject);

    if(m_bBasline){
        QPair<QVariant, QVariant> baselinePair;
        baselinePair.first = QVariant(m_fBaselineFrom);
        baselinePair.second = QVariant(m_fBaselineTo);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if(m_bRejection){
        lstEpochDataList.dropRejected();
    }

    *m_pFiffEvoked = lstEpochDataList.average(pFiffRaw->info,
                                            0,
                                            lstEpochDataList.first()->epoch.cols());

//    //DEBUG
//    std::cout << "lstEpochDataList.first()->epoch.cols()" << lstEpochDataList.first()->epoch.cols() << std::endl;
//    std::cout << "lstEpochDataList.first()->tmin" << lstEpochDataList.first()->tmin<< std::endl;
//    std::cout << "lstEpochDataList.first()->tmax" << lstEpochDataList.first()->tmax<< std::endl;
//    std::cout << "m_pFiffEvoked->data.block(0,0,10,10)" << m_pFiffEvoked->data.block(0,0,10,10) << std::endl;
//    std::cout << "m_pFiffEvoked->data.cols()" << m_pFiffEvoked->data.cols()<< std::endl;

    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());
    m_pFiffEvokedSet->evoked.append(*(m_pFiffEvoked.data()));
    m_pFiffEvokedSet->info = *(m_pFiffRawModel->getFiffInfo());

    if(m_bBasline){
        m_pFiffEvokedSet->evoked[0].baseline.first = QVariant(m_fBaselineFrom);
        m_pFiffEvokedSet->evoked[0].baseline.second = QVariant(m_fBaselineTo);
    }

    m_pEvokedModel->setEvokedSet(m_pFiffEvokedSet);

    m_pButterflyView->setEvokedSetModel(m_pEvokedModel);
    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    m_pButterflyView->setChannelInfoModel(m_pChannelInfoModel);
    m_pAverageLayoutView->setChannelInfoModel(m_pChannelInfoModel);

    m_pButterflyView->dataUpdate();
    m_pButterflyView->updateView();
    m_pAverageLayoutView->updateData();

    qInfo() << "[Averaging::computeAverage] Average computed.";
}

//=============================================================================================================

void Averaging::onCheckBoxStateChanged()
{
    if (m_pAnnCheck->isChecked()){
        m_bUseAnn = true;
    } else {
        m_bUseAnn = false;
    }
}

//=============================================================================================================

void Averaging::loadFullGUI()
{
    //This function needs to be called after we have the FiffRawModel, because we need FiffInfo to initialize objects herein
    m_pFiffInfo = m_pFiffRawModel->getFiffInfo(0);

    //Init Models
    m_pChannelInfoModel = DISPLIB::ChannelInfoModel::SPtr::create(m_pFiffInfo);
    m_pEvokedModel = QSharedPointer<DISPLIB::EvokedSetModel>(new DISPLIB::EvokedSetModel());
    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());

    //Channel Selection View
    m_pChannelSelectionView = QSharedPointer<DISPLIB::ChannelSelectionView>(new DISPLIB::ChannelSelectionView(QString("MNEANALYZE/AVERAGING"),
                                                                           Q_NULLPTR,
                                                                           m_pChannelInfoModel,
                                                                           Qt::Window));

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::loadedLayoutMap,
            m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::layoutChanged);

    connect(m_pChannelInfoModel.data(), &DISPLIB::ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::setCurrentlyMappedFiffChannels);

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::showSelectedChannelsOnly,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::showSelectedChannelsOnly);

    connect(m_pChannelSelectionView.data(), &DISPLIB::ChannelSelectionView::selectionChanged,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::channelSelectionManagerChanged);

    QPushButton* pChanSelButton = new QPushButton();
    pChanSelButton->setText("Channel Selection");
    connect(pChanSelButton, &QPushButton::clicked,
            this, &Averaging::onChannelButtonClicked);

    //Init View components
    m_pButterflyView->setChannelInfoModel(m_pChannelInfoModel);
    m_pChannelSelectionView->updateDataView();
    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);
    m_pAverageLayoutView->setChannelInfoModel(m_pChannelInfoModel);
    m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

    //Scaling View
    DISPLIB::ScalingView* pScalingView = new DISPLIB::ScalingView(QString("MNEANALYZE/AVERAGING"));
    pScalingView->setObjectName("group_tab_View_Scaling");

    connect(pScalingView, &DISPLIB::ScalingView::scalingChanged,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setScaleMap);

    connect(pScalingView, &DISPLIB::ScalingView::scalingChanged,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setScaleMap);


    m_pButterflyView->setScaleMap(pScalingView->getScaleMap());
    m_pAverageLayoutView->setScaleMap(pScalingView->getScaleMap());

    //Modality selection
    DISPLIB::ModalitySelectionView* pModalitySelectionView = new DISPLIB::ModalitySelectionView(m_pFiffInfo->chs,
                                                                              QString("MNEANALYZE/AVERAGING"));
    pModalitySelectionView->setObjectName("group_tab_View_Modalities");

    connect(pModalitySelectionView, &DISPLIB::ModalitySelectionView::modalitiesChanged,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setModalityMap);

    m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

    // Quick control average selection
    DISPLIB::AverageSelectionView* pAverageSelectionView = new DISPLIB::AverageSelectionView(QString("MNEANALYZE/AVERAGING"));
    pAverageSelectionView->setObjectName("group_tab_View_Selection");

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            pAverageSelectionView, &DISPLIB::AverageSelectionView::setAverageActivation);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            pAverageSelectionView, &DISPLIB::AverageSelectionView::setAverageColor);

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageColor);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageActivation);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageActivationMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageActivation);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageColorMap,
            m_pButterflyView.data(), &DISPLIB::ButterflyView::setAverageColor);

    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageColorMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageColor);
    connect(m_pEvokedModel.data(), &DISPLIB::EvokedSetModel::newAverageActivationMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageActivation);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageActivationMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageActivation);
    connect(pAverageSelectionView, &DISPLIB::AverageSelectionView::newAverageColorMap,
            m_pAverageLayoutView.data(), &DISPLIB::AverageLayoutView::setAverageColor);

    m_pEvokedModel->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pEvokedModel->setAverageColor(pAverageSelectionView->getAverageColor());
    m_pButterflyView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pButterflyView->setAverageColor(pAverageSelectionView->getAverageColor());
    m_pAverageLayoutView->setAverageActivation(pAverageSelectionView->getAverageActivation());
    m_pAverageLayoutView->setAverageColor(pAverageSelectionView->getAverageColor());

    //Add new widgets
    m_pLayout->addWidget(pChanSelButton);
    m_pTabView->addTab(pScalingView, "Scaling");
    m_pTabView->addTab(pModalitySelectionView, "Modality");
//    m_pTabView->addTab(pAverageSelectionView, "Average Selection");

    //Update saved params
    m_fBaselineFrom = static_cast<float>(m_pAveragingSettingsView->getBaselineFromSeconds())/1000.f;
    m_fBaselineTo = static_cast<float>(m_pAveragingSettingsView->getBaselineToSeconds())/1000.f;

    m_fPreStim = -(static_cast<float>(m_pAveragingSettingsView->getPreStimMSeconds())/1000.f);
    m_fPostStim = static_cast<float>(m_pAveragingSettingsView->getPostStimMSeconds())/1000.f;
}

//=============================================================================================================

void Averaging::onChannelButtonClicked()
{
    if(m_pChannelSelectionView->isActiveWindow()) {
        m_pChannelSelectionView->hide();
    } else {
        m_pChannelSelectionView->activateWindow();
        m_pChannelSelectionView->show();
    }
}

//=============================================================================================================

void Averaging::clearAveraging()
{
    m_pFiffEvokedSet->evoked.clear();
    m_pFiffEvokedSet.clear();
    m_pFiffEvoked.clear();
}

//=============================================================================================================

void Averaging::onRejectionChecked()
{
    if (m_pCheckRejection->isChecked()){
        m_bRejection = true;
    } else {
        m_bRejection = false;
    }
}
