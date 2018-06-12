//=============================================================================================================
/**
* @file     realtimeevokedsetwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the RealTimeEvokedSetWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedsetwidget.h"
//#include "annotationwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintEvent>
#include <QPainter>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScroller>
#include <QSettings>
#include <QSvgGenerator>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;


//=============================================================================================================
/**
* Tool enumeration.
*/
enum Tool
{
    Freeze     = 0,     /**< Freezing tool. */
    Annotation = 1      /**< Annotation tool. */
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSetWidget::RealTimeEvokedSetWidget(QSharedPointer<RealTimeEvokedSet> pRTESet, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTESetModel(Q_NULLPTR)
, m_pButterflyPlot(Q_NULLPTR)
, m_pAverageScene(Q_NULLPTR)
, m_pRTESet(pRTESet)
, m_pQuickControlWidget(Q_NULLPTR)
, m_pSelectionManagerWindow(Q_NULLPTR)
, m_pChInfoModel(Q_NULLPTR)
, m_pFilterWindow(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)
    //qRegisterMetaType<SCDISPLIB::AverageInfoMap>("SCDISPLIB::AverageInfoMap");
    qRegisterMetaTypeStreamOperators<SCDISPLIB::AverageInfoMap>("SCDISPLIB::AverageInfoMap");

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the region selection widget (F11)"),this);
    m_pActionSelectSensors->setShortcut(tr("F11"));
    m_pActionSelectSensors->setStatusTip(tr("Show the region selection widget (F11)"));
    connect(m_pActionSelectSensors, &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(false);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl, &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showQuickControlWidget);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(false);

    //set vertical layout
    m_pRTESetLayout = new QVBoxLayout(this);

    //Acquire label
    m_pLabelInit= new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;font.setBold(true);font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRTESetLayout->addWidget(m_pLabelInit);

    //Create toolboxes with butterfly and 2D layout plot
    m_pToolBox = new QToolBox(this);
    m_pToolBox->hide();

    //Butterfly
    m_pButterflyPlot = RealTimeButterflyPlot::SPtr(new RealTimeButterflyPlot(this));
    m_pButterflyPlot->installEventFilter(this);

    m_pToolBox->insertItem(0, m_pButterflyPlot.data(), QIcon(), "Butterfly plot");

    //2D layout plot
    m_pAverageLayoutView = new QGraphicsView(this);

    //m_pAverageLayoutView->installEventFilter(this);
    m_pToolBox->insertItem(0, m_pAverageLayoutView, QIcon(), "2D Layout plot");

    m_pRTESetLayout->addWidget(m_pToolBox);

    //set layouts
    this->setLayout(m_pRTESetLayout);

    getData();
}


//*************************************************************************************************************

RealTimeEvokedSetWidget::~RealTimeEvokedSetWidget()
{
    //
    // Store Settings
    //
    if(!m_pRTESet->getName().isEmpty())
    {
        QString t_sRTESWName = m_pRTESet->getName();

        QSettings settings;

        //Store modalities
        for(qint32 i = 0; i < m_qListModalities.size(); ++i) {
            settings.setValue(QString("RTESW/%1/%2/active").arg(t_sRTESWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_bActive);
            settings.setValue(QString("RTESW/%1/%2/norm").arg(t_sRTESWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_fNorm);
        }

        //Store filter
        if(m_pFilterWindow != 0) {
            FilterData filter = m_pFilterWindow->getUserDesignedFilter();

            settings.setValue(QString("RTESW/%1/filterHP").arg(t_sRTESWName), filter.m_dHighpassFreq);
            settings.setValue(QString("RTESW/%1/filterLP").arg(t_sRTESWName), filter.m_dLowpassFreq);
            settings.setValue(QString("RTESW/%1/filterOrder").arg(t_sRTESWName), filter.m_iFilterOrder);
            settings.setValue(QString("RTESW/%1/filterType").arg(t_sRTESWName), (int)filter.m_Type);
            settings.setValue(QString("RTESW/%1/filterDesignMethod").arg(t_sRTESWName), (int)filter.m_designMethod);
            settings.setValue(QString("RTESW/%1/filterTransition").arg(t_sRTESWName), filter.m_dParksWidth*(filter.m_sFreq/2));
            settings.setValue(QString("RTESW/%1/filterUserDesignActive").arg(t_sRTESWName), m_pFilterWindow->userDesignedFiltersIsActive());
            settings.setValue(QString("RTESW/%1/filterChannelType").arg(t_sRTESWName), m_pFilterWindow->getChannelType());
        }

        //Store scaling
        if(m_qMapChScaling.contains(FIFF_UNIT_T)) {
            settings.setValue(QString("RTESW/%1/scaleMAG").arg(t_sRTESWName), m_qMapChScaling[FIFF_UNIT_T]);
            qDebug()<<"m_qMapChScaling[FIFF_UNIT_T]: "<<m_qMapChScaling[FIFF_UNIT_T];
        }

        if(m_qMapChScaling.contains(FIFF_UNIT_T_M))
            settings.setValue(QString("RTESW/%1/scaleGRAD").arg(t_sRTESWName), m_qMapChScaling[FIFF_UNIT_T_M]);

        if(m_qMapChScaling.contains(FIFFV_EEG_CH))
            settings.setValue(QString("RTESW/%1/scaleEEG").arg(t_sRTESWName), m_qMapChScaling[FIFFV_EEG_CH]);

        if(m_qMapChScaling.contains(FIFFV_EOG_CH))
            settings.setValue(QString("RTESW/%1/scaleEOG").arg(t_sRTESWName), m_qMapChScaling[FIFFV_EOG_CH]);

        if(m_qMapChScaling.contains(FIFFV_STIM_CH))
            settings.setValue(QString("RTESW/%1/scaleSTIM").arg(t_sRTESWName), m_qMapChScaling[FIFFV_STIM_CH]);

        if(m_qMapChScaling.contains(FIFFV_MISC_CH))
            settings.setValue(QString("RTESW/%1/scaleMISC").arg(t_sRTESWName), m_qMapChScaling[FIFFV_MISC_CH]);

        //Store selected layout file
        if(!m_pSelectionManagerWindow == 0) {
            settings.setValue(QString("RTESW/%1/selectedLayoutFile").arg(t_sRTESWName), m_pSelectionManagerWindow->getCurrentLayoutFile());
        }

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("RTESW/%1/selectedView").arg(t_sRTESWName), m_pToolBox->currentIndex());
        }

        //Store average colors per type
        if(m_pRTESetModel) {
            QVariant data;
            SCDISPLIB::AverageInfoMap avrMap = m_pQuickControlWidget->getAverageInformationMap();
            data.setValue(avrMap);
            settings.setValue(QString("RTESW/%1/averageInfoMap").arg(t_sRTESWName), data);
        }

        //Store signal and background colors
        if(m_pQuickControlWidget != 0) {
            settings.setValue(QString("RTESW/%1/signalColor").arg(t_sRTESWName), m_pQuickControlWidget->getSignalColor());
            settings.setValue(QString("RTESW/%1/butterflyBackgroundColor").arg(t_sRTESWName), m_pButterflyPlot->getBackgroundColor());
            settings.setValue(QString("RTESW/%1/layoutBackgroundColor").arg(t_sRTESWName), m_pAverageScene->backgroundBrush().color());
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::update(SCMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::getData()
{
    if(!m_bInitialized) {
        if(m_pRTESet->isInitialized()) {
            m_qListChInfo = m_pRTESet->chInfo();
            m_pFiffInfo = m_pRTESet->info();

            if(!m_pRTESet->getValue()->evoked.isEmpty()) {
                m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();
            }

            init();

            m_pRTESetModel->updateData();
        }
    }
    else {
        //Check if block size has changed, if yes update the filter
        if(!m_pRTESet->getValue()->evoked.isEmpty()) {
            if(m_iMaxFilterTapSize != m_pRTESet->getValue()->evoked.first().data.cols()) {
                m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();

                m_pFilterWindow->setWindowSize(m_iMaxFilterTapSize);
                m_pFilterWindow->setMaxFilterTaps(m_iMaxFilterTapSize);
            }
        }

        m_pRTESetModel->updateData();
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::init()
{
    if(m_qListChInfo.size() > 0)
    {
        qDebug()<<"RealTimeEvokedSetWidget::init() - "<<m_pRTESet->getName();
        QSettings settings;
        QString t_sRTESWName = m_pRTESet->getName();
        m_pRTESetLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pToolBox->show();

        m_pRTESetModel = RealTimeEvokedSetModel::SPtr(new RealTimeEvokedSetModel(this));
        m_pRTESetModel->setRTESet(m_pRTESet);

        m_pButterflyPlot->setModel(m_pRTESetModel.data());

        //Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("RTESW/%1/selectedView").arg(t_sRTESWName), 0).toInt());

        //
        //-------- Init modalities --------
        //
        m_qListModalities.clear();
        bool hasMag = false;
        bool hasGrad = false;
        bool hasEEG = false;
        bool hasEOG = false;
        bool hasMISC = false;
        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        {
            if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
            {
                if(!hasMag && m_pFiffInfo->chs[i].unit == FIFF_UNIT_T)
                    hasMag = true;
                else if(!hasGrad &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M)
                    hasGrad = true;
            }
            else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH)
                hasEEG = true;
            else if(!hasEOG && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH)
                hasEOG = true;
            else if(!hasMISC && m_pFiffInfo->chs[i].kind == FIFFV_MISC_CH)
                hasMISC = true;
        }
        bool sel = true;
        float val = 1e-11f;
        if(hasMag) {
            sel = settings.value(QString("RTESW/%1/MAG/active").arg(t_sRTESWName), true).toBool();
            val = settings.value(QString("RTESW/%1/MAG/norm").arg(t_sRTESWName), 1e-11f).toFloat();
            m_qListModalities.append(Modality("MAG",sel,val));
        }
        if(hasGrad) {
            sel = settings.value(QString("RTESW/%1/GRAD/active").arg(t_sRTESWName), true).toBool();
            val = settings.value(QString("RTESW/%1/GRAD/norm").arg(t_sRTESWName), 1e-10f).toFloat();
            m_qListModalities.append(Modality("GRAD",sel,val));
        }
        if(hasEEG) {
            sel = settings.value(QString("RTESW/%1/EEG/active").arg(t_sRTESWName), true).toBool();
            val = settings.value(QString("RTESW/%1/EEG/norm").arg(t_sRTESWName), 1e-4f).toFloat();
            m_qListModalities.append(Modality("EEG",sel,val));
        }
        if(hasEOG) {
            sel = settings.value(QString("RTESW/%1/EOG/active").arg(t_sRTESWName), true).toBool();
            val = settings.value(QString("RTESW/%1/EOG/norm").arg(t_sRTESWName), 1e-3f).toFloat();
            m_qListModalities.append(Modality("EOG",sel,val));
        }
        if(hasMISC) {
            sel = settings.value(QString("RTESW/%1/MISC/active").arg(t_sRTESWName), true).toBool();
            val = settings.value(QString("RTESW/%1/MISC/norm").arg(t_sRTESWName), 1e-3f).toFloat();
            m_qListModalities.append(Modality("MISC",sel,val));
        }

        m_pButterflyPlot->setSettings(m_qListModalities);

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

        QString t_sRTEName = m_pRTESet->getName();

        if(!t_sRTEName.isEmpty())
        {
            qDebug()<<"Init scaling";
            m_qMapChScaling.clear();

            QSettings settings;
            float val = 0.0f;
            if(availabeChannelTypes.contains(FIFF_UNIT_T)) {
                val = settings.value(QString("RTESW/%1/scaleMAG").arg(t_sRTESWName), 1e-11f).toFloat();
                m_qMapChScaling.insert(FIFF_UNIT_T, val);
            }

            if(availabeChannelTypes.contains(FIFF_UNIT_T_M)) {
                val = settings.value(QString("RTESW/%1/scaleGRAD").arg(t_sRTESWName), 1e-10f).toFloat();
                m_qMapChScaling.insert(FIFF_UNIT_T_M, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EEG_CH)) {
                val = settings.value(QString("RTESW/%1/scaleEEG").arg(t_sRTESWName), 1e-4f).toFloat();
                m_qMapChScaling.insert(FIFFV_EEG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EOG_CH)) {
                val = settings.value(QString("RTESW/%1/scaleEOG").arg(t_sRTESWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_EOG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_STIM_CH)) {
                val = settings.value(QString("RTESW/%1/scaleSTIM").arg(t_sRTESWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_STIM_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_MISC_CH)) {
                val = settings.value(QString("RTESW/%1/scaleMISC").arg(t_sRTESWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_MISC_CH, val);
            }

            m_pRTESetModel->setScaling(m_qMapChScaling);
        }

        //
        //-------- Init filter window --------
        //
        m_pFilterWindow = FilterWindow::SPtr(new FilterWindow(this, Qt::Window));
        //m_pFilterWindow->setWindowFlags(Qt::WindowStaysOnTopHint);

        m_pFilterWindow->init(m_pFiffInfo->sfreq);
        m_pFilterWindow->setWindowSize(m_iMaxFilterTapSize);
        m_pFilterWindow->setMaxFilterTaps(m_iMaxFilterTapSize);

        connect(m_pFilterWindow.data(), static_cast<void (FilterWindow::*)(QString)>(&FilterWindow::applyFilter),
                m_pRTESetModel.data(),static_cast<void (RealTimeEvokedSetModel::*)(QString)>(&RealTimeEvokedSetModel::setFilterChannelType));

        connect(m_pFilterWindow.data(), &FilterWindow::filterChanged,
                m_pRTESetModel.data(), &RealTimeEvokedSetModel::filterChanged);

        //Init downsampled sampling frequency
        m_pFilterWindow->setSamplingRate(m_pFiffInfo->sfreq);

        //Set stored filter settings from last session
        m_pFilterWindow->setFilterParameters(settings.value(QString("RTESW/%1/filterHP").arg(t_sRTESWName), 5.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterLP").arg(t_sRTESWName), 40.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterOrder").arg(t_sRTESWName), 128).toInt(),
                                                settings.value(QString("RTESW/%1/filterType").arg(t_sRTESWName), 2).toInt(),
                                                settings.value(QString("RTESW/%1/filterDesignMethod").arg(t_sRTESWName), 0).toInt(),
                                                settings.value(QString("RTESW/%1/filterTransition").arg(t_sRTESWName), 5.0).toDouble(),
                                                settings.value(QString("RTESW/%1/filterUserDesignActive").arg(t_sRTESWName), false).toBool(),
                                                settings.value(QString("RTESW/%1/filterChannelType").arg(t_sRTESWName), "MEG").toString());

        //
        //-------- Init channel selection manager --------
        //
        m_pChInfoModel = QSharedPointer<ChInfoModel>(new ChInfoModel(m_pFiffInfo, this));
        m_pSelectionManagerWindow = QSharedPointer<SelectionManagerWindow>(new SelectionManagerWindow(this, m_pChInfoModel));

        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::showSelectedChannelsOnly,
                this, &RealTimeEvokedSetWidget::showSelectedChannelsOnly);

        //Connect channel info model
        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::loadedLayoutMap,
                m_pChInfoModel.data(), &ChInfoModel::layoutChanged);

        connect(m_pChInfoModel.data(), &ChInfoModel::channelsMappedToLayout,
                m_pSelectionManagerWindow.data(), &SelectionManagerWindow::setCurrentlyMappedFiffChannels);

        m_pChInfoModel->fiffInfoChanged(m_pFiffInfo);

        m_pSelectionManagerWindow->setCurrentLayoutFile(settings.value(QString("RTESW/%1/selectedLayoutFile").arg(t_sRTESWName), "babymeg-mag-inner-layer.lout").toString());

        m_pActionSelectSensors->setVisible(true);

        //
        //-------- Init quick control widget --------
        //
        QStringList slFlags;
        slFlags <<  "projections" << "compensators" << "filter" << "scaling" << "modalities" << "colors" << "averages";

        m_pQuickControlWidget = QSharedPointer<QuickControlWidget>(new QuickControlWidget(m_qMapChScaling, m_pFiffInfo, "RT Averaging", slFlags));

        //m_pQuickControlWidget = QuickControlWidget::SPtr(new QuickControlWidget(m_qMapChScaling, m_pFiffInfo, "RT Averaging", 0, true, true, false, true, true, false));
        m_pQuickControlWidget->setWindowFlags(Qt::WindowStaysOnTopHint);

        //Handle scaling
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::scalingChanged,
                this, &RealTimeEvokedSetWidget::broadcastScaling);

        //Handle background color changes
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::backgroundColorChanged,
                this, &RealTimeEvokedSetWidget::onTableViewBackgroundColorChanged);

        //Handle screenshot signals
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::makeScreenshot,
                this, &RealTimeEvokedSetWidget::onMakeScreenshot);

        //Handle compensators
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::compSelectionChanged,
                m_pRTESetModel.data(), &RealTimeEvokedSetModel::updateCompensator);

        //Handle projections
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::projSelectionChanged,
                m_pRTESetModel.data(), &RealTimeEvokedSetModel::updateProjection);

        //Handle modalities
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::settingsChanged,
                this, &RealTimeEvokedSetWidget::broadcastSettings);

        //Handle filtering
        connect(m_pFilterWindow.data(), &FilterWindow::activationCheckBoxListChanged,
                m_pQuickControlWidget.data(), &QuickControlWidget::filterGroupChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::showFilterOptions,
                this, &RealTimeEvokedSetWidget::showFilterWidget);

        //Handle updating the butterfly and layout plot
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::updateConnectedView,
                m_pButterflyPlot.data(), &RealTimeButterflyPlot::updateView);
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::updateConnectedView,
                this, &RealTimeEvokedSetWidget::onSelectionChanged);

        m_pQuickControlWidget->setViewParameters(settings.value(QString("RTESW/%1/viewZoomFactor").arg(t_sRTESWName), 1.0).toFloat(),
                                                     settings.value(QString("RTESW/%1/viewWindowSize").arg(t_sRTESWName), 10).toInt(),
                                                     settings.value(QString("RTESW/%1/viewOpacity").arg(t_sRTESWName), 95).toInt());

        m_pQuickControlWidget->filterGroupChanged(m_pFilterWindow->getActivationCheckBoxList());

        QColor signalDefault = Qt::darkBlue;
        QColor butterflyBackgroundDefault = Qt::white;
        QColor layoutBackgroundDefault = Qt::black;
        m_pQuickControlWidget->setSignalBackgroundColors(settings.value(QString("RTESW/%1/signalColor").arg(t_sRTESWName), signalDefault).value<QColor>(), settings.value(QString("RTESW/%1/butterflyBackgroundColor").arg(t_sRTESWName), butterflyBackgroundDefault).value<QColor>());

        m_pActionQuickControl->setVisible(true);

        //Activate projections as default
        m_pRTESetModel->updateProjection();

        //
        //-------- Init average scene --------
        //
        m_pAverageScene = AverageScene::SPtr(new AverageScene(m_pAverageLayoutView, this));
        m_pAverageLayoutView->setScene(m_pAverageScene.data());
        QBrush brush(Qt::black);
        m_pAverageScene->setBackgroundBrush(brush);

        //Connect selection manager with average manager
        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::selectionChanged,
                this, &RealTimeEvokedSetWidget::channelSelectionManagerChanged);

        connect(m_pRTESetModel.data(), &RealTimeEvokedSetModel::dataChanged,
                this, &RealTimeEvokedSetWidget::onSelectionChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::scalingChanged,
                this, &RealTimeEvokedSetWidget::broadcastScaling);

        //Handle averages
        connect(this->m_pRTESetModel.data(), &RealTimeEvokedSetModel::newAverageTypeReceived,
                m_pQuickControlWidget.data(), &QuickControlWidget::setAverageInformationMap);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::averageInformationChanged,
                m_pAverageScene.data(), &AverageScene::setAverageInformationMap);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::averageInformationChanged,
                m_pButterflyPlot.data(), &RealTimeButterflyPlot::setAverageInformationMap);

        QVariant data;
        QMap<double, QPair<QColor, QPair<QString,bool> > > emptyMap;
        data.setValue(emptyMap);
        SCDISPLIB::AverageInfoMap map = settings.value(QString("RTESW/%1/averageInfoMap").arg(t_sRTESWName), data).value<SCDISPLIB::AverageInfoMap>();
        m_pQuickControlWidget->setAverageInformationMapOld(map);

        m_pSelectionManagerWindow->updateDataView();

        broadcastScaling(m_qMapChScaling);

        //
        //-------- Init signal and background colors --------
        //
        QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
        backgroundBrush.setColor(settings.value(QString("RTESW/%1/layoutBackgroundColor").arg(t_sRTESWName), layoutBackgroundDefault).value<QColor>());
        m_pAverageScene->setBackgroundBrush(backgroundBrush);

        m_pButterflyPlot->setBackgroundColor(settings.value(QString("RTESW/%1/butterflyBackgroundColor").arg(t_sRTESWName), butterflyBackgroundDefault).value<QColor>());

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    //qDebug()<<" RealTimeEvokedWidget::channelSelectionManagerChanged - selectedChannelItems.size()" << selectedChannelItems.size();

    //Repaint the average items in the average scene based on the input parameter selectedChannelItems
    m_pAverageScene->repaintItems(selectedChannelItems);

    //call the onSelection function manually to replot the data for the givven average items
    onSelectionChanged();

    //fit everything in the view and update the scene
    m_pAverageLayoutView->fitInView(m_pAverageScene->sceneRect(), Qt::KeepAspectRatio);
    m_pAverageScene->update(m_pAverageScene->sceneRect());
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showSensorSelectionWidget()
{
    if(!m_pSelectionManagerWindow)
        m_pSelectionManagerWindow = QSharedPointer<SelectionManagerWindow>(new SelectionManagerWindow);

    m_pSelectionManagerWindow->show();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showSelectedChannelsOnly(QStringList selectedChannels)
{
    QList<int> selectedChannelsIndexes;

    for(int i = 0; i<selectedChannels.size(); i++)
        selectedChannelsIndexes<<m_pChInfoModel->getIndexFromOrigChName(selectedChannels.at(i));

    m_pButterflyPlot->setSelectedChannels(selectedChannelsIndexes);
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::broadcastScaling(QMap<qint32,float> scaleMap)
{
    //Set the scale map received from the scale window
    m_pAverageScene->setScaleMap(scaleMap);
    m_qMapChScaling = scaleMap;
    m_pRTESetModel->setScaling(scaleMap);
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::broadcastSettings(QList<Modality> modalityList)
{
    m_qListModalities = modalityList;
    m_pButterflyPlot->setSettings(modalityList);
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showQuickControlWidget()
{
    m_pQuickControlWidget->show();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::onSelectionChanged()
{
    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Get only the necessary data from the average model (use column 2)
        QList<QPair<double, SCDISPLIB::RowVectorPair> > averageData = m_pRTESetModel->data(0, 2, RealTimeEvokedSetModelRoles::GetAverageData).value<QList<QPair<double, SCDISPLIB::RowVectorPair> > >();

        //Get the averageScenItem specific data row
        int channelNumber = m_pChInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName);

        if(channelNumber != -1) {
            averageSceneItemTemp->m_iChannelKind = m_pFiffInfo->chs.at(channelNumber).kind;
            averageSceneItemTemp->m_iChannelUnit = m_pFiffInfo->chs.at(channelNumber).unit;
            averageSceneItemTemp->m_firstLastSample.first = (-1)*m_pRTESet->getNumPreStimSamples();

            if(!averageData.isEmpty()) {
                averageSceneItemTemp->m_firstLastSample.second = averageData.first().second.second - m_pRTESet->getNumPreStimSamples();
            }

            averageSceneItemTemp->m_iChannelNumber = channelNumber;
            averageSceneItemTemp->m_iTotalNumberChannels = m_pFiffInfo->ch_names.size();
            averageSceneItemTemp->m_lAverageData = averageData;
        }
    }

    m_pAverageScene->update();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showFilterWidget(bool state)
{
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


//*************************************************************************************************************

void RealTimeEvokedSetWidget::onTableViewBackgroundColorChanged(const QColor& backgroundColor)
{
    //Handle the butterfly plot and 2d layout plot differently
    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "2D Layout plot") {
        QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
        backgroundBrush.setColor(backgroundColor);
        m_pAverageScene->setBackgroundBrush(backgroundBrush);
    }

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "Butterfly plot") {
        m_pButterflyPlot->setBackgroundColor(backgroundColor);
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    //Handle the butterfly plot and 2d layout plot differently
    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "2D Layout plot") {
        if(imageType.contains("SVG"))
        {
            QString fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.svg").arg(sDate).arg(sTime);
            // Generate screenshot
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            QRectF rect = m_pAverageScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pAverageScene->render(&painter);
        }

        if(imageType.contains("PNG"))
        {
            QString fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.png").arg(sDate).arg(sTime);
            QPixmap pixMap = QPixmap::grabWidget(m_pAverageLayoutView);
            pixMap.save(fileName);
        }
    }

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "Butterfly plot") {
        if(imageType.contains("SVG"))
        {
            QString fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.svg").arg(sDate).arg(sTime);

            // Generate screenshot
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(m_pButterflyPlot->size());
            svgGen.setViewBox(m_pButterflyPlot->rect());

            m_pButterflyPlot->render(&svgGen);
        }

        if(imageType.contains("PNG"))
        {
            QString fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.png").arg(sDate).arg(sTime);

            QImage image(m_pButterflyPlot->size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);

            QPainter painter(&image);
            m_pButterflyPlot->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::wheelEvent(QWheelEvent * event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

bool RealTimeEvokedSetWidget::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pButterflyPlot || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pRTESetModel->toggleFreeze();
    }
    return false;
}

