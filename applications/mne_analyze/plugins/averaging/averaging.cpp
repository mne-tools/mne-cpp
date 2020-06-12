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

#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/averagingsettingsview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/butterflyview.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne_epoch_data.h>

#include <fiff/fiff_evoked.h>

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
, m_iNumAve(0)
, m_iBaselineFrom(0)
, m_iBaselineTo(0)
, m_fPreStim(0)
, m_fPostStim(0)
, m_bUseAnn(0)
, m_pAnnCheck(Q_NULLPTR)
, m_pStimCheck(Q_NULLPTR)
{
    qDebug() << "[Averaging::Averaging]";
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
    qDebug() << "[Averaging::getView]";

    //return Q_NULLPTR;
    m_pButterflyView = new DISPLIB::ButterflyView();
    m_pAverageLayoutView = new DISPLIB::AverageLayoutView();

    QTabWidget* pTabView = new QTabWidget();

//    return m_pButterflyView;

    pTabView->addTab(m_pButterflyView, "Butterfly View");
    pTabView->addTab(m_pAverageLayoutView, "2D Layout View");

    QWidget* testWidget = new QWidget();
    QVBoxLayout* testLayout = new QVBoxLayout();
    //QLabel* testLabel = new QLabel("Test Test");

    testLayout->addWidget(pTabView);
    testWidget->setLayout(testLayout);

//    testLayout->addWidget(m_pButterflyView);
//    testWidget->setLayout(testLayout);

    qDebug() << "Created Widget";

    testWidget->setMinimumSize(256, 256);
    testWidget->setFocusPolicy(Qt::TabFocus);
    testWidget->setAttribute(Qt::WA_DeleteOnClose, false);

    return testWidget;

//    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget* Averaging::getControl()
{
    QVBoxLayout* pLayout = new QVBoxLayout;
    QWidget* pWidget = new QWidget();
    m_pAveragingSettingsView = new DISPLIB::AveragingSettingsView(QString("MNEANALYZE/%1").arg(this->getName()));
    QDockWidget* pControl = new QDockWidget(getName());
    QPushButton* pButton = new QPushButton();
    pButton->setText("COMPUTE");

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
    connect(pButton, &QPushButton::clicked,
            this, &Averaging::onComputeButtonClicked);

    m_pAnnCheck = new QRadioButton("Average with annotations");
    m_pStimCheck = new QRadioButton("Average with stim channels");

    connect(m_pAnnCheck, &QCheckBox::toggled,
            this, &Averaging::onCheckBoxStateChanged);

    connect(m_pStimCheck, &QRadioButton::toggled,
            this, &Averaging::onCheckBoxStateChanged);

    //Sets Default state
    m_pAnnCheck->click();
//    m_pStimCheck->click()

//    QGroupBox* pGBox = new QGroupBox();
//    QVBoxLayout* pVBLayout = new QVBoxLayout();

//    pGBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
//    pVBLayout->addWidget(m_pAnnCheck);
//    pVBLayout->addWidget(m_pStimCheck);
//    pVBLayout->addWidget(pButton);

//    pGBox->setLayout(pVBLayout);

    pLayout->addWidget(m_pAveragingSettingsView);
//    pLayout->addWidget(pGBox);
    pLayout->addWidget(m_pAnnCheck);
    pLayout->addWidget(m_pStimCheck);
    pLayout->addWidget(pButton);
    pWidget->setLayout(pLayout);

    pControl->setWidget(pWidget);
    pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    pControl->setObjectName("Averaging");
    pControl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Preferred));

    m_iNumAve = m_pAveragingSettingsView->getNumAverages();
    m_iBaselineFrom = m_pAveragingSettingsView->getBaselineFromSeconds();
    m_iBaselineTo = m_pAveragingSettingsView->getBaselineToSeconds();

    m_fPreStim = m_pAveragingSettingsView->getPreStimSeconds();
    m_fPostStim = m_pAveragingSettingsView->getPostStimSeconds();

    return pControl;
}

//=============================================================================================================

void Averaging::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            qDebug() << "[Averaging::handleEvent] SELECTED_MODEL_CHANGED";
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
                qDebug() << "[Averaging::onModelChanged] Model is the same";
                return;
            }
        }
        m_pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        qDebug() << "[Averaging::onModelChanged] New model loaded";
    }
}

//=============================================================================================================

void Averaging::onChangeNumAverages(qint32 numAve)
{
    qDebug() << "[Averaging::onChangeNumAverages]" << numAve;
    m_iNumAve = numAve;
//    if(m_pAve) {
//        m_pAve->setAverageNumber(numAve);
//    }
}

//=============================================================================================================

void Averaging::onChangeBaselineFrom(qint32 fromMSeconds)
{

    qDebug() << "[Averaging::onChangeBaselineFrom]" << fromMSeconds;
    m_iBaselineFrom = fromMSeconds;
//    if(!m_pFiffInfo) {
//        return;
//    }

//    int iBaselineFromSamples = ((float)(fromMSeconds)/1000)*m_pFiffInfo->sfreq;

//    if(m_pAve) {
//        m_pAve->setBaselineFrom(iBaselineFromSamples, fromMSeconds);
//    }
}


//=============================================================================================================

void Averaging::onChangeBaselineTo(qint32 toMSeconds)
{
    qDebug() << "[Averaging::onChangeBaselineTo]" << toMSeconds;
    m_iBaselineTo = toMSeconds;
//    if(!m_pFiffInfo) {
//        return;
//    }

//    int iBaselineToSamples = ((float)(toMSeconds)/1000)*m_pFiffInfo->sfreq;

//    if(m_tAve) {
//        m_tAve->setBaselineTo(iBaselineToSamples, toMSeconds);
//    }
}

//=============================================================================================================

void Averaging::onChangePreStim(qint32 mseconds)
{
    qDebug() << "[Averaging::onChangePreStim]" << mseconds;

    m_fPreStim =  -(static_cast<float>(mseconds)/1000);

//    if(!m_pFiffInfo) {
//        return;
//    }

//    int iPreStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

//    if(m_pAveragingOutput) {
//        m_pAveragingOutput->data()->setNumPreStimSamples(iPreStimSamples);
//    }

//    if(m_pAve) {
//        m_pAve->setPreStim(iPreStimSamples, mseconds);
//    }
}

//=============================================================================================================

void Averaging::onChangePostStim(qint32 mseconds)
{

    qDebug() << "[Averaging::onChangePostStim]";

    m_fPostStim = (static_cast<float>(mseconds)/1000);
//    if(!m_pFiffInfo) {
//        return;
//    }

//    int iPostStimSamples = ((float)(mseconds)/1000)*m_pFiffInfo->sfreq;

//    if(m_pAve) {
//        m_pAve->setPostStim(iPostStimSamples, mseconds);
//    }
}

//=============================================================================================================

void Averaging::onChangeBaselineActive(bool state)
{
    qDebug() << "[Averaging::onChangeBaselineActive]";
    if(m_pAve) {
        m_pAve->setBaselineActive(state);
    }
}

//=============================================================================================================

void Averaging::onResetAverage(bool state)
{
    Q_UNUSED(state)

    qDebug() << "[Averaging::onResetAverage]";
    if(m_pAve) {
        m_pAve->reset();
    }
}

//=============================================================================================================

void Averaging::onComputeButtonClicked(bool bChecked)
{
    qDebug() << "[Averaging::onComputeButtonCLicked]";
    Q_UNUSED(bChecked);
//    m_lTriggerList = QSharedPointer<QList<QPair<int,double>>>(new QList<QPair<int,double>>);

//    for (int i = 0; i < m_pFiffRawModel->getTimeListSize(); i++){
//        qDebug() << "At" <<  i << ":" << m_pFiffRawModel->getTimeMarks(i);
//        m_lTriggerList->append(QPair<int, double>(i, m_pFiffRawModel->getTimeMarks(i)));
//    }

//    QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(m_pFiffRawModel->getFiffInfo());

//    float fFreq = m_pFiffRawModel->getFiffInfo()->sfreq;

//    int iPreStimSamples = ((float)m_pAveragingSettingsView->getPreStimSeconds()/1000)*fFreq;
//    int iPostStimSamples = ((float)m_pAveragingSettingsView->getPostStimSeconds()/1000)*fFreq;
//    int iBaselineFromSamples = ((float)m_pAveragingSettingsView->getBaselineFromSeconds()/1000)*fFreq;
//    int iBaselineToSamples = ((float)m_pAveragingSettingsView->getBaselineToSeconds()/1000)*fFreq;

//    m_pAve = QSharedPointer<Ave>(new Ave(m_pAveragingSettingsView->getNumAverages(),
//                                         iPreStimSamples,
//                                         iPostStimSamples,
//                                         m_pAveragingSettingsView->getBaselineFromSeconds(),
//                                         m_pAveragingSettingsView->getBaselineToSeconds(),
//                                         0, //temp value, change later
//                                         pFiffInfo,
//                                         m_lTriggerList));

//    m_pAve->setBaselineFrom(iBaselineFromSamples, m_pAveragingSettingsView->getBaselineFromSeconds());
//    m_pAve->setBaselineTo(iBaselineToSamples, m_pAveragingSettingsView->getBaselineToSeconds());
//    m_pAve->setBaselineActive(m_pAveragingSettingsView->getDoBaselineCorrection());

//    connect(m_pAve.data(), &Ave::evokedStim,
//            this, &Averaging::onNewEvokedSet);
    if(!m_pFiffRawModel){
        qWarning() << "No model loaded. Cannot calculate average";
        return;
    }

    if(m_pFiffRawModel->getAnnotationModel()->getNumberOfAnnotations() < 2){
        qWarning() << "Not enough annotations to calculate average.";
        return;
    }

    MatrixXi matEvents;
    QMap<QString,double> mapReject;
    MNELIB::MNEEpochDataList lstEpochDataList;
//    FIFFLIB::FiffEvoked FiffEvoked;

    m_pFiffEvoked = QSharedPointer<FIFFLIB::FiffEvoked>(new FIFFLIB::FiffEvoked());
    int iType = 1; //hardwired for now, change later to annotation type
    mapReject.insert("eog", 300e-06);

    FIFFLIB::FiffRawData* pFiffRaw = this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data();
    //*(this->m_pFiffRawModel->getFiffIO()->m_qlistRaw.first().data());

    qDebug() << "Initialized varibles";

    if (m_bUseAnn){
        qDebug() << "using annotations";
        matEvents = m_pFiffRawModel->getAnnotationModel()->getAnnotationMatrix();

        qDebug() << "Event Matrix:";
        std::cout << matEvents;
    } else {
        qDebug() << "using stim";
        //TODO : add reading from stim channels
    }


    qDebug() << "PreStim:" << m_fPreStim <<", Post Stim:" << m_fPostStim;
    qDebug() << "Type:" << iType;

    lstEpochDataList = MNELIB::MNEEpochDataList::readEpochs(*pFiffRaw,
                                                          matEvents,
                                                          m_fPreStim,
                                                          m_fPostStim,
                                                          iType,
                                                          mapReject);

    lstEpochDataList.dropRejected();

    qDebug() << "Got Epoch List and dropped rejected";

//    FiffEvoked = pEpochDataList.average(pFiffRaw->info,
//                                            pFiffRaw->first_samp,
//                                            pFiffRaw->last_samp);

    *m_pFiffEvoked = lstEpochDataList.average(pFiffRaw->info,
                                            pFiffRaw->first_samp,
                                            pFiffRaw->last_samp);



    //m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>(new FIFFLIB::FiffEvokedSet());
    //m_pFiffEvokedSet->evoked.append(*(m_pFiffEvoked.data()));
    //m_pFiffEvokedSet->info = *(m_pFiffInfo.data());


//    m_pEvokedModel = QSharedPointer<DISPLIB::EvokedSetModel>(new DISPLIB::EvokedSetModel());
//    m_pEvokedModel->setEvokedSet(m_pFiffEvokedSet);

//    m_pButterflyView->setEvokedSetModel(m_pEvokedModel);
//    m_pAverageLayoutView->setEvokedSetModel(m_pEvokedModel);

    qDebug() << "Averaging done.";
}

//=============================================================================================================

void Averaging::onCheckBoxStateChanged()
{
    qDebug() << "[Averaging::onCheckBoxStateChanged]";

    if (m_pAnnCheck->isChecked()){
        m_bUseAnn = true;
    } else {
        m_bUseAnn = false;
    }

    qDebug() << "useAnn:" << m_bUseAnn;
}
