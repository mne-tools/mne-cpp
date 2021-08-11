//=============================================================================================================
/**
 * @file     realtimeevokedsetwidget.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedsetwidget.h"

#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/averagingsettingsview.h>

#include <scMeas/realtimeevokedset.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QLabel>
#include <QToolBox>
#include <QDate>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGraphicsItem>
#include <QDir>
#include <QSettings>
#include <QEvent>
#include <QToolBar>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSetWidget::RealTimeEvokedSetWidget(QSharedPointer<QTime> &pTime,
                                                 QWidget* parent)
: MeasurementWidget(parent)
, m_iMaxFilterTapSize(0)
{
    Q_UNUSED(pTime)

    QAction* pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the channel selection window"),this);
    pActionSelectSensors->setStatusTip(tr("Show the channel selection view"));
    connect(pActionSelectSensors, &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showSensorSelectionWidget);

    //Create GUI
    m_pRTESetLayout = new QVBoxLayout(this);

    //Set acquire label
    m_pLabelInit= new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setBold(true);
    font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRTESetLayout->addWidget(m_pLabelInit);
    m_pRTESetLayout->setContentsMargins(0,0,0,0);

    //Create toolboxes with butterfly and 2D layout plot
    m_pToolBox = new QToolBox(this);
    m_pToolBox->hide();

    // Create tool bar
    QToolBar* pToolBar = new QToolBar;
    pToolBar->addAction(pActionSelectSensors);

    //Butterfly
    m_pButterflyView = new ButterflyView("MNESCAN/RTESW", this);
    m_pButterflyView->installEventFilter(this);

    //2D layout plot
    m_pAverageLayoutView = new AverageLayoutView("MNESCAN/RTESW", this);
    //m_pAverageLayoutView->installEventFilter(this);

    m_pToolBox->insertItem(0, m_pButterflyView, QIcon(), "Butterfly plot");
    m_pToolBox->insertItem(0, m_pAverageLayoutView, QIcon(), "2D Layout plot");

    m_pRTESetLayout->addWidget(m_pToolBox);

    //set layouts
    this->setLayout(m_pRTESetLayout);

    m_bHideBadChannels = false;
}

//=============================================================================================================

RealTimeEvokedSetWidget::~RealTimeEvokedSetWidget()
{
    // Save Settings
    if(!m_pRTESet->getName().isEmpty())
    {
        QSettings settings("MNECPP");

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("MNESCAN/RTESW/selectedView"), m_pToolBox->currentIndex());
        }
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(!m_pRTESet) {
        m_pRTESet = qSharedPointerDynamicCast<RealTimeEvokedSet>(pMeasurement);
    }

    if(m_pRTESet) {
        if(!m_pFiffInfo) {
            m_pFiffInfo = m_pRTESet->info();

            if(!m_bDisplayWidgetsInitialized) {
                initDisplayControllWidgets();
            }
        }

        if(m_pRTESet->isInitialized()) {
            //Check if block size has changed, if yes update the filter
            if(!m_pRTESet->getValue()->evoked.isEmpty()) {
                if(m_iMaxFilterTapSize != m_pRTESet->getValue()->evoked.first().data.cols()) {
                    m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();

                    emit windowSizeChanged(m_iMaxFilterTapSize);
                }
            }

            FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
            pEvokedSet->info = *(m_pFiffInfo.data());
            m_pEvokedSetModel->setEvokedSet(pEvokedSet);
        }
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::initDisplayControllWidgets()
{
    if(m_pFiffInfo) {
        QSettings settings("MNECPP");
        QString t_sRTESName = m_pRTESet->getName();

        //Initialize leftover scalars to default values
        if(!m_pRTESet->getValue()->evoked.isEmpty()){
            m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();
        } else {
            m_iMaxFilterTapSize = 0;
        }

        // Remove temporary label and show actual average display
        m_pRTESetLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();
        m_pToolBox->show();

        // Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("MNESCAN/RTESW/selectedView"), 0).toInt());

        // Init data model and set first data
        m_pEvokedSetModel = EvokedSetModel::SPtr::create(this);
        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *m_pFiffInfo.data();
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);
        m_pButterflyView->setEvokedSetModel(m_pEvokedSetModel);

        //Init channel info and selection view
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create(QString("MNESCAN/RTESW"),
                                                                               this,
                                                                               m_pChannelInfoModel,
                                                                               Qt::Window);
        m_pChannelSelectionView->setWindowTitle(tr(QString("%1: Channel Selection Window").arg(t_sRTESName).toUtf8()));

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pButterflyView.data(), &ButterflyView::showSelectedChannelsOnly);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::selectionChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::channelSelectionManagerChanged);

        // Init the views. This needs to be done in the following order because we need to draw the items in the m_pChannelSelectionView first, hence we need to init it first.
        m_pButterflyView->setChannelInfoModel(m_pChannelInfoModel);
        m_pChannelSelectionView->updateDataView();
        m_pAverageLayoutView->setEvokedSetModel(m_pEvokedSetModel);
        m_pAverageLayoutView->setChannelInfoModel(m_pChannelInfoModel);
        m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

        //Init control widgets
        QList<QWidget*> lControlWidgets;

//        // Quick control projectors
//        ProjectorsView* pProjectorsView = new ProjectorsView(QString("MNESCAN/RTESW"));
//        pProjectorsView->setObjectName("group_tab_View_SSP");
//        lControlWidgets.append(pProjectorsView);

//        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
//                m_pEvokedSetModel.data(), &EvokedSetModel::updateProjection);

//        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
//                m_pButterflyView.data(), &ButterflyView::updateView);

//        pProjectorsView->setProjectors(m_pFiffInfo->projs);

//        // Quick control compensators
//        CompensatorView* pCompensatorView = new CompensatorView(QString("MNESCAN/RTESW"));
//        pCompensatorView->setObjectName("group_tab_View_Comp");
//        lControlWidgets.append(pCompensatorView);

//        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
//                m_pEvokedSetModel.data(), &EvokedSetModel::updateCompensator);

//        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
//                m_pButterflyView.data(), &ButterflyView::updateView);

//        pCompensatorView->setCompensators(m_pFiffInfo->comps);

//        // Quick control filter settings
//        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("MNESCAN/RTESW"));
//        pFilterSettingsView->setObjectName("group_tab_View_Filter");
//        lControlWidgets.append(pFilterSettingsView);

//        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
//                m_pEvokedSetModel.data(), &EvokedSetModel::setFilterChannelType);

//        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
//                m_pEvokedSetModel.data(), &EvokedSetModel::setFilter);

//        connect(this, &RealTimeEvokedSetWidget::windowSizeChanged,
//                pFilterSettingsView->getFilterView().data(), &FilterDesignView::setWindowSize);

//        connect(this, &RealTimeEvokedSetWidget::windowSizeChanged,
//                pFilterSettingsView->getFilterView().data(), &FilterDesignView::setMaxAllowedFilterTaps);

//        connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
//                m_pEvokedSetModel.data(), &EvokedSetModel::setFilterActive);

//        m_pEvokedSetModel->setFilterActive(pFilterSettingsView->getFilterActive());

//        pFilterSettingsView->getFilterView()->init(m_pFiffInfo->sfreq);

//        if(!m_pRTESet->getValue()->evoked.isEmpty()) {
//            m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();

//            pFilterSettingsView->getFilterView()->setWindowSize(m_iMaxFilterTapSize);
//            pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(m_iMaxFilterTapSize);
//        }

        // Scaling
        ScalingView* pScalingView = new ScalingView(QString("MNESCAN/RTESW"),0, Qt::Widget, m_pFiffInfo->get_channel_types());
        pScalingView->setObjectName("group_tab_View_Scaling");
        lControlWidgets.append(pScalingView);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pButterflyView.data(), &ButterflyView::setScaleMap);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setScaleMap);

        m_pButterflyView->setScaleMap(pScalingView->getScaleMap());
        m_pAverageLayoutView->setScaleMap(pScalingView->getScaleMap());

        // Quick control channel data settings
        FiffRawViewSettings* pChannelDataSettingsView = new FiffRawViewSettings(QString("MNESCAN/RTESW"));
        pChannelDataSettingsView->setWidgetList(QStringList() << "screenshot" << "backgroundColor");
        pChannelDataSettingsView->setObjectName("group_tab_View_General");
        lControlWidgets.append(pChannelDataSettingsView);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::backgroundColorChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setBackgroundColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::backgroundColorChanged,
                m_pButterflyView.data(), &ButterflyView::setBackgroundColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::makeScreenshot,
                this, &RealTimeEvokedSetWidget::onMakeScreenshot);

        m_pAverageLayoutView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
        m_pButterflyView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());

        // Quick control modality selection
        ModalitySelectionView* pModalitySelectionView = new ModalitySelectionView(m_pFiffInfo->chs,
                                                                                  QString("MNESCAN/RTESW"));
        pModalitySelectionView->setObjectName("group_tab_View_Modalities");
        lControlWidgets.append(pModalitySelectionView);

        connect(pModalitySelectionView, &ModalitySelectionView::modalitiesChanged,
                m_pButterflyView.data(), &ButterflyView::setModalityMap);

        m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

        // Quick control average selection
        AverageSelectionView* pAverageSelectionView = new AverageSelectionView(QString("MNESCAN/RTESW"));
        pAverageSelectionView->setObjectName("group_tab_View_Selection");
        lControlWidgets.append(pAverageSelectionView);

        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageActivationMap,
                pAverageSelectionView, &AverageSelectionView::setAverageActivation);
        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageColorMap,
                pAverageSelectionView, &AverageSelectionView::setAverageColor);

        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageColorMap,
                m_pButterflyView.data(), &ButterflyView::setAverageColor);
        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageActivationMap,
                m_pButterflyView.data(), &ButterflyView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageActivationMap,
                m_pButterflyView.data(), &ButterflyView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageColorMap,
                m_pButterflyView.data(), &ButterflyView::setAverageColor);

        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageColorMap,
                m_pAverageLayoutView.data(), &AverageLayoutView::setAverageColor);
        connect(m_pEvokedSetModel.data(), &EvokedSetModel::newAverageActivationMap,
                m_pAverageLayoutView.data(), &AverageLayoutView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageActivationMap,
                m_pAverageLayoutView.data(), &AverageLayoutView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageColorMap,
                m_pAverageLayoutView.data(), &AverageLayoutView::setAverageColor);

        m_pEvokedSetModel->setAverageActivation(pAverageSelectionView->getAverageActivation());
        m_pEvokedSetModel->setAverageColor(pAverageSelectionView->getAverageColor());
        m_pButterflyView->setAverageActivation(pAverageSelectionView->getAverageActivation());
        m_pButterflyView->setAverageColor(pAverageSelectionView->getAverageColor());
        m_pAverageLayoutView->setAverageActivation(pAverageSelectionView->getAverageActivation());
        m_pAverageLayoutView->setAverageColor(pAverageSelectionView->getAverageColor());

        emit displayControlWidgetsChanged(lControlWidgets, t_sRTESName);

        //Initialized
        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::updateOpenGLViewport()
{
    if(m_pAverageLayoutView) {
        m_pAverageLayoutView->updateOpenGLViewport();
    }
    if(m_pButterflyView) {
        m_pButterflyView->updateOpenGLViewport();
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::showSensorSelectionWidget()
{
    if(!m_pChannelSelectionView) {
        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create();
    }

    m_pChannelSelectionView->show();
}

//=============================================================================================================

void RealTimeEvokedSetWidget::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    //Handle the butterfly plot and 2D layout plot differently
    QString fileName;

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "2D Layout plot") {
        if(imageType.contains("SVG")) {
            fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.svg").arg(sDate).arg(sTime);
        } else if(imageType.contains("PNG")) {
            fileName = QString("./Screenshots/%1-%2-LayoutScreenshot.png").arg(sDate).arg(sTime);
        }
    }

    if(m_pToolBox->itemText(m_pToolBox->currentIndex()) == "Butterfly plot") {
        if(imageType.contains("SVG")) {
            fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.svg").arg(sDate).arg(sTime);
        } else if(imageType.contains("PNG")) {
            fileName = QString("./Screenshots/%1-%2-ButterflyScreenshot.png").arg(sDate).arg(sTime);
        }
    }

    m_pButterflyView->takeScreenshot(fileName);
}

//=============================================================================================================

bool RealTimeEvokedSetWidget::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pButterflyView || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pEvokedSetModel->toggleFreeze();
    }
    return false;
}

