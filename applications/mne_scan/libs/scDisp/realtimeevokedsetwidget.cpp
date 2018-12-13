//=============================================================================================================
/**
* @file     realtimeevokedsetwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
#include <disp/viewers/filterview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/channeldatasettingsview.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/averagingsettingsview.h>

#include <scMeas/realtimeevokedset.h>

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
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
    m_pActionQuickControl->setVisible(false);

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
    m_pQuickControlView = QSharedPointer<QuickControlView>::create("RT Averaging", Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint, this);
    QSettings settings;
    m_pQuickControlView->setOpacityValue(settings.value(QString("RTESW/%1/viewOpacity").arg(m_pRTESet->getName()), 100).toInt());
    m_pActionQuickControl->setVisible(true);

    QList<QSharedPointer<QWidget> > lControlWidgets = m_pRTESet->getControlWidgets();

    for(int i = 0; i < lControlWidgets.size(); ++i) {
        QString sObjectName = lControlWidgets.at(i)->objectName();

        if(sObjectName.contains("widget_", Qt::CaseInsensitive)) {
            m_pQuickControlView->addWidget(lControlWidgets.at(i));
        }

        if(sObjectName.contains("group_", Qt::CaseInsensitive)) {
            if(sObjectName.contains("group_tab_", Qt::CaseInsensitive)) {
                sObjectName.remove("group_tab_");
                QStringList sList = sObjectName.split("_");
                qDebug() << "RealTimeMultiSampleArrayWidget sList" << sList;
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

    //set layouts
    this->setLayout(m_pRTESetLayout);
}


//*************************************************************************************************************

RealTimeEvokedSetWidget::~RealTimeEvokedSetWidget()
{
    // Store Settings
    if(!m_pRTESet->getName().isEmpty())
    {
        QString t_sRTESName = m_pRTESet->getName();

        QSettings settings;

        //Store current view toolbox index - butterfly or 2D layout
        if(m_pToolBox) {
            settings.setValue(QString("RTESW/%1/selectedView").arg(t_sRTESName), m_pToolBox->currentIndex());
        }

        //Store average colors per type
        if(m_pEvokedSetModel) {
            settings.beginGroup(QString("RTESW/%1/averageColorMap").arg(t_sRTESName));
            QMap<QString, QColor>::const_iterator iColor = m_pEvokedSetModel->getAverageColor()->constBegin();
            while (iColor != m_pEvokedSetModel->getAverageColor()->constEnd()) {
                 settings.setValue(iColor.key(), iColor.value());
                 ++iColor;
            }
            settings.endGroup();

            settings.beginGroup(QString("RTESW/%1/averageActivationMap").arg(t_sRTESName));
            QMap<QString, bool>::const_iterator iActivation = m_pEvokedSetModel->getAverageActivation()->constBegin();
            while (iActivation != m_pEvokedSetModel->getAverageActivation()->constEnd()) {
                 settings.setValue(iActivation.key(), iActivation.value());
                 ++iActivation;
            }
            settings.endGroup();
        }

        //Store signal and background colors
        if(m_pQuickControlView) {
            settings.setValue(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), m_pButterflyView->getBackgroundColor());
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

//                m_pFilterView->setWindowSize(m_iMaxFilterTapSize);
//                m_pFilterView->setMaxFilterTaps(m_iMaxFilterTapSize);
            }
        }

        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *(m_pFiffInfo.data());
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);

        if(m_pAveragingSettingsView) {
            m_pAveragingSettingsView->setDetectedEpochs(pEvokedSet);
        }
    }
}


//*************************************************************************************************************

void RealTimeEvokedSetWidget::init()
{
    if(m_pFiffInfo) {
        QSettings settings;
        QString t_sRTESName = m_pRTESet->getName();

        // Remove temporary label and show actual average display
        m_pRTESetLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();
        m_pToolBox->show();
        m_pActionSelectSensors->setVisible(true);

        // Choose current view toolbox index - butterfly or 2D layout
        m_pToolBox->setCurrentIndex(settings.value(QString("RTESW/%1/selectedView").arg(t_sRTESName), 0).toInt());

        // Init data model
        m_pEvokedSetModel = EvokedSetModel::SPtr::create(this);

        // Set the inital data
        FiffEvokedSet::SPtr pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *m_pFiffInfo.data();
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);

        QMap<QString, QColor> qMapAverageColor;
        settings.beginGroup(QString("RTESW/%1/averageColorMap").arg(t_sRTESName));
        QStringList keys = settings.childKeys();
        foreach (QString key, keys) {
             qMapAverageColor[key] = settings.value(key).value<QColor>();
        }
        settings.endGroup();
        QSharedPointer<QMap<QString, QColor> > pqMapAverageColor = QSharedPointer<QMap<QString, QColor> >::create(qMapAverageColor);
        m_pEvokedSetModel->setAverageColor(pqMapAverageColor);

        QMap<QString, bool> qMapAverageActivation;
        settings.beginGroup(QString("RTESW/%1/averageActivationMap").arg(t_sRTESName));
        keys = settings.childKeys();
        foreach (QString key, keys) {
             qMapAverageActivation[key] = settings.value(key).toBool();
        }
        settings.endGroup();
        QSharedPointer<QMap<QString, bool> > pqMapAverageActivation = QSharedPointer<QMap<QString, bool> >::create(qMapAverageActivation);
        m_pEvokedSetModel->setAverageActivation(pqMapAverageActivation); 

        //Init channel selection manager
        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = QSharedPointer<ChannelSelectionView>::create(QString("RTESW/%1").arg(t_sRTESName),
                                                                               this,
                                                                               m_pChannelInfoModel,
                                                                               Qt::Window);

        //Connect channel info model
        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);

        connect(m_pChannelInfoModel.data(), &ChannelInfoModel::channelsMappedToLayout,
                m_pChannelSelectionView.data(), &ChannelSelectionView::setCurrentlyMappedFiffChannels);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::showSelectedChannelsOnly,
                m_pButterflyView.data(), &ButterflyView::showSelectedChannelsOnly);

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::selectionChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::channelSelectionManagerChanged);

        m_pChannelInfoModel->fiffInfoChanged(m_pFiffInfo);

        // Quick control scaling        
        ScalingView* pScalingView = new ScalingView(QString("RTESW/%1").arg(t_sRTESName),
                                                    m_pFiffInfo->chs);
        m_pQuickControlView->addGroupBox(pScalingView, "Scaling");

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pButterflyView.data(), &ButterflyView::setScaleMap);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setScaleMap);

        // Quick control projectors
        ProjectorsView* pProjectorsView = new ProjectorsView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pProjectorsView, "Noise", "SSP");

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateProjection);

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        m_pEvokedSetModel->updateProjection(pProjectorsView->getProjectors());

        // Quick control compensators
        CompensatorView* pCompensatorView = new CompensatorView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pCompensatorView, "Noise", "Comp");

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::updateCompensator);

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                m_pButterflyView.data(), &ButterflyView::updateView);

        m_pEvokedSetModel->updateCompensator(pCompensatorView->getLastTo());

        // Quick control filter settings
        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("RTESW/%1").arg(t_sRTESName));
        m_pQuickControlView->addGroupBoxWithTabs(pFilterSettingsView, "Noise", "Filter");

        connect(pFilterSettingsView->getFilterView().data(), &FilterView::filterChannelTypeChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setFilterChannelType);

        connect(pFilterSettingsView->getFilterView().data(), &FilterView::filterChanged,
                m_pEvokedSetModel.data(), &EvokedSetModel::setFilter);

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
        ChannelDataSettingsView* pChannelDataSettingsView = new ChannelDataSettingsView();
        pChannelDataSettingsView->init(QStringList() << "screenshot" << "backgroundColor");
        m_pQuickControlView->addGroupBoxWithTabs(pChannelDataSettingsView, "Other", "View");

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::backgroundColorChanged,
                m_pAverageLayoutView.data(), &AverageLayoutView::setBackgroundColor);

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::backgroundColorChanged,
                m_pButterflyView.data(), &ButterflyView::setBackgroundColor);

        connect(pChannelDataSettingsView, &ChannelDataSettingsView::makeScreenshot,
                this, &RealTimeEvokedSetWidget::onMakeScreenshot);

        QColor backgroundDefault = Qt::black;
        pChannelDataSettingsView->setSignalBackgroundColors(QColor(),
                                                            settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());

        // Quick control modality selection
        ModalitySelectionView* pModalitySelectionView = new ModalitySelectionView(QString("RTESW/%1").arg(t_sRTESName),
                                                                                  m_pFiffInfo->chs);
        m_pQuickControlView->addGroupBoxWithTabs(pModalitySelectionView, "Other", "Modalities");

        connect(pModalitySelectionView, &ModalitySelectionView::modalitiesChanged,
                m_pButterflyView.data(), &ButterflyView::setModalityMap);

        // Quick control average selection
        AverageSelectionView* pAverageSelectionView = new AverageSelectionView();
        pAverageSelectionView->setAverageColor(pqMapAverageColor);
        pAverageSelectionView->setAverageActivation(pqMapAverageActivation);
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

        // View settings
        m_pButterflyView->setModel(m_pEvokedSetModel);
        m_pButterflyView->setAverageActivation(pqMapAverageActivation);
        m_pButterflyView->setAverageColor(pqMapAverageColor);
        m_pButterflyView->setModel(m_pChannelInfoModel);
        m_pButterflyView->setScaleMap(pScalingView->getScaleMap());
        m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());
        m_pButterflyView->setBackgroundColor(settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());

        // Call this function before the layout calls below so that the scene items are drawn first
        m_pChannelSelectionView->updateDataView();

        m_pAverageLayoutView->setModel(m_pEvokedSetModel);
        m_pAverageLayoutView->setAverageActivation(pqMapAverageActivation);
        m_pAverageLayoutView->setAverageColor(pqMapAverageColor);
        m_pAverageLayoutView->setModel(m_pChannelInfoModel);
        m_pAverageLayoutView->setScaleMap(pScalingView->getScaleMap());
        m_pAverageLayoutView->setBackgroundColor(settings.value(QString("RTESW/%1/backgroundColor").arg(t_sRTESName), backgroundDefault).value<QColor>());

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

