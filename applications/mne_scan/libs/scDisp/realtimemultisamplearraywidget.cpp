//=============================================================================================================
/**
 * @file     realtimemultisamplearraywidget.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lars Debor, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RealTimeMultiSampleArrayWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearraywidget.h"

#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/rtfiffrawview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/spharasettingsview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/triggerdetectionview.h>

#include <scMeas/realtimemultisamplearray.h>

#include <rtprocessing/helpers/filterkernel.h>

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QDate>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QSettings>
#include <QToolBar>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayWidget::RealTimeMultiSampleArrayWidget(QSharedPointer<QTime> &pTime,
                                                               QWidget* parent)
: MeasurementWidget(parent)
, m_iMaxFilterTapSize(-1)
{
    Q_UNUSED(pTime)

    qRegisterMetaType<QMap<int,QList<QPair<int,double> > > >();
}

//=============================================================================================================

RealTimeMultiSampleArrayWidget::~RealTimeMultiSampleArrayWidget()
{
    QSettings settings("MNECPP");

    if(m_pChannelDataView && m_pRTMSA) {
        settings.setValue(QString("RTMSAW/showHideBad"), m_pChannelDataView->getBadChannelHideStatus());
    }
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(!m_pRTMSA) {
        m_pRTMSA = qSharedPointerDynamicCast<RealTimeMultiSampleArray>(pMeasurement);
    }

    if(m_pRTMSA) {
        if(m_pRTMSA->isChInit() && !m_pFiffInfo) {
            m_pFiffInfo = m_pRTMSA->info();
            m_iMaxFilterTapSize = m_pRTMSA->getMultiSampleArray().first().cols();

            if(!m_bDisplayWidgetsInitialized) {
                initDisplayControllWidgets();
            }
        }
        if (!m_pRTMSA->getMultiSampleArray().isEmpty()) {
            //Add data to table view
            m_pChannelDataView->addData(m_pRTMSA->getMultiSampleArray());
        }
    }
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::initDisplayControllWidgets()
{
    if(m_pFiffInfo) {        
        //Create table view and set layout
        m_pChannelDataView = new RtFiffRawView(QString("MNESCAN/RTMSAW"),
                                               this);
        m_pChannelDataView->hide();

        QVBoxLayout *rtmsaLayout = new QVBoxLayout(this);
        rtmsaLayout->setContentsMargins(0,0,0,0);
        this->setLayout(rtmsaLayout);
        this->setMinimumSize(300,50);

        // Prepare actions
        QToolBar* pToolBar = new QToolBar;

        QAction* pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the channel selection view"),this);
        pActionSelectSensors->setToolTip(tr("Show the channel selection view"));
        connect(pActionSelectSensors, &QAction::triggered,
                this, &RealTimeMultiSampleArrayWidget::showSensorSelectionWidget);
        pActionSelectSensors->setVisible(true);
        pToolBar->addAction(pActionSelectSensors);

        m_pActionHideBad = new QAction(QIcon(":/images/hideBad.png"), tr("Toggle bad channel visibility"),this);
        m_pActionHideBad->setStatusTip(tr("Toggle bad channel visibility"));
        connect(m_pActionHideBad.data(), &QAction::triggered,
                this, &RealTimeMultiSampleArrayWidget::onHideBadChannels);
        m_pActionHideBad->setVisible(true);
        pToolBar->addAction(m_pActionHideBad);

        // Add toolbar and channel data view
        rtmsaLayout->addWidget(pToolBar);
        rtmsaLayout->addWidget(m_pChannelDataView);

        // Init channel view
        QSettings settings("MNECPP");
        QString sRTMSAWName = m_pRTMSA->getName();

        m_pChannelDataView->show();
        m_pChannelDataView->init(m_pFiffInfo);

        if(settings.value(QString("RTMSAW/showHideBad"), false).toBool()) {
            this->onHideBadChannels();
        }

        //Init channel selection manager
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = ChannelSelectionView::SPtr::create(QString("MNESCAN/RTMSAW"),
                                                                     this,
                                                                     m_pChannelInfoModel,
                                                                     Qt::Window);
        m_pChannelSelectionView->setWindowTitle(tr(QString("%1: Channel Selection Window").arg(sRTMSAWName).toUtf8()));

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pChannelDataView.data(), &RtFiffRawView::showSelectedChannelsOnly);

        connect(m_pChannelDataView.data(), &RtFiffRawView::channelMarkingChanged,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

        //Init control widgets
        QList<QWidget*> lControlWidgets;

//        // Quick control projectors
//        ProjectorsView* pProjectorsView = new ProjectorsView(QString("MNESCAN/RTMSAW"));
//        pProjectorsView->setObjectName("group_tab_Noise_SSP");
//        lControlWidgets.append(pProjectorsView);

//        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::updateProjection);

//        pProjectorsView->setProjectors(m_pFiffInfo->projs);

//        // Quick control compensators
//        CompensatorView* pCompensatorView = new CompensatorView(QString("MNESCAN/RTMSAW"));
//        pCompensatorView->setObjectName("group_tab_Noise_Comp");
//        lControlWidgets.append(pCompensatorView);

//        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::updateCompensator);

//        pCompensatorView->setCompensators(m_pFiffInfo->comps);

//        // Quick control filter
//        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("MNESCAN/RTMSAW"));
//        pFilterSettingsView->setObjectName("group_tab_Noise_Filter");
//        lControlWidgets.append(pFilterSettingsView);

//        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::setFilterChannelType);

//        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::setFilter);

//        connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::setFilterActive);

//        m_pChannelDataView->setFilterActive(pFilterSettingsView->getFilterActive());
//        m_pChannelDataView->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());
//        pFilterSettingsView->getFilterView()->setWindowSize(m_iMaxFilterTapSize);
//        pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(m_iMaxFilterTapSize);
//        pFilterSettingsView->getFilterView()->init(m_pFiffInfo->sfreq);

//        // Quick control SPHARA settings
//        SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView();
//        pSpharaSettingsView->setObjectName("group_tab_Noise_SPHARA");
//        lControlWidgets.append(pSpharaSettingsView);

//        connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::updateSpharaActivation);

//        connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
//                m_pChannelDataView.data(), &RtFiffRawView::updateSpharaOptions);

        // Quick control scaling
        ScalingView* pScalingView = new ScalingView(QString("MNESCAN/RTMSAW"), 0, Qt::Widget, m_pFiffInfo->get_channel_types());
        pScalingView->setObjectName("group_tab_View_Scaling");
        lControlWidgets.append(pScalingView);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setScalingMap);

        m_pChannelDataView->setScalingMap(pScalingView->getScaleMap());

        // Quick control channel data settings
        FiffRawViewSettings* pChannelDataSettingsView = new FiffRawViewSettings(QString("MNESCAN/RTMSAW"));
        pChannelDataSettingsView->setObjectName("group_tab_View_General");
        lControlWidgets.append(pChannelDataSettingsView);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::signalColorChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setSignalColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::backgroundColorChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setBackgroundColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::zoomChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setZoom);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::timeWindowChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setWindowSize);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::distanceTimeSpacerChanged,
                m_pChannelDataView.data(), &RtFiffRawView::setDistanceTimeSpacer);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::makeScreenshot,
                this, &RealTimeMultiSampleArrayWidget::onMakeScreenshot);

        m_pChannelDataView->setZoom(pChannelDataSettingsView->getZoom());
        m_pChannelDataView->setWindowSize(pChannelDataSettingsView->getWindowSize());
        m_pChannelDataView->setDistanceTimeSpacer(pChannelDataSettingsView->getDistanceTimeSpacer());
        m_pChannelDataView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
        m_pChannelDataView->setSignalColor(pChannelDataSettingsView->getSignalColor());

        // Quick control trigger detection settings
        TriggerDetectionView* pTriggerDetectionView = new TriggerDetectionView(QString("MNESCAN/RTMSAW"));
        pTriggerDetectionView->setObjectName("group_tab_View_Triggers");
        lControlWidgets.append(pTriggerDetectionView);

        connect(pTriggerDetectionView, &TriggerDetectionView::triggerInfoChanged,
                m_pChannelDataView.data(), &RtFiffRawView::triggerInfoChanged);

        connect(pTriggerDetectionView, &TriggerDetectionView::resetTriggerCounter,
                m_pChannelDataView.data(), &RtFiffRawView::resetTriggerCounter);

        connect(m_pChannelDataView.data(), &RtFiffRawView::triggerDetected,
                pTriggerDetectionView, &TriggerDetectionView::setNumberDetectedTriggersAndTypes);

        pTriggerDetectionView->init(m_pFiffInfo);

        emit displayControlWidgetsChanged(lControlWidgets, sRTMSAWName);

        //Initialized
        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::showSensorSelectionWidget()
{
    if(m_pChannelSelectionView->isActiveWindow()) {
        m_pChannelSelectionView->hide();
    } else {
        m_pChannelSelectionView->activateWindow();
        m_pChannelSelectionView->show();
    }
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName;

    if(imageType.contains("SVG")) {
        fileName = QString("./Screenshots/%1-%2-DataView.svg").arg(sDate).arg(sTime);
    } else if(imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-DataView.png").arg(sDate).arg(sTime);
    }

    m_pChannelDataView->takeScreenshot(fileName);
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::onHideBadChannels()
{
    m_pChannelDataView->hideBadChannels();

    if(m_pActionHideBad->toolTip() == "Show all bad channels") {
        m_pActionHideBad->setIcon(QIcon(":/images/hideBad.png"));
        m_pActionHideBad->setToolTip("Hide all bad channels");
        m_pActionHideBad->setStatusTip(tr("Hide all bad channels"));
    } else {
        m_pActionHideBad->setIcon(QIcon(":/images/showBad.png"));
        m_pActionHideBad->setToolTip("Show all bad channels");
        m_pActionHideBad->setStatusTip(tr("Show all bad channels"));
    }
}

//=============================================================================================================

void RealTimeMultiSampleArrayWidget::updateOpenGLViewport()
{
    if(m_pChannelDataView) {
        m_pChannelDataView->updateOpenGLViewport();
    }
}
