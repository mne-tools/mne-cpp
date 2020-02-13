//=============================================================================================================
/**
 * @file     realtimeevokedsetwidget.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedsetwidget.h"

#include <disp/viewers/quickcontrolview.h>
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

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSetWidget::RealTimeEvokedSetWidget(QSharedPointer<RealTimeEvokedSet> pRTESet,
                                                 QSharedPointer<QTime> &pTime,
                                                 QWidget* parent)
: MeasurementWidget(parent)
, m_bInitialized(false)
, m_pRTESet(pRTESet)
{
    Q_UNUSED(pTime)

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the region selection widget (F11)"),this);
    m_pActionSelectSensors->setShortcut(tr("F11"));
    m_pActionSelectSensors->setStatusTip(tr("Show the region selection widget (F11)"));
    connect(m_pActionSelectSensors.data(), &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);
    m_pActionSelectSensors->setVisible(false);

    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showQuickControlWidget);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

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

    //Create toolboxes with butterfly and 2D layout plot
    m_pToolBox = new QToolBox(this);
    m_pToolBox->hide();

    //Butterfly
    m_pButterflyView = new ButterflyView(this);
    m_pButterflyView->installEventFilter(this);

    //2D layout plot
    m_pAverageLayoutView = new AverageLayoutView(this);
    //m_pAverageLayoutView->installEventFilter(this);

    m_pToolBox->insertItem(0, m_pButterflyView, QIcon(), "Butterfly plot");
    m_pToolBox->insertItem(0, m_pAverageLayoutView, QIcon(), "2D Layout plot");

    m_pRTESetLayout->addWidget(m_pToolBox);

    // Init quick control view
    m_pQuickControlView = QSharedPointer<QuickControlView>::create(QString("RTESW/%1/viewOpacity").arg(m_pRTESet->getName()),
                                                                   "RT Averaging",
                                                                   Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint,
                                                                   this);

    this->addControlWidgets(m_pQuickControlView,
                            m_pRTESet->getControlWidgets());

    //set layouts
    this->setLayout(m_pRTESetLayout);

    //Initialize leftover scalars to default values
    if(!m_pRTESet->getValue()->evoked.isEmpty()){
        m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();
    } else {
        m_iMaxFilterTapSize = 0;
    }

    m_bHideBadChannels = false;
}


//*************************************************************************************************************

RealTimeEvokedSetWidget::~RealTimeEvokedSetWidget()
{
    // Store Settings
    if(!m_pRTESet->getName().isEmpty())
    {
        QSettings settings;

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("RTESW/%1/selectedView").arg(m_pRTESet->getName()), m_pToolBox->currentIndex());
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::update(SCMEASLIB::Measurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::getData()
{
    if(!m_bInitialized) {
        if(m_pRTESet->isInitialized()) {
            m_pFiffInfo = m_pRTESet->info();

            init();
        }
    } else {
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


//*************************************************************************************************************

void RealTimeEvokedSetWidget::init()
{
    if(m_pFiffInfo) {
        m_pActionQuickControl->setVisible(true);

        QSettings settings;
        QString t_sRTESName = m_pRTESet->getName();

        // Remove temporary label and show actual average display
        m_pRTESetLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();
        m_pToolBox->show();
        m_pActionSelectSensors->setVisible(true);

        // Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("RTESW/%1/selectedView").arg(t_sRTESName), 0).toInt());

        // Init data model and set first data
        m_pEvokedSetModel = EvokedSetModel::SPtr::create(this);
        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *m_pFiffInfo.data();
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);
        m_pButterflyView->setEvokedSetModel(m_pEvokedSetModel);

        //Init channel info and selection view
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create(QString("RTESW/%1").arg(t_sRTESName),
                                                                               this,
                                                                               m_pChannelInfoModel,
                                                                               Qt::Window);

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

        // Quick control scaling        
        ScalingView* pScalingView = new ScalingView(QString("RTESW/%1").arg(t_sRTESName),
                                                    m_pFiffInfo->chs);
        m_pQuickControlView->addGroupBox(pScalingView, "Scaling");

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pButterflyView.data(), &ButterflyView::setScaleMap);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setScaleMap);

        m_pButterflyView->setScaleMap(pScalingView->getScaleMap());
        m_pAverageLayoutView->setScaleMap(pScalingView->getScaleMap());

        // Quick control projectors
        ProjectorsView* pProjectorsView = new ProjectorsView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pProjectorsView, "Noise", "SSP");

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateProjection);

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        pProjectorsView->setProjectors(m_pFiffInfo->projs);

        // Quick control compensators
        CompensatorView* pCompensatorView = new CompensatorView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pCompensatorView, "Noise", "Comp");

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateCompensator);

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        pCompensatorView->setCompensators(m_pFiffInfo->comps);

        // Quick control filter settings
        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pFilterSettingsView, "Noise", "Filter");

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setFilterChannelType);

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setFilter);

        connect(this, &RealTimeEvokedSetWidget::windowSizeChanged,
                pFilterSettingsView->getFilterView().data(), &FilterDesignView::setWindowSize);

        connect(this, &RealTimeEvokedSetWidget::windowSizeChanged,
                pFilterSettingsView->getFilterView().data(), &FilterDesignView::setMaxFilterTaps);

        connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setFilterActive);

        m_pEvokedSetModel->setFilterActive(pFilterSettingsView->getFilterActive());

        pFilterSettingsView->getFilterView()->init(m_pFiffInfo->sfreq);

        if(!m_pRTESet->getValue()->evoked.isEmpty()) {
            m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.first().data.cols();

            pFilterSettingsView->getFilterView()->setWindowSize(m_iMaxFilterTapSize);
            pFilterSettingsView->getFilterView()->setMaxFilterTaps(m_iMaxFilterTapSize);
        }

        // Quick control channel data settings
        FiffRawViewSettings* pChannelDataSettingsView = new FiffRawViewSettings(QString("RTESW/%1").arg(t_sRTESName));
        pChannelDataSettingsView->setWidgetList(QStringList() << "screenshot" << "backgroundColor");
        m_pQuickControlView->addGroupBoxWithTabs(pChannelDataSettingsView, "Other", "View");

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
                                                                                  QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pModalitySelectionView, "Other", "Modalities");

        connect(pModalitySelectionView, &ModalitySelectionView::modalitiesChanged,
                m_pButterflyView.data(), &ButterflyView::setModalityMap);

        m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

        // Quick control average selection
        AverageSelectionView* pAverageSelectionView = new AverageSelectionView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pAverageSelectionView, "Averaging", "Selection");

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

        //Initialized
        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showSensorSelectionWidget()
{
    if(!m_pChannelSelectionView) {
        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create();
    }

    m_pChannelSelectionView->show();
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::showQuickControlWidget()
{
    m_pQuickControlView->raise();
    m_pQuickControlView->show();
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


//*************************************************************************************************************

bool RealTimeEvokedSetWidget::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pButterflyView || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pEvokedSetModel->toggleFreeze();
    }
    return false;
}

