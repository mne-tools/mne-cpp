//=============================================================================================================
/**
* @file     realtimemultisamplearraywidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
#include <disp/viewers/filterview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/channeldataview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/spharasettingsview.h>
#include <disp/viewers/channeldatasettingsview.h>
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
, m_pChannelDataView(new ChannelDataView(this))
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
    m_pChannelDataView->hide();

    // Quick control selection
    QSettings settings;

    m_pQuickControlView = QuickControlView::SPtr::create("RT Display", Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint, this);
    m_pQuickControlView->setOpacityValue(settings.value(QString("RTMSAW/%1/viewOpacity").arg(m_pRTMSA->getName()), 95).toInt());

    QList<QSharedPointer<QWidget> > lControlWidgets = m_pRTMSA->getControlWidgets();

    for(int i = 0; i < lControlWidgets.size(); ++i) {
        QString sObjectName = lControlWidgets.at(i)->objectName();

        if(sObjectName.contains("widget_", Qt::CaseInsensitive)) {
            m_pQuickControlView->addWidget(lControlWidgets.at(i));
        }

        if(sObjectName.contains("group_", Qt::CaseInsensitive)) {
            if(sObjectName.contains("group_tab_", Qt::CaseInsensitive)) {
                sObjectName.remove("group_tab_");
                QStringList sList = sObjectName.split("_");
                if(sList.size() >= 2) {
                   m_pQuickControlView->addGroupBoxWithTabs(lControlWidgets.at(i), sList.at(0), sList.at(1));
                } else {
                    m_pQuickControlView->addGroupBoxWithTabs(lControlWidgets.at(i), "", sObjectName);
                }
            } else {
                sObjectName.remove("group_");
                m_pQuickControlView->addGroupBox(lControlWidgets.at(i), sObjectName);
            }
        }
    }

    QVBoxLayout *rtmsaLayout = new QVBoxLayout(this);
    rtmsaLayout->setContentsMargins(0,0,0,0);
    rtmsaLayout->addWidget(m_pChannelDataView);
    this->setLayout(rtmsaLayout);

    qRegisterMetaType<QMap<int,QList<QPair<int,double> > > >();
}


//*************************************************************************************************************

RealTimeMultiSampleArrayWidget::~RealTimeMultiSampleArrayWidget()
{
    // Store Settings
    if(!m_pRTMSA->getName().isEmpty()) {
        QString sRTMSAWName = m_pRTMSA->getName();

        QSettings settings;

        //Store view
        if(m_pChannelDataView) {
            //Zoom and window size
            settings.setValue(QString("RTMSAW/%1/viewZoomFactor").arg(sRTMSAWName), m_pChannelDataView->getZoom());
            settings.setValue(QString("RTMSAW/%1/viewWindowSize").arg(sRTMSAWName), m_pChannelDataView->getWindowSize());

            //Store show/hide bad channel flag
            settings.setValue(QString("RTMSAW/%1/showHideBad").arg(sRTMSAWName), m_pChannelDataView->getBadChannelHideStatus());

            settings.setValue(QString("RTMSAW/%1/signalColor").arg(sRTMSAWName), m_pChannelDataView->getSignalColor());

            settings.setValue(QString("RTMSAW/%1/backgroundColor").arg(sRTMSAWName), m_pChannelDataView->getBackgroundColor());

            settings.setValue(QString("RTMSAW/%1/distanceTimeSpacer").arg(sRTMSAWName), m_pChannelDataView->getDistanceTimeSpacer());
        }

        //Store QuickControlView
        if(m_pQuickControlView) {
            settings.setValue(QString("RTMSAW/%1/viewOpacity").arg(sRTMSAWName), m_pQuickControlView->getOpacityValue());
        }
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
        QColor signalDefault = Qt::darkBlue;
        QColor backgroundDefault = Qt::white;
        QColor signal = settings.value(QString("RTMSAW/%1/signalColor").arg(sRTMSAWName), signalDefault).value<QColor>();
        QColor background = settings.value(QString("RTMSAW/%1/backgroundColor").arg(sRTMSAWName), backgroundDefault).value<QColor>();

        m_pChannelDataView->show();
        m_pChannelDataView->init(m_pFiffInfo);
        m_pChannelDataView->setBackgroundColor(background);
        m_pChannelDataView->setSignalColor(signal);

        if(settings.value(QString("RTMSAW/%1/showHideBad").arg(sRTMSAWName), false).toBool()) {
            this->onHideBadChannels();
        }

        //Init channel selection manager
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = ChannelSelectionView::SPtr::create(QString("RTMSAW/%1").arg(sRTMSAWName),
                                                                     this,
                                                                     m_pChannelInfoModel, Qt::Window);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pChannelDataView.data(), &ChannelDataView::showSelectedChannelsOnly);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        connect(m_pChannelDataView.data(), &ChannelDataView::channelMarkingChanged,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        m_pChannelInfoModel->fiffInfoChanged(m_pFiffInfo);

        //Init quick control widget
        QStringList slFlags = m_pRTMSA->getDisplayFlags();
        #ifdef BUILD_BASIC_MNESCAN_VERSION
            std::cout<<"BUILD_BASIC_MNESCAN_VERSION Defined"<<std::endl;
            slFlags.clear();
            slFlags << "projections" << "view" << "scaling";
        #endif

        // Quick control scaling
        if(slFlags.contains("scaling")) {
            ScalingView* pScalingView = new ScalingView(QString("RTMSAW/%1").arg(sRTMSAWName),
                                                        m_pFiffInfo->chs);
            m_pQuickControlView->addGroupBox(pScalingView, "Scaling");

            connect(pScalingView, &ScalingView::scalingChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setScalingMap);

            m_pChannelDataView->setScalingMap(pScalingView->getScaleMap());
        }

        // Quick control projectors
        if(slFlags.contains("projections")) {
            ProjectorsView* pProjectorsView = new ProjectorsView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pProjectorsView, "Noise", "SSP");

            connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                    m_pChannelDataView.data(), &ChannelDataView::updateProjection);

            pProjectorsView->setProjectors(m_pFiffInfo->projs);
        }

        // Quick control compensators
        if(slFlags.contains("compensators")) {
            CompensatorView* pCompensatorView = new CompensatorView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pCompensatorView, "Noise", "Comp");

            connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                    m_pChannelDataView.data(), &ChannelDataView::updateCompensator);

            pCompensatorView->setCompensators(m_pFiffInfo->comps);
        }

        // Quick control filter
        if(slFlags.contains("filter")) {
            FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pFilterSettingsView, "Noise", "Filter");

            connect(pFilterSettingsView->getFilterView().data(), &FilterView::filterChannelTypeChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setFilterChannelType);

            connect(pFilterSettingsView->getFilterView().data(), &FilterView::filterChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setFilter);

            connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setFilterActive);

            m_pChannelDataView->setFilterActive(pFilterSettingsView->getFilterActive());
            m_pChannelDataView->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());
            pFilterSettingsView->getFilterView()->init(m_pFiffInfo->sfreq);
            pFilterSettingsView->getFilterView()->setWindowSize(m_iMaxFilterTapSize);
            pFilterSettingsView->getFilterView()->setMaxFilterTaps(m_iMaxFilterTapSize);
        }

        // Quick control SPHARA settings
        if(slFlags.contains("sphara")) {
            SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView();
            m_pQuickControlView->addGroupBoxWithTabs(pSpharaSettingsView, "Noise", "SPHARA");

            connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
                    m_pChannelDataView.data(), &ChannelDataView::updateSpharaActivation);

            connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
                    m_pChannelDataView.data(), &ChannelDataView::updateSpharaOptions);
        }

        // Quick control channel data settings
        if(slFlags.contains("view")) {
            ChannelDataSettingsView* pChannelDataSettingsView = new ChannelDataSettingsView();
            pChannelDataSettingsView->init();
            m_pQuickControlView->addGroupBoxWithTabs(pChannelDataSettingsView, "Other", "View");

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::signalColorChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setSignalColor);

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::backgroundColorChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setBackgroundColor);

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::zoomChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setZoom);

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::timeWindowChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setWindowSize);

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::distanceTimeSpacerChanged,
                    m_pChannelDataView.data(), &ChannelDataView::setDistanceTimeSpacer);

            connect(pChannelDataSettingsView, &ChannelDataSettingsView::makeScreenshot,
                    this, &RealTimeMultiSampleArrayWidget::onMakeScreenshot);

            pChannelDataSettingsView->setViewParameters(settings.value(QString("RTMSAW/%1/viewZoomFactor").arg(sRTMSAWName), 1.0).toFloat(),
                                                        settings.value(QString("RTMSAW/%1/viewWindowSize").arg(sRTMSAWName), 10).toInt());
            pChannelDataSettingsView->setDistanceTimeSpacer(settings.value(QString("RTMSAW/%1/distanceTimeSpacer").arg(sRTMSAWName), 100).toInt());
            pChannelDataSettingsView->setSignalBackgroundColors(signal, background);
        }

        // Quick control trigger detection settings
        if(slFlags.contains("triggerdetection")) {
            TriggerDetectionView* pTriggerDetectionView = new TriggerDetectionView(QString("RTMSAW/%1").arg(sRTMSAWName));
            m_pQuickControlView->addGroupBoxWithTabs(pTriggerDetectionView, "Other", "Triggers");

            connect(pTriggerDetectionView, &TriggerDetectionView::triggerInfoChanged,
                    m_pChannelDataView.data(), &ChannelDataView::triggerInfoChanged);

            connect(pTriggerDetectionView, &TriggerDetectionView::resetTriggerCounter,
                    m_pChannelDataView.data(), &ChannelDataView::resetTriggerCounter);

            connect(m_pChannelDataView.data(), &ChannelDataView::triggerDetected,
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


