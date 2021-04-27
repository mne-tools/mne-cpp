//=============================================================================================================
/**
 * @file     timefrequency.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the TimeFrequency class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/progressview.h>
#include <disp/viewers/timefrequencyview.h>
#include <disp/viewers/timefrequencylayoutview.h>
#include <disp/viewers/timefrequencysettingsview.h>
#include <disp/viewers/helpers/timefrequencymodel.h>
#include <disp/viewers/helpers/selectionsceneitem.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/plots/tfplot.h>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>

#include <rtprocessing/timefrequency.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TIMEFREQUENCYPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequency::TimeFrequency()
: m_sSettingsPath("MNEANALYZE/TimeFrequency")
, m_pAvgModel(Q_NULLPTR)
{
    m_pEvokedModel = QSharedPointer<DISPLIB::EvokedSetModel>(new DISPLIB::EvokedSetModel());
    loadSettings();
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{

}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TimeFrequency::clone() const
{
    QSharedPointer<TimeFrequency> pTimeFrequencyClone(new TimeFrequency);
    return pTimeFrequencyClone;
}

//=============================================================================================================

void TimeFrequency::init()
{
    m_pCommu = new Communicator(this);

    m_pTFModel = QSharedPointer<DISPLIB::TimeFrequencyModel>(new DISPLIB::TimeFrequencyModel());
}

//=============================================================================================================

void TimeFrequency::unload()
{

}

//=============================================================================================================

QString TimeFrequency::getName() const
{
    return "Time-Frequency";
}

//=============================================================================================================

QMenu *TimeFrequency::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *TimeFrequency::getControl()
{
    QDockWidget* pDock = new QDockWidget(getName());

//    QWidget* pWidget = new QWidget(pDock);
//    QVBoxLayout* pLayout = new QVBoxLayout(pDock);
//    QPushButton* pButton = new QPushButton("Press me.", pWidget);

//    pLayout->addWidget(pButton);
//    pWidget->setLayout(pLayout);
//    pDock->setWidget(pWidget);

//    connect(pButton, &QPushButton::pressed,
//            this, &TimeFrequency::computeTimeFreqency);

    DISPLIB::TimeFrequencySettingsView* pSettings = new DISPLIB::TimeFrequencySettingsView();

    connect(pSettings, &DISPLIB::TimeFrequencySettingsView::computePushed,
            this, &TimeFrequency::computeTimeFreqency, Qt::UniqueConnection);

    connect(pSettings, &DISPLIB::TimeFrequencySettingsView::minFreqChanged,
            m_pTFModel.data(), &DISPLIB::TimeFrequencyModel::setMinFreq, Qt::UniqueConnection);

    connect(pSettings, &DISPLIB::TimeFrequencySettingsView::maxFreqChanged,
            m_pTFModel.data(), &DISPLIB::TimeFrequencyModel::setMaxFreq, Qt::UniqueConnection);

    pDock->setWidget(pSettings);


    return pDock;
}

//=============================================================================================================

QWidget *TimeFrequency::getView()
{
    QWidget* pTimeFreqViewWidget = new QWidget();
    QTabWidget* pTabView = new QTabWidget(pTimeFreqViewWidget);
    QVBoxLayout* pViewLayout = new QVBoxLayout();

    m_pTimeFreqView = new DISPLIB::TimeFrequencyView();
    m_pTimeFreqLayoutView = new DISPLIB::TimeFrequencyLayoutView();

    m_pTimeFreqView->setTimeFrequencyModel(m_pTFModel);
    m_pTimeFreqLayoutView->setTimeFrequencyModel(m_pTFModel);

    pTabView->addTab(m_pTimeFreqView, "Average View");
    pTabView->addTab(m_pTimeFreqLayoutView, "Layout View");

    pViewLayout->addWidget(pTabView);
    pTimeFreqViewWidget->setLayout(pViewLayout);

    return pTimeFreqViewWidget;
}

//=============================================================================================================

void TimeFrequency::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:{
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    }
    case EVENT_TYPE::CHANNEL_SELECTION_ITEMS:{
        setChannelSelection(e->getData());
    }
    default:
        qWarning() << "[Averaging::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> TimeFrequency::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(CHANNEL_SELECTION_ITEMS);

    return temp;
}

//=============================================================================================================

void TimeFrequency::saveSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
}

//=============================================================================================================

void TimeFrequency::loadSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
}

//=============================================================================================================

void TimeFrequency::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        m_pAvgModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
        m_pEvokedModel->setEvokedSet(m_pAvgModel->data(QModelIndex()).value<QSharedPointer<FIFFLIB::FiffEvokedSet>>());
        m_pTimeFreqView->setEvokedSetModel(m_pEvokedModel);
    }
}

//=============================================================================================================

void TimeFrequency::setChannelSelection(const QVariant &data)
{
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("layoutview")){
        if(m_pTimeFreqLayoutView){
            m_pTimeFreqLayoutView->channelSelectionChanged(data);
        }
    }
    if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("butterflyview")){
//        if(data.value<DISPLIB::SelectionItem*>()->m_bShowAll){
//            emit showAllChannels();
//        } else {
//            emit showSelectedChannels(data.value<DISPLIB::SelectionItem*>()->m_iChannelNumber);
//        }

    }

    if(m_pTFModel){
        if(data.value<DISPLIB::SelectionItem*>()->m_sViewsToApply.contains("butterflyview")){
            m_pTFModel->setChannelSelection(data.value<DISPLIB::SelectionItem*>()->m_iChannelNumber);
        }
    }
}

//=============================================================================================================

void TimeFrequency::computeTimeFreqency()
{
    qDebug() << "[TimeFrequency::computeTimeFreqency]";
    if (!m_pAvgModel){
        return;
    }

    auto spectr = RTPROCESSINGLIB::TimeFrequencyData::computeComplexTimeFrequency(*m_pAvgModel->getEvokedSet());

//    DISPLIB::TFplot* tfplot = new DISPLIB::TFplot(spectr.front(), m_pAvgModel->getEvokedSet()->evoked.first().info.sfreq, 0, 100, DISPLIB::ColorMaps::Jet);
//    tfplot->show();

    auto spectr2 = RTPROCESSINGLIB::TimeFrequencyData::computeTimeFrequency(*m_pAvgModel->getEvokedSet());

    DISPLIB::TFplot* tfplot2 = new DISPLIB::TFplot(spectr2.front(), m_pAvgModel->getEvokedSet()->evoked.first().info.sfreq, 0, 100, DISPLIB::ColorMaps::Jet);
    tfplot2->show();

    DISPLIB::TFplot* tfplot3 = new DISPLIB::TFplot(spectr2.at(20), m_pAvgModel->getEvokedSet()->evoked.first().info.sfreq, 0, 100, DISPLIB::ColorMaps::Jet);
    tfplot3->show();

    DISPLIB::TFplot* tfplot4 = new DISPLIB::TFplot(spectr2.at(80), m_pAvgModel->getEvokedSet()->evoked.first().info.sfreq, 0, 100, DISPLIB::ColorMaps::Jet);
    tfplot4->show();


    m_pTFModel->setFiffInfo(m_pAvgModel->getEvokedSet()->evoked.first().info);
    m_pTFModel->setSpectr(spectr);
}
