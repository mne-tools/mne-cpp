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

#include "helpers/quickcontrolwidget.h"

#include <disp/viewers/filterview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/chinfomodel.h>
#include <disp/viewers/channeldataview.h>

#include <scMeas/realtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSvgGenerator>
#include <QToolBox>
#include <QMenu>
#include <QAction>
#include <QHeaderView>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;


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
, m_pToolBox(new QToolBox(this))
{
    Q_UNUSED(pTime)

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Shows the region selection widget (F9)"),this);
    m_pActionSelectSensors->setShortcut(tr("F9"));
    m_pActionSelectSensors->setToolTip(tr("Shows the region selection widget (F9)"));
    connect(m_pActionSelectSensors, &QAction::triggered,
            this, &RealTimeMultiSampleArrayWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(true);

    m_pActionHideBad = new QAction(QIcon(":/images/hideBad.png"), tr("Toggle all bad channels"),this);
    m_pActionHideBad->setStatusTip(tr("Toggle all bad channels"));
    connect(m_pActionHideBad, &QAction::triggered,
            m_pChannelDataView, &ChannelDataView::hideBadChannels);
    addDisplayAction(m_pActionHideBad);
    m_pActionHideBad->setVisible(true);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget"),this);
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget"));
    connect(m_pActionQuickControl, &QAction::triggered,
            this, &RealTimeMultiSampleArrayWidget::showQuickControlWidget);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

    //Create toolboxes with table view and real-time interpolation plot
    m_pToolBox->hide();

    //Add views to toolbox
    m_pToolBox->insertItem(0, m_pChannelDataView, QIcon(), "Channel plot");

    m_pToolBox->setCurrentIndex(0);

    //set layout
    QVBoxLayout *rtmsaLayout = new QVBoxLayout(this);
    rtmsaLayout->addWidget(m_pToolBox.data());
    this->setLayout(rtmsaLayout);

    qRegisterMetaType<QMap<int,QList<QPair<int,double> > > >();

    init();
}


//*************************************************************************************************************

RealTimeMultiSampleArrayWidget::~RealTimeMultiSampleArrayWidget()
{
    //
    // Store Settings
    //
    if(!m_pRTMSA->getName().isEmpty()) {
        QString t_sRTMSAWName = m_pRTMSA->getName();

        QSettings settings;

        //Store scaling
        QMap<qint32, float> qMapChScaling = m_pChannelDataView->getScalingMap();

        if(qMapChScaling.contains(FIFF_UNIT_T))
            settings.setValue(QString("RTMSAW/%1/scaleMAG").arg(t_sRTMSAWName), qMapChScaling[FIFF_UNIT_T]);

        if(qMapChScaling.contains(FIFF_UNIT_T_M))
            settings.setValue(QString("RTMSAW/%1/scaleGRAD").arg(t_sRTMSAWName), qMapChScaling[FIFF_UNIT_T_M]);

        if(qMapChScaling.contains(FIFFV_EEG_CH))
            settings.setValue(QString("RTMSAW/%1/scaleEEG").arg(t_sRTMSAWName), qMapChScaling[FIFFV_EEG_CH]);

        if(qMapChScaling.contains(FIFFV_EOG_CH))
            settings.setValue(QString("RTMSAW/%1/scaleEOG").arg(t_sRTMSAWName), qMapChScaling[FIFFV_EOG_CH]);

        if(qMapChScaling.contains(FIFFV_STIM_CH))
            settings.setValue(QString("RTMSAW/%1/scaleSTIM").arg(t_sRTMSAWName), qMapChScaling[FIFFV_STIM_CH]);

        if(qMapChScaling.contains(FIFFV_MISC_CH))
            settings.setValue(QString("RTMSAW/%1/scaleMISC").arg(t_sRTMSAWName), qMapChScaling[FIFFV_MISC_CH]);

        //Store filter
        if(m_pFilterWindow) {
            FilterData filter = m_pFilterWindow->getUserDesignedFilter();

            settings.setValue(QString("RTMSAW/%1/filterHP").arg(t_sRTMSAWName), filter.m_dHighpassFreq);
            settings.setValue(QString("RTMSAW/%1/filterLP").arg(t_sRTMSAWName), filter.m_dLowpassFreq);
            settings.setValue(QString("RTMSAW/%1/filterOrder").arg(t_sRTMSAWName), filter.m_iFilterOrder);
            settings.setValue(QString("RTMSAW/%1/filterType").arg(t_sRTMSAWName), (int)filter.m_Type);
            settings.setValue(QString("RTMSAW/%1/filterDesignMethod").arg(t_sRTMSAWName), (int)filter.m_designMethod);
            settings.setValue(QString("RTMSAW/%1/filterTransition").arg(t_sRTMSAWName), filter.m_dParksWidth*(filter.m_sFreq/2));
            settings.setValue(QString("RTMSAW/%1/filterUserDesignActive").arg(t_sRTMSAWName), m_pFilterWindow->userDesignedFiltersIsActive());
            settings.setValue(QString("RTMSAW/%1/filterChannelType").arg(t_sRTMSAWName), m_pFilterWindow->getChannelType());
        }

        //Store view
        settings.setValue(QString("RTMSAW/%1/viewZoomFactor").arg(t_sRTMSAWName), m_fZoomFactor);
        settings.setValue(QString("RTMSAW/%1/viewWindowSize").arg(t_sRTMSAWName), m_iT);
        if(m_pQuickControlWidget) {
            settings.setValue(QString("RTMSAW/%1/viewOpacity").arg(t_sRTMSAWName), m_pQuickControlWidget->getOpacityValue());
        }

        //Store show/hide bad channel flag
        settings.setValue(QString("RTMSAW/%1/showHideBad").arg(t_sRTMSAWName), m_bHideBadChannels);

        //Store selected layout file
        if(m_pChannelSelectionView) {
            settings.setValue(QString("RTMSAW/%1/selectedLayoutFile").arg(t_sRTMSAWName), m_pChannelSelectionView->getCurrentLayoutFile());
        }

        //Store show/hide bad channel flag
        if(m_pQuickControlWidget) {
            settings.setValue(QString("RTMSAW/%1/distanceTimeSpacerIndex").arg(t_sRTMSAWName), m_pQuickControlWidget->getDistanceTimeSpacerIndex());
        }

        //Store signal and background colors
        if(m_pQuickControlWidget) {
            settings.setValue(QString("RTMSAW/%1/signalColor").arg(t_sRTMSAWName), m_pQuickControlWidget->getSignalColor());
            settings.setValue(QString("RTMSAW/%1/backgroundColor").arg(t_sRTMSAWName), m_pQuickControlWidget->getBackgroundColor());
        }
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::update(SCMEASLIB::Measurement::SPtr)
{
    if(!m_bInitialized)
    {
        if(m_pRTMSA->isChInit())
        {
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
    if(!m_pFiffInfo->chs.isEmpty()) {
        QSettings settings;
        QString t_sRTMSAWName = m_pRTMSA->getName();

        if(settings.value(QString("RTMSAW/%1/showHideBad").arg(t_sRTMSAWName), false).toBool()) {
            m_pChannelDataView->hideBadChannels();
        }

        //
        //-------- Init signal and background colors --------
        //
        QColor signalDefault = Qt::darkBlue;
        QColor backgroundDefault = Qt::white;
        QColor signal = settings.value(QString("RTMSAW/%1/signalColor").arg(t_sRTMSAWName), signalDefault).value<QColor>();
        QColor background = settings.value(QString("RTMSAW/%1/backgroundColor").arg(t_sRTMSAWName), backgroundDefault).value<QColor>();

        m_pChannelDataView->setBackgroundColorChanged(background);
        m_pChannelDataView->setSignalColor(signal);

        //
        //-------- Init scaling --------
        //
        //Show only spin boxes and labels which type are present in the current loaded fiffinfo
        QList<FiffChInfo> channelList = m_pFiffInfo->chs;
        QList<int> availabeChannelTypes;

        for(int i = 0; i<channelList.size(); i++) {
            int unit = channelList.at(i).unit;
            int type = channelList.at(i).kind;

            if(!availabeChannelTypes.contains(unit))
                availabeChannelTypes.append(unit);

            if(!availabeChannelTypes.contains(type))
                availabeChannelTypes.append(type);
        }

//        if(!t_sRTMSAWName.isEmpty())
//        {
            QMap<qint32,float> qMapChScaling;

            float val = 0.0f;
            if(availabeChannelTypes.contains(FIFF_UNIT_T)) {
                val = settings.value(QString("RTMSAW/%1/scaleMAG").arg(t_sRTMSAWName), 1e-11f).toFloat();
                qMapChScaling.insert(FIFF_UNIT_T, val);
            }

            if(availabeChannelTypes.contains(FIFF_UNIT_T_M)) {
                val = settings.value(QString("RTMSAW/%1/scaleGRAD").arg(t_sRTMSAWName), 1e-10f).toFloat();
                qMapChScaling.insert(FIFF_UNIT_T_M, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EEG_CH)) {
                val = settings.value(QString("RTMSAW/%1/scaleEEG").arg(t_sRTMSAWName), 1e-4f).toFloat();
                qMapChScaling.insert(FIFFV_EEG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EOG_CH)) {
                val = settings.value(QString("RTMSAW/%1/scaleEOG").arg(t_sRTMSAWName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_EOG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_STIM_CH)) {
                val = settings.value(QString("RTMSAW/%1/scaleSTIM").arg(t_sRTMSAWName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_STIM_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_MISC_CH)) {
                val = settings.value(QString("RTMSAW/%1/scaleMISC").arg(t_sRTMSAWName), 1e-3f).toFloat();
                qMapChScaling.insert(FIFFV_MISC_CH, val);
            }

            m_pChannelDataView->setScaling(qMapChScaling);
//        }

        //-------- Init filter window --------
        m_pFilterWindow = FilterView::SPtr::create(this, Qt::Window);

        m_pFilterWindow->init(m_pFiffInfo->sfreq);
        m_pFilterWindow->setWindowSize(m_iMaxFilterTapSize);
        m_pFilterWindow->setMaxFilterTaps(m_iMaxFilterTapSize);

        connect(m_pFilterWindow.data(),static_cast<void (FilterView::*)(QString)>(&FilterView::applyFilter),
                m_pRTMSAModel.data(),static_cast<void (ChannelDataModel::*)(QString)>(&ChannelDataModel::setFilterChannelType));

        connect(m_pFilterWindow.data(), &FilterView::filterChanged,
                m_pRTMSAModel.data(), &ChannelDataModel::filterChanged);

        connect(m_pFilterWindow.data(), &FilterView::filterActivated,
                m_pRTMSAModel.data(), &ChannelDataModel::filterActivated);

        //Set stored filter settings from last session
        m_pFilterWindow->setFilterParameters(settings.value(QString("RTMSAW/%1/filterHP").arg(t_sRTMSAWName), 5.0).toDouble(),
                                                settings.value(QString("RTMSAW/%1/filterLP").arg(t_sRTMSAWName), 40.0).toDouble(),
                                                settings.value(QString("RTMSAW/%1/filterOrder").arg(t_sRTMSAWName), 128).toInt(),
                                                settings.value(QString("RTMSAW/%1/filterType").arg(t_sRTMSAWName), 2).toInt(),
                                                settings.value(QString("RTMSAW/%1/filterDesignMethod").arg(t_sRTMSAWName), 0).toInt(),
                                                settings.value(QString("RTMSAW/%1/filterTransition").arg(t_sRTMSAWName), 5.0).toDouble(),
                                                settings.value(QString("RTMSAW/%1/filterUserDesignActive").arg(t_sRTMSAWName), false).toBool(),
                                                settings.value(QString("RTMSAW/%1/filterChannelType").arg(t_sRTMSAWName), "MEG").toString());

        //
        //-------- Init channel selection manager --------
        //
        m_pChInfoModel = QSharedPointer<ChInfoModel>(new ChInfoModel(m_pFiffInfo, this));

        m_pChannelSelectionView = ChannelSelectionView::SPtr(new ChannelSelectionView(this, m_pChInfoModel, Qt::Window));

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pChannelDataView, &ChannelDataView::showSelectedChannelsOnly);

        //Connect channel info model
        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChInfoModel.data(), &ChInfoModel::layoutChanged);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        connect(m_pChannelDataView.data(), &ChannelDataView::channelMarkingChanged,
                m_pChannelSelectionView.data(), &ChannelSelectionView::updateBadChannels);

        connect(m_pChInfoModel.data(), &ChInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        m_pChInfoModel->fiffInfoChanged(m_pFiffInfo);

        m_pChannelSelectionView->setCurrentLayoutFile(settings.value(QString("RTMSAW/%1/selectedLayoutFile").arg(t_sRTMSAWName),
                                                                       "babymeg-mag-inner-layer.lout").toString());

        //
        //-------- Init quick control widget --------
        //
        QStringList slFlags = m_pRTMSA->getDisplayFlags();

        #ifdef BUILD_BASIC_MNESCAN_VERSION
            std::cout<<"BUILD_BASIC_MNESCAN_VERSION Defined"<<std::endl;
            slFlags.clear();
            slFlags << "projections" << "view" << "scaling";
        #endif

        m_pQuickControlWidget = QSharedPointer<QuickControlWidget>(new QuickControlWidget(qMapChScaling, m_pFiffInfo, "RT Display", slFlags, this));

        //Handle scaling
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::scalingChanged,
                m_pChannelDataView, &ChannelDataView::setScalingMap);

        //Handle signal color changes
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::signalColorChanged,
                m_pChannelDataView, &ChannelDataView::setSignalColor);

        //Handle background color changes
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::backgroundColorChanged,
                m_pChannelDataView, &ChannelDataView::setBackgroundColorChanged);

        //Handle screenshot signals
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::makeScreenshot,
                this, &RealTimeMultiSampleArrayWidget::onMakeScreenshot);

        //Handle projections
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::projSelectionChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::updateProjection);

        //Handle compensators
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::compSelectionChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::updateCompensator);

        //Handle SPHARA
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::spharaActivationChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::updateSpharaActivation);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::spharaOptionsChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::updateSpharaOptions);

        //Handle view changes
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::zoomChanged,
                m_pChannelDataView, &ChannelDataView::zoomChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::timeWindowChanged,
                m_pChannelDataView, &ChannelDataView::timeWindowChanged);

        //Handle Filtering
        connect(m_pFilterWindow.data(), &FilterView::activationCheckBoxListChanged,
                m_pQuickControlWidget.data(), &QuickControlWidget::filterGroupChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::showFilterOptions,
                this, &RealTimeMultiSampleArrayWidget::showFilterWidget);

        //Handle trigger detection
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::triggerInfoChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::triggerInfoChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::resetTriggerCounter,
                this->m_pRTMSAModel.data(), &ChannelDataModel::resetTriggerCounter);

        connect(this->m_pRTMSAModel.data(), &ChannelDataModel::triggerDetected,
                m_pQuickControlWidget.data(), &QuickControlWidget::setNumberDetectedTriggersAndTypes);

        //Handle time spacer distance
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::distanceTimeSpacerChanged,
                this->m_pRTMSAModel.data(), &ChannelDataModel::distanceTimeSpacerChanged);

        m_pQuickControlWidget->filterGroupChanged(m_pFilterWindow->getActivationCheckBoxList());

        m_pQuickControlWidget->setViewParameters(settings.value(QString("RTMSAW/%1/viewZoomFactor").arg(t_sRTMSAWName), 1.0).toFloat(),
                                                 settings.value(QString("RTMSAW/%1/viewWindowSize").arg(t_sRTMSAWName), 10).toInt(),
                                                 settings.value(QString("RTMSAW/%1/viewOpacity").arg(t_sRTMSAWName), 95).toInt());

        m_pQuickControlWidget->setDistanceTimeSpacerIndex(settings.value(QString("RTMSAW/%1/distanceTimeSpacerIndex").arg(t_sRTMSAWName), 3).toInt());

        m_pQuickControlWidget->setSignalBackgroundColors(signal, background);

        //If projections are wanted activate projections as default
        if(slFlags.contains("projections")) {
            m_pRTMSAModel->updateProjection();
        }

        m_pToolBox->show();

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::showFilterWidget(bool state)
{
    if(m_pFilterWindow) {
        if(state) {
            if(m_pFilterWindow->isActiveWindow())
                m_pFilterWindow->hide();
            else {
                m_pFilterWindow->activateWindow();
                m_pFilterWindow->show();
            }
        } else {
            m_pFilterWindow->hide();
        }
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

void RealTimeMultiSampleArrayWidget::showQuickControlWidget()
{
    m_pQuickControlWidget->show();
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

    if(imageType.contains("SVG"))
    {
        QString fileName = QString("./Screenshots/%1-%2-DataView.svg").arg(sDate).arg(sTime);
        m_pChannelDataView->takeScreenshot(fileName);
    }

    if(imageType.contains("PNG"))
    {
        QString fileName = QString("./Screenshots/%1-%2-DataView.png").arg(sDate).arg(sTime);
        m_pChannelDataView->takeScreenshot(fileName);
    }
}
