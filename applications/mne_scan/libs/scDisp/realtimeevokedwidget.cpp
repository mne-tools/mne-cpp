//=============================================================================================================
/**
* @file     realtimeevokedwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the RealTimeEvokedWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedwidget.h"
//#include "annotationwindow.h"

#include <scMeas/realtimeevoked.h>


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

RealTimeEvokedWidget::RealTimeEvokedWidget(QSharedPointer<RealTimeEvoked> pRTE, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTEModel(Q_NULLPTR)
, m_pButterflyPlot(Q_NULLPTR)
, m_pAverageScene(Q_NULLPTR)
, m_pRTE(pRTE)
, m_pQuickControlWidget(Q_NULLPTR)
, m_pSelectionManagerWindow(Q_NULLPTR)
, m_pChInfoModel(Q_NULLPTR)
, m_pFilterWindow(Q_NULLPTR)
, m_pFiffInfo(Q_NULLPTR)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the region selection widget (F11)"),this);
    m_pActionSelectSensors->setShortcut(tr("F11"));
    m_pActionSelectSensors->setStatusTip(tr("Show the region selection widget (F11)"));
    connect(m_pActionSelectSensors, &QAction::triggered,
            this, &RealTimeEvokedWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(false);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl, &QAction::triggered,
            this, &RealTimeEvokedWidget::showQuickControlWidget);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(false);

    //set vertical layout
    m_pRteLayout = new QVBoxLayout(this);

    //Acquire label
    m_pLabelInit= new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;font.setBold(true);font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRteLayout->addWidget(m_pLabelInit);

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

    m_pRteLayout->addWidget(m_pToolBox);

    //set layouts
    this->setLayout(m_pRteLayout);

    getData();
}


//*************************************************************************************************************

RealTimeEvokedWidget::~RealTimeEvokedWidget()
{
    //
    // Store Settings
    //
    if(!m_pRTE->getName().isEmpty())
    {
        QString t_sRTEWName = m_pRTE->getName();

        QSettings settings;

        //Store modalities
        for(qint32 i = 0; i < m_qListModalities.size(); ++i) {
            settings.setValue(QString("RTEW/%1/%2/active").arg(t_sRTEWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_bActive);
            settings.setValue(QString("RTEW/%1/%2/norm").arg(t_sRTEWName).arg(m_qListModalities[i].m_sName), m_qListModalities[i].m_fNorm);
        }

        //Store filter
        if(m_pFilterWindow != 0) {
            FilterData filter = m_pFilterWindow->getUserDesignedFilter();

            settings.setValue(QString("RTEW/%1/filterHP").arg(t_sRTEWName), filter.m_dHighpassFreq);
            settings.setValue(QString("RTEW/%1/filterLP").arg(t_sRTEWName), filter.m_dLowpassFreq);
            settings.setValue(QString("RTEW/%1/filterOrder").arg(t_sRTEWName), filter.m_iFilterOrder);
            settings.setValue(QString("RTEW/%1/filterType").arg(t_sRTEWName), (int)filter.m_Type);
            settings.setValue(QString("RTEW/%1/filterDesignMethod").arg(t_sRTEWName), (int)filter.m_designMethod);
            settings.setValue(QString("RTEW/%1/filterTransition").arg(t_sRTEWName), filter.m_dParksWidth*(filter.m_sFreq/2));
            settings.setValue(QString("RTEW/%1/filterUserDesignActive").arg(t_sRTEWName), m_pFilterWindow->userDesignedFiltersIsActive());
            settings.setValue(QString("RTEW/%1/filterChannelType").arg(t_sRTEWName), m_pFilterWindow->getChannelType());
        }

        //Store scaling
        if(m_qMapChScaling.contains(FIFF_UNIT_T)) {
            settings.setValue(QString("RTEW/%1/scaleMAG").arg(t_sRTEWName), m_qMapChScaling[FIFF_UNIT_T]);
            qDebug()<<"m_qMapChScaling[FIFF_UNIT_T]: "<<m_qMapChScaling[FIFF_UNIT_T];
        }

        if(m_qMapChScaling.contains(FIFF_UNIT_T_M))
            settings.setValue(QString("RTEW/%1/scaleGRAD").arg(t_sRTEWName), m_qMapChScaling[FIFF_UNIT_T_M]);

        if(m_qMapChScaling.contains(FIFFV_EEG_CH))
            settings.setValue(QString("RTEW/%1/scaleEEG").arg(t_sRTEWName), m_qMapChScaling[FIFFV_EEG_CH]);

        if(m_qMapChScaling.contains(FIFFV_EOG_CH))
            settings.setValue(QString("RTEW/%1/scaleEOG").arg(t_sRTEWName), m_qMapChScaling[FIFFV_EOG_CH]);

        if(m_qMapChScaling.contains(FIFFV_STIM_CH))
            settings.setValue(QString("RTEW/%1/scaleSTIM").arg(t_sRTEWName), m_qMapChScaling[FIFFV_STIM_CH]);

        if(m_qMapChScaling.contains(FIFFV_MISC_CH))
            settings.setValue(QString("RTEW/%1/scaleMISC").arg(t_sRTEWName), m_qMapChScaling[FIFFV_MISC_CH]);

        //Store selected layout file
        if(!m_pSelectionManagerWindow == 0) {
            settings.setValue(QString("RTEW/%1/selectedLayoutFile").arg(t_sRTEWName), m_pSelectionManagerWindow->getCurrentLayoutFile());
        }

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("RTEW/%1/selectedView").arg(t_sRTEWName), m_pToolBox->currentIndex());
        }

        //Store signal and background colors
        if(m_pQuickControlWidget != 0) {
            settings.setValue(QString("RTEW/%1/signalColor").arg(t_sRTEWName), m_pQuickControlWidget->getSignalColor());
            settings.setValue(QString("RTEW/%1/butterflyBackgroundColor").arg(t_sRTEWName), m_pButterflyPlot->getBackgroundColor());
            settings.setValue(QString("RTEW/%1/layoutBackgroundColor").arg(t_sRTEWName), m_pAverageScene->backgroundBrush().color());
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::update(SCMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::getData()
{
    if(!m_bInitialized) {
        if(m_pRTE->isInitialized()) {
            m_qListChInfo = m_pRTE->chInfo();
            m_pFiffInfo = m_pRTE->info();

            m_iMaxFilterTapSize = m_pRTE->getValue()->data.cols();
            init();

            m_pRTEModel->updateData();
        }
    }
    else {
        //Check if block size has changed, if yes update the filter
        if(m_iMaxFilterTapSize != m_pRTE->getValue()->data.cols()) {
            m_iMaxFilterTapSize = m_pRTE->getValue()->data.cols();

            m_pFilterWindow->setWindowSize(m_iMaxFilterTapSize);
            m_pFilterWindow->setMaxFilterTaps(m_iMaxFilterTapSize);
        }

        m_pRTEModel->updateData();
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::init()
{
    if(m_qListChInfo.size() > 0)
    {
        //qDebug()<<"RealTimeEvokedWidget::init() - "<<m_pRTE->getName();
        QSettings settings;
        QString t_sRTEWName = m_pRTE->getName();
        m_pRteLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pToolBox->show();

        m_pRTEModel = RealTimeEvokedModel::SPtr(new RealTimeEvokedModel(this));
        m_pRTEModel->setRTE(m_pRTE);

        //m_pButterflyPlot->setModel(m_pRTEModel.data());

        //Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("RTEW/%1/selectedView").arg(t_sRTEWName), 0).toInt());

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
            sel = settings.value(QString("RTEW/%1/MAG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/MAG/norm").arg(t_sRTEWName), 1e-11f).toFloat();
            m_qListModalities.append(Modality("MAG",sel,val));
        }
        if(hasGrad) {
            sel = settings.value(QString("RTEW/%1/GRAD/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/GRAD/norm").arg(t_sRTEWName), 1e-10f).toFloat();
            m_qListModalities.append(Modality("GRAD",sel,val));
        }
        if(hasEEG) {
            sel = settings.value(QString("RTEW/%1/EEG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/EEG/norm").arg(t_sRTEWName), 1e-4f).toFloat();
            m_qListModalities.append(Modality("EEG",sel,val));
        }
        if(hasEOG) {
            sel = settings.value(QString("RTEW/%1/EOG/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/EOG/norm").arg(t_sRTEWName), 1e-3f).toFloat();
            m_qListModalities.append(Modality("EOG",sel,val));
        }
        if(hasMISC) {
            sel = settings.value(QString("RTEW/%1/MISC/active").arg(t_sRTEWName), true).toBool();
            val = settings.value(QString("RTEW/%1/MISC/norm").arg(t_sRTEWName), 1e-3f).toFloat();
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

        QString t_sRTEName = m_pRTE->getName();

        if(!t_sRTEName.isEmpty())
        {
            qDebug()<<"Init scaling";
            m_qMapChScaling.clear();

            QSettings settings;
            float val = 0.0f;
            if(availabeChannelTypes.contains(FIFF_UNIT_T)) {
                val = settings.value(QString("RTEW/%1/scaleMAG").arg(t_sRTEWName), 1e-11f).toFloat();
                m_qMapChScaling.insert(FIFF_UNIT_T, val);
            }

            if(availabeChannelTypes.contains(FIFF_UNIT_T_M)) {
                val = settings.value(QString("RTEW/%1/scaleGRAD").arg(t_sRTEWName), 1e-10f).toFloat();
                m_qMapChScaling.insert(FIFF_UNIT_T_M, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EEG_CH)) {
                val = settings.value(QString("RTEW/%1/scaleEEG").arg(t_sRTEWName), 1e-4f).toFloat();
                m_qMapChScaling.insert(FIFFV_EEG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_EOG_CH)) {
                val = settings.value(QString("RTEW/%1/scaleEOG").arg(t_sRTEWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_EOG_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_STIM_CH)) {
                val = settings.value(QString("RTEW/%1/scaleSTIM").arg(t_sRTEWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_STIM_CH, val);
            }

            if(availabeChannelTypes.contains(FIFFV_MISC_CH)) {
                val = settings.value(QString("RTEW/%1/scaleMISC").arg(t_sRTEWName), 1e-3f).toFloat();
                m_qMapChScaling.insert(FIFFV_MISC_CH, val);
            }

            m_pRTEModel->setScaling(m_qMapChScaling);
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
                m_pRTEModel.data(),static_cast<void (RealTimeEvokedModel::*)(QString)>(&RealTimeEvokedModel::setFilterChannelType));

        connect(m_pFilterWindow.data(), &FilterWindow::filterChanged,
                m_pRTEModel.data(), &RealTimeEvokedModel::filterChanged);

        //Init downsampled sampling frequency
        m_pFilterWindow->setSamplingRate(m_pFiffInfo->sfreq);

        //Set stored filter settings from last session
        m_pFilterWindow->setFilterParameters(settings.value(QString("RTEW/%1/filterHP").arg(t_sRTEWName), 5.0).toDouble(),
                                                settings.value(QString("RTEW/%1/filterLP").arg(t_sRTEWName), 40.0).toDouble(),
                                                settings.value(QString("RTEW/%1/filterOrder").arg(t_sRTEWName), 128).toInt(),
                                                settings.value(QString("RTEW/%1/filterType").arg(t_sRTEWName), 2).toInt(),
                                                settings.value(QString("RTEW/%1/filterDesignMethod").arg(t_sRTEWName), 0).toInt(),
                                                settings.value(QString("RTEW/%1/filterTransition").arg(t_sRTEWName), 5.0).toDouble(),
                                                settings.value(QString("RTEW/%1/filterUserDesignActive").arg(t_sRTEWName), false).toBool(),
                                                settings.value(QString("RTEW/%1/filterChannelType").arg(t_sRTEWName), "MEG").toString());

        //
        //-------- Init channel selection manager --------
        //
        m_pChInfoModel = QSharedPointer<ChInfoModel>(new ChInfoModel(m_pFiffInfo, this));
        m_pSelectionManagerWindow = QSharedPointer<SelectionManagerWindow>(new SelectionManagerWindow(this, m_pChInfoModel));

        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::showSelectedChannelsOnly,
                this, &RealTimeEvokedWidget::showSelectedChannelsOnly);

        //Connect channel info model
        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::loadedLayoutMap,
                m_pChInfoModel.data(), &ChInfoModel::layoutChanged);

        connect(m_pChInfoModel.data(), &ChInfoModel::channelsMappedToLayout,
                m_pSelectionManagerWindow.data(), &SelectionManagerWindow::setCurrentlyMappedFiffChannels);

        m_pChInfoModel->fiffInfoChanged(m_pFiffInfo);

        m_pSelectionManagerWindow->setCurrentLayoutFile(settings.value(QString("RTEW/%1/selectedLayoutFile").arg(t_sRTEWName), "babymeg-mag-inner-layer.lout").toString());

        m_pActionSelectSensors->setVisible(true);

        //
        //-------- Init quick control widget --------
        //
        QStringList slFlags;
        slFlags <<  "projections" << "compensators" << "filter" << "scaling" << "modalities" << "colors";

        m_pQuickControlWidget = QSharedPointer<QuickControlWidget>(new QuickControlWidget(m_qMapChScaling, m_pFiffInfo, "RT Averaging", slFlags));

        //m_pQuickControlWidget = QuickControlWidget::SPtr(new QuickControlWidget(m_qMapChScaling, m_pFiffInfo, "RT Averaging", 0, true, true, false, true, true, false));
        m_pQuickControlWidget->setWindowFlags(Qt::WindowStaysOnTopHint);

        //Handle scaling
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::scalingChanged,
                this, &RealTimeEvokedWidget::broadcastScaling);

        //Handle background color changes
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::backgroundColorChanged,
                this, &RealTimeEvokedWidget::onTableViewBackgroundColorChanged);

        //Handle screenshot signals
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::makeScreenshot,
                this, &RealTimeEvokedWidget::onMakeScreenshot);

        //Handle compensators
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::compSelectionChanged,
                m_pRTEModel.data(), &RealTimeEvokedModel::updateCompensator);

        //Handle projections
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::projSelectionChanged,
                m_pRTEModel.data(), &RealTimeEvokedModel::updateProjection);

        //Handle modalities
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::settingsChanged,
                this, &RealTimeEvokedWidget::broadcastSettings);

        //Handle filtering
        connect(m_pFilterWindow.data(), &FilterWindow::activationCheckBoxListChanged,
                m_pQuickControlWidget.data(), &QuickControlWidget::filterGroupChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::showFilterOptions,
                this, &RealTimeEvokedWidget::showFilterWidget);

        //Handle updating the butterfly and layout plot
        connect(m_pQuickControlWidget.data(), &QuickControlWidget::updateConnectedView,
                m_pButterflyPlot.data(), &RealTimeButterflyPlot::updateView);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::updateConnectedView,
                this, &RealTimeEvokedWidget::onSelectionChanged);

        m_pQuickControlWidget->setViewParameters(settings.value(QString("RTEW/%1/viewZoomFactor").arg(t_sRTEWName), 1.0).toFloat(),
                                                     settings.value(QString("RTEW/%1/viewWindowSize").arg(t_sRTEWName), 10).toInt(),
                                                     settings.value(QString("RTEW/%1/viewOpacity").arg(t_sRTEWName), 95).toInt());

        m_pQuickControlWidget->filterGroupChanged(m_pFilterWindow->getActivationCheckBoxList());

        QColor signalDefault = Qt::darkBlue;
        QColor butterflyBackgroundDefault = Qt::white;
        QColor layoutBackgroundDefault = Qt::black;
        m_pQuickControlWidget->setSignalBackgroundColors(settings.value(QString("RTEW/%1/signalColor").arg(t_sRTEWName), signalDefault).value<QColor>(), settings.value(QString("RTEW/%1/butterflyBackgroundColor").arg(t_sRTEWName), butterflyBackgroundDefault).value<QColor>());

        m_pActionQuickControl->setVisible(true);

        //Activate projections as default
        m_pRTEModel->updateProjection();

        //
        //-------- Init average scene --------
        //
        m_pAverageScene = AverageScene::SPtr(new AverageScene(m_pAverageLayoutView, this));
        m_pAverageLayoutView->setScene(m_pAverageScene.data());
        QBrush brush(Qt::black);
        m_pAverageScene->setBackgroundBrush(brush);

        //Connect selection manager with average manager
        connect(m_pSelectionManagerWindow.data(), &SelectionManagerWindow::selectionChanged,
                this, &RealTimeEvokedWidget::channelSelectionManagerChanged);

        connect(m_pRTEModel.data(), &RealTimeEvokedModel::dataChanged,
                this, &RealTimeEvokedWidget::onSelectionChanged);

        connect(m_pQuickControlWidget.data(), &QuickControlWidget::scalingChanged,
                this, &RealTimeEvokedWidget::scaleAveragedData);

        m_pSelectionManagerWindow->updateDataView();

        //
        //-------- Init signal and background colors --------
        //
        QBrush backgroundBrush = m_pAverageScene->backgroundBrush();
        backgroundBrush.setColor(settings.value(QString("RTEW/%1/layoutBackgroundColor").arg(t_sRTEWName), layoutBackgroundDefault).value<QColor>());
        m_pAverageScene->setBackgroundBrush(backgroundBrush);

        m_pButterflyPlot->setBackgroundColor(settings.value(QString("RTEW/%1/butterflyBackgroundColor").arg(t_sRTEWName), butterflyBackgroundDefault).value<QColor>());

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    //Repaint the average items in the average scene based on the input parameter selectedChannelItems
    m_pAverageScene->repaintItems(selectedChannelItems);

    //call the onSelection function manually to replot the data for the givven average items
    onSelectionChanged();

    //fit everything in the view and update the scene
    m_pAverageLayoutView->fitInView(m_pAverageScene->sceneRect(), Qt::KeepAspectRatio);
    m_pAverageScene->update(m_pAverageScene->sceneRect());
}


//*************************************************************************************************************

void RealTimeEvokedWidget::scaleAveragedData(const QMap<qint32, float> &scaleMap)
{
    //Set the scale map received from the scale window
    m_pAverageScene->setScaleMap(scaleMap);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showSensorSelectionWidget()
{
    if(!m_pSelectionManagerWindow)
        m_pSelectionManagerWindow = QSharedPointer<SelectionManagerWindow>(new SelectionManagerWindow);

    m_pSelectionManagerWindow->show();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showSelectedChannelsOnly(QStringList selectedChannels)
{
    QList<int> selectedChannelsIndexes;

    for(int i = 0; i<selectedChannels.size(); i++)
        selectedChannelsIndexes<<m_pChInfoModel->getIndexFromOrigChName(selectedChannels.at(i));

    m_pButterflyPlot->setSelectedChannels(selectedChannelsIndexes);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::broadcastScaling(QMap<qint32,float> scaleMap)
{
    m_pRTEModel->setScaling(scaleMap);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::broadcastSettings(QList<Modality> modalityList)
{
    m_qListModalities = modalityList;
    m_pButterflyPlot->setSettings(modalityList);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showQuickControlWidget()
{
    m_pQuickControlWidget->show();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::onSelectionChanged()
{
    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Get only the necessary data from the average model (use column 2)
        RowVectorPair averageData = m_pRTEModel->data(0, 2, RealTimeEvokedModelRoles::GetAverageData).value<RowVectorPair>();

        //Get the averageScenItem specific data row
        int channelNumber = m_pChInfoModel->getIndexFromMappedChName(averageSceneItemTemp->m_sChannelName);

        if(channelNumber != -1) {
            averageSceneItemTemp->m_iChannelKind = m_pFiffInfo->chs.at(channelNumber).kind;
            averageSceneItemTemp->m_iChannelUnit = m_pFiffInfo->chs.at(channelNumber).unit;
            averageSceneItemTemp->m_firstLastSample.first = (-1)*m_pRTE->getNumPreStimSamples();
            averageSceneItemTemp->m_firstLastSample.second = averageData.second-m_pRTE->getNumPreStimSamples();
            averageSceneItemTemp->m_iChannelNumber = channelNumber;
            averageSceneItemTemp->m_iTotalNumberChannels = m_pFiffInfo->ch_names.size();
            averageSceneItemTemp->m_lAverageData.append(QPair<double, RowVectorPair>(0,averageData));
        }
    }

    m_pAverageScene->update();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showFilterWidget(bool state)
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

void RealTimeEvokedWidget::onTableViewBackgroundColorChanged(const QColor& backgroundColor)
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

void RealTimeEvokedWidget::onMakeScreenshot(const QString& imageType)
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

void RealTimeEvokedWidget::wheelEvent(QWheelEvent * event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

bool RealTimeEvokedWidget::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pButterflyPlot || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pRTEModel->toggleFreeze();
    }
    return false;
}

