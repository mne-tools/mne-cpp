//=============================================================================================================
/**
 * @file     realtimemultisamplearraywidget.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearraywidget.h"

#include <disp/viewers/quickcontrolview.h>
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

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAction>
#include <QDate>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayWidget::RealTimeMultiSampleArrayWidget(QSharedPointer<RealTimeMultiSampleArray> pRTMSA,
                                                               QSharedPointer<QTime> &pTime,
                                                               QWidget* parent)
: MeasurementWidget(parent)
, m_pRTMSA(pRTMSA)
, m_bInitialized(false)
, m_iMaxFilterTapSize(0)
{
    Q_UNUSED(pTime)

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Shows the region selection widget (F9)"),this);
    m_pActionSelectSensors->setShortcut(tr("F9"));
    m_pActionSelectSensors->setToolTip(tr("Shows the region selection widget (F9)"));
    connect(m_pActionSelectSensors.data(), &QAction::triggered,
            this, &RealTimeMultiSampleArrayWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(true);

    m_pActionHideBad = new QAction(QIcon(":/images/hideBad.png"), tr("Toggle all bad channels"),this);
    m_pActionHideBad->setStatusTip(tr("Toggle all bad channels"));
    connect(m_pActionHideBad.data(), &QAction::triggered,
            this, &RealTimeMultiSampleArrayWidget::onHideBadChannels);
    addDisplayAction(m_pActionHideBad);
    m_pActionHideBad->setVisible(true);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget"),this);
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeMultiSampleArrayWidget::showQuickControlView);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

    //Create table view and set layout
    m_pChannelDataView = new RtFiffRawView(QString("RTMSAW/%1").arg(m_pRTMSA->getName()),
                                             this);
    m_pChannelDataView->hide();

    // Quick control selection
    m_pQuickControlView = QuickControlView::SPtr::create(QString("RTMSAW/%1").arg(m_pRTMSA->getName()),
                                                         "RT Display",
                                                         Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint,
                                                         this);

    this->addControlWidgets(m_pQuickControlView,
                            m_pRTMSA->getControlWidgets());

    QVBoxLayout *rtmsaLayout = new QVBoxLayout(this);
    rtmsaLayout->setContentsMargins(0,0,0,0);
    rtmsaLayout->addWidget(m_pChannelDataView);
    this->setLayout(rtmsaLayout);

    qRegisterMetaType<QMap<int,QList<QPair<int,double> > > >();
}


//*************************************************************************************************************

RealTimeMultiSampleArrayWidget::~RealTimeMultiSampleArrayWidget()
{
    QSettings settings;

    if(m_pChannelDataView && m_pRTMSA) {
        settings.setValue(QString("RTMSAW/%1/showHideBad").arg(m_pRTMSA->getName()), m_pChannelDataView->getBadChannelHideStatus());
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::update(SCMEASLIB::Measurement::SPtr)
{
    if(!m_bInitialized) {
        if(m_pRTMSA->isChInit()) {
            m_pFiffInfo = m_pRTMSA->info();

            m_iMaxFilterTapSize = m_pRTMSA->getMultiSampleArray().at(m_pRTMSA->getMultiSampleArray().size()-1).cols();

            init();
        }
    } else {
        //Add data to table view
        m_pChannelDataView->addData(m_pRTMSA->getMultiSampleArray());
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::init()
{
    if(m_pFiffInfo) {
        QSettings settings;
        QString sRTMSAWName = m_pRTMSA->getName();

        // Init channel view
        m_pChannelDataView->show();
        m_pChannelDataView->init(m_pFiffInfo);

        if(settings.value(QString("RTMSAW/%1/showHideBad").arg(sRTMSAWName), false).toBool()) {
            this->onHideBadChannels();
        }

        //Init channel selection manager
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = ChannelSelectionView::SPtr::create(QString("RTMSAW/%1").arg(sRTMSAWName),
                                                                     this,
                                                                     m_pChannelInfoModel,
                                                                     Qt::Window);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pChannelDataView.data(), &RtFiffRawView::showSelectedChannelsOnly);

        connect(m_pChannelDataView.data(), &RtFiffRawView::channelMarkingChanged,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

        //Init quick control widget
        QStringList slFlags = m_pRTMSA->getDisplayFlags();

        // Quick control scaling
        if(slFlags.contains("scaling")) {
            ScalingView* pScalingView = new ScalingView(QString("RTMSAW/%1").arg(sRTMSAWName),
                                                        m_pFiffInfo->chs);
            m_pQuickControlView->addGroupBox(pScalingView, "Scaling");

            connect(pScalingView, &ScalingView::scalingChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::setScalingMap);

            m_pChannelDataView->setScalingMap(pScalingView->getScaleMap());
        }

        // Quick control projectors
        if(slFlags.contains("projections")) {
            ProjectorsView* pProjectorsView = new ProjectorsView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pProjectorsView, "Noise", "SSP");

            connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::updateProjection);

            pProjectorsView->setProjectors(m_pFiffInfo->projs);
        }

        // Quick control compensators
        if(slFlags.contains("compensators")) {
            CompensatorView* pCompensatorView = new CompensatorView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pCompensatorView, "Noise", "Comp");

            connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::updateCompensator);

            pCompensatorView->setCompensators(m_pFiffInfo->comps);
        }

        // Quick control filter
        if(slFlags.contains("filter")) {
            FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pFilterSettingsView, "Noise", "Filter");

            connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::setFilterChannelType);

            connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::setFilter);

            connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::setFilterActive);

            m_pChannelDataView->setFilterActive(pFilterSettingsView->getFilterActive());
            m_pChannelDataView->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());
            pFilterSettingsView->getFilterView()->setWindowSize(m_iMaxFilterTapSize);
            pFilterSettingsView->getFilterView()->setMaxFilterTaps(m_iMaxFilterTapSize);
            pFilterSettingsView->getFilterView()->init(m_pFiffInfo->sfreq);
        }

        // Quick control SPHARA settings
        if(slFlags.contains("sphara")) {
            SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView();
            m_pQuickControlView->addGroupBoxWithTabs(pSpharaSettingsView, "Noise", "SPHARA");

            connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::updateSpharaActivation);

            connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::updateSpharaOptions);
        }

        // Quick control channel data settings
        if(slFlags.contains("view")) {
            FiffRawViewSettings* pChannelDataSettingsView = new FiffRawViewSettings(QString("RTMSAW/%1").arg(sRTMSAWName));
            pChannelDataSettingsView->setWidgetList();
            m_pQuickControlView->addGroupBoxWithTabs(pChannelDataSettingsView, "Other", "View");

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
        }

        // Quick control trigger detection settings
        if(slFlags.contains("triggerdetection")) {
            TriggerDetectionView* pTriggerDetectionView = new TriggerDetectionView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pTriggerDetectionView, "Other", "Triggers");

            connect(pTriggerDetectionView, &TriggerDetectionView::triggerInfoChanged,
                    m_pChannelDataView.data(), &RtFiffRawView::triggerInfoChanged);

            connect(pTriggerDetectionView, &TriggerDetectionView::resetTriggerCounter,
                    m_pChannelDataView.data(), &RtFiffRawView::resetTriggerCounter);

            connect(m_pChannelDataView.data(), &RtFiffRawView::triggerDetected,
                    pTriggerDetectionView, &TriggerDetectionView::setNumberDetectedTriggersAndTypes);

            pTriggerDetectionView->init(m_pFiffInfo);
        }

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::showSensorSelectionWidget()
{
    if(m_pChannelSelectionView->isActiveWindow()) {
        m_pChannelSelectionView->hide();
    } else {
        m_pChannelSelectionView->activateWindow();
        m_pChannelSelectionView->show();
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::showQuickControlView()
{
    if(m_pQuickControlView->isActiveWindow()) {
        m_pQuickControlView->hide();
    } else {
        m_pQuickControlView->activateWindow();
        m_pQuickControlView->show();
    }
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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


