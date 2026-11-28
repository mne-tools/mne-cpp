//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     realtimeevokedsetwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2018
 * @brief    Definition of the RealTimeEvokedSetWidget Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "realtimeevokedsetwidget.h"

#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/channel_selection_panel.h>
#include <mne/viz/backends/qt/viewers/helpers/channelinfomodel.h>
#include <mne/viz/backends/qt/viewers/helpers/evokedsetmodel.h>
#include <mne/viz/backends/qt/viewers/butterflyview.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/backends/qt/kit/sceneviewport.h>
#include <mne/viz/backends/qt/qtconversions.h>
#include <mne/viz/backends/qt/viewers/helpers/selectionitem.h>
#include <mne/viz/figures/svg_renderer.h>
#include <mne/viz/panels/average_layout_panel.h>
#include <mne/base/log.h>
#include <mne/viz/backends/qt/viewers/scalingview.h>
#include <mne/viz/backends/qt/viewers/modalityselectionview.h>
#include <mne/viz/backends/qt/viewers/fiffrawviewsettings.h>
#include <mne/viz/backends/qt/viewers/averageselectionview.h>
#include <scMeas/realtimeevokedset.h>
#include <mne/dsp/filterkernel.h>

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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSetWidget::RealTimeEvokedSetWidget(QSharedPointer<QTime>& pTime,
                                                 QWidget* parent)
: MeasurementWidget(parent)
, m_iMaxFilterTapSize(0)
{
    Q_UNUSED(pTime)

    QAction* pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the channel selection window"), this);
    pActionSelectSensors->setStatusTip(tr("Show the channel selection view"));
    connect(pActionSelectSensors, &QAction::triggered,
            this, &RealTimeEvokedSetWidget::showSensorSelectionWidget);

    //Create GUI
    m_pRTESetLayout = new QVBoxLayout(this);

    //Set acquire label
    m_pLabelInit = new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setBold(true);
    font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRTESetLayout->addWidget(m_pLabelInit);
    m_pRTESetLayout->setContentsMargins(0, 0, 0, 0);

    //Create toolboxes with butterfly and 2D layout plot
    m_pToolBox = new QToolBox(this);
    m_pToolBox->hide();

    //Butterfly
    m_pButterflyView = new ButterflyView("MNESCAN/RTESW", this);
    m_pButterflyView->installEventFilter(this);

    //2D layout plot
    m_pAverageLayout = std::make_shared<viz::AverageLayoutPanel>();
    m_pAverageLayoutView = new PanelWidget(m_pAverageLayout, "MNESCAN/RTESW", this);
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
    // m_pRTESet is only assigned once a measurement arrives through update, so
    // a widget destroyed before its plugin produced anything still has it null.
    // That happens whenever a scene is torn down shortly after being built.
    if (m_pRTESet && !m_pRTESet->getName().isEmpty()) {
        QSettings settings("MNECPP");

        //Store current view toolbox index - butterfly or 2D layout
        if (m_pToolBox) {
            settings.setValue(QString("MNESCAN/RTESW/selectedView"), m_pToolBox->currentIndex());
        }
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (!m_pRTESet) {
        m_pRTESet = qSharedPointerDynamicCast<RealTimeEvokedSet>(pMeasurement);
    }

    if (m_pRTESet) {
        if (!m_pFiffInfo) {
            m_pFiffInfo = m_pRTESet->info();

            if (!m_bDisplayWidgetsInitialized) {
                initDisplayControllWidgets();
            }
        }

        if (m_pRTESet->isInitialized()) {
            //Check if block size has changed, if yes update the filter
            if (!m_pRTESet->getValue()->evoked.empty()) {
                if (m_iMaxFilterTapSize != m_pRTESet->getValue()->evoked.front().data.cols()) {
                    m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.front().data.cols();

                    emit windowSizeChanged(m_iMaxFilterTapSize);
                }
            }

            std::shared_ptr<FiffEvokedSet> pEvokedSet = m_pRTESet->getValue();
            pEvokedSet->info = *(m_pFiffInfo.get());
            m_pEvokedSetModel->setEvokedSet(pEvokedSet);
        }
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::initDisplayControllWidgets()
{
    if (m_pFiffInfo) {
        QSettings settings("MNECPP");
        QString t_sRTESName = m_pRTESet->getName();

        //Initialize leftover scalars to default values
        if (!m_pRTESet->getValue()->evoked.empty()) {
            m_iMaxFilterTapSize = m_pRTESet->getValue()->evoked.front().data.cols();
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
        m_pEvokedSetModel = std::make_shared<EvokedSetModel>(this);
        std::shared_ptr<FiffEvokedSet> pEvokedSet = m_pRTESet->getValue();
        pEvokedSet->info = *m_pFiffInfo.get();
        m_pEvokedSetModel->setEvokedSet(pEvokedSet);
        m_pButterflyView->setEvokedSetModel(m_pEvokedSetModel);

        //Init channel info and selection view
        m_pChannelInfoModel = std::make_shared<ChannelInfoModel>(m_pFiffInfo,
                                                                 this);

        // The panel reports to the views and the channel info model before its window restores the stored files
        m_pChannelSelection = std::make_shared<viz::ChannelSelectionPanel>();
        m_pChannelSelection->set_channel_table(ChannelInfoModel::sharedTable(m_pChannelInfoModel));
        viz::ChannelSelectionPanel::Callbacks callbacks;
        callbacks.layout_loaded = [this](const std::map<std::string, viz::Point>& layout) {
            m_pChannelInfoModel->setLayout(layout);
        };
        callbacks.channels_shown = [this](const std::vector<std::string>& channels) {
            m_pButterflyView->showSelectedChannelsOnly(channels);
        };
        callbacks.selection_changed = [this](const std::vector<viz::SensorLayout::Sensor>& sensors) {
            m_pAverageLayout->set_evoked(m_pEvokedSetModel->evokedAverages());
            m_pAverageLayout->set_selection(SelectionItem::fromSensors(sensors).averageChannels());
        };
        m_pChannelSelection->set_callbacks(std::move(callbacks));
        m_pChannelSelectionView = std::make_shared<PanelWidget>(m_pChannelSelection, QString("MNESCAN/RTESW"), this);
        m_pChannelSelectionView->setWindowFlag(Qt::Window);
        m_pChannelSelectionView->setWindowTitle(tr(QString("%1: Channel Selection Window").arg(t_sRTESName).toUtf8()));

        connect(m_pChannelInfoModel.get(), &ChannelInfoModel::channelsMappedToLayout,
                this, [this](const std::vector<std::string>& names) {
                    m_pChannelSelection->set_mapped_channels(names);
                });

        // Init the views. This needs to be done in the following order because we need to draw the items in the m_pChannelSelectionView first, hence we need to init it first.
        m_pButterflyView->setChannelInfoModel(m_pChannelInfoModel);
        m_pChannelSelection->update_data_view();
        // The selection names the channels by layout name; the channel info model knows their rows
        m_pAverageLayout->set_info(m_pFiffInfo);
        m_pAverageLayout->set_channel_lookup([model = m_pChannelInfoModel](const std::string& name) {
            return model->getIndexFromMappedChName(name);
        });
        connect(m_pEvokedSetModel.get(), &EvokedSetModel::dataChanged,
                this, [this]() {
                    m_pAverageLayout->set_evoked(m_pEvokedSetModel->evokedAverages());
                });
        m_pAverageLayout->set_evoked(m_pEvokedSetModel->evokedAverages());
        m_pChannelInfoModel->setLayout(m_pChannelSelection->layout_map());

        //Init control widgets
        QList<QWidget*> lControlWidgets;

        // Scaling
        ScalingView* pScalingView = new ScalingView(QString("MNESCAN/RTESW"), this, Qt::Widget, m_pFiffInfo->get_channel_types());
        pScalingView->setObjectName("group_tab_View_Scaling");
        lControlWidgets.append(pScalingView);

        connect(pScalingView, &ScalingView::scalingChanged,
                m_pButterflyView.data(), &ButterflyView::setScaleMap);

        connect(pScalingView, &ScalingView::scalingChanged,
                this, [this](const std::map<int, float>& scaleMap) {
                    m_pAverageLayout->set_scale_map(scaleMap);
                });

        m_pButterflyView->setScaleMap(pScalingView->getScaleMap());
        m_pAverageLayout->set_scale_map(pScalingView->getScaleMap());

        // Quick control channel data settings
        FiffRawViewSettings* pChannelDataSettingsView = new FiffRawViewSettings(QString("MNESCAN/RTESW"), this);
        pChannelDataSettingsView->setWidgetList({"screenshot", "backgroundColor"});
        pChannelDataSettingsView->setObjectName("group_tab_View_General");
        lControlWidgets.append(pChannelDataSettingsView);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::backgroundColorChanged,
                m_pAverageLayoutView->figureViewport(), &SceneViewport::setBackgroundColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::backgroundColorChanged,
                m_pButterflyView.data(), &ButterflyView::setBackgroundColor);

        connect(pChannelDataSettingsView, &FiffRawViewSettings::makeScreenshot,
                this, &RealTimeEvokedSetWidget::onMakeScreenshot);

        m_pAverageLayoutView->figureViewport()->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());
        m_pButterflyView->setBackgroundColor(pChannelDataSettingsView->getBackgroundColor());

        // Quick control modality selection
        ModalitySelectionView* pModalitySelectionView = new ModalitySelectionView(m_pFiffInfo->chs, QString("MNESCAN/RTESW"), this);
        pModalitySelectionView->setObjectName("group_tab_View_Modalities");
        lControlWidgets.append(pModalitySelectionView);

        connect(pModalitySelectionView, &ModalitySelectionView::modalitiesChanged,
                m_pButterflyView.data(), &ButterflyView::setModalityMap);

        m_pButterflyView->setModalityMap(pModalitySelectionView->getModalityMap());

        // Quick control average selection
        AverageSelectionView* pAverageSelectionView = new AverageSelectionView(QString("MNESCAN/RTESW"), this);
        pAverageSelectionView->setObjectName("group_tab_View_Selection");
        lControlWidgets.append(pAverageSelectionView);

        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageActivationMap,
                pAverageSelectionView, &AverageSelectionView::setAverageActivation);
        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageColorMap,
                pAverageSelectionView, &AverageSelectionView::setAverageColor);

        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageColorMap,
                m_pButterflyView.data(), &ButterflyView::setAverageColor);
        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageActivationMap,
                m_pButterflyView.data(), &ButterflyView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageActivationMap,
                m_pButterflyView.data(), &ButterflyView::setAverageActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageColorMap,
                m_pButterflyView.data(), &ButterflyView::setAverageColor);

        const auto setLayoutColors = [this](const std::shared_ptr<std::map<std::string, QColor>>& colors) {
            m_pAverageLayout->set_average_colors(colors ? toColors(*colors) : std::map<std::string, viz::Color>{});
        };
        const auto setLayoutActivation = [this](const std::shared_ptr<std::map<std::string, bool>>& activation) {
            m_pAverageLayout->set_average_activation(activation ? *activation : std::map<std::string, bool>{});
        };
        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageColorMap, this, setLayoutColors);
        connect(m_pEvokedSetModel.get(), &EvokedSetModel::newAverageActivationMap, this, setLayoutActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageActivationMap, this, setLayoutActivation);
        connect(pAverageSelectionView, &AverageSelectionView::newAverageColorMap, this, setLayoutColors);

        m_pEvokedSetModel->setAverageActivation(pAverageSelectionView->getAverageActivation());
        m_pEvokedSetModel->setAverageColor(pAverageSelectionView->getAverageColor());
        m_pButterflyView->setAverageActivation(pAverageSelectionView->getAverageActivation());
        m_pButterflyView->setAverageColor(pAverageSelectionView->getAverageColor());
        setLayoutActivation(pAverageSelectionView->getAverageActivation());
        setLayoutColors(pAverageSelectionView->getAverageColor());

        emit displayControlWidgetsChanged(lControlWidgets, t_sRTESName);

        //Initialized
        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::updateViewport()
{
    if (m_pAverageLayoutView) {
        m_pAverageLayoutView->figureViewport()->update();
    }
    if (m_pButterflyView) {
        m_pButterflyView->updateViewport();
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::showSensorSelectionWidget()
{
    // The window exists once the first evoked set arrived
    if (m_pChannelSelectionView) {
        m_pChannelSelectionView->show();
    }
}

//=============================================================================================================

void RealTimeEvokedSetWidget::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if (!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    //
    // Only PNG and SVG can be written. Anything else would leave the file name
    // empty and be handed on to a view that then writes nothing.
    //
    QString sSuffix;
    if (imageType.contains("SVG")) {
        sSuffix = "svg";
    } else if (imageType.contains("PNG")) {
        sSuffix = "png";
    } else {
        Log::warning("[RealTimeEvokedSetWidget::onMakeScreenshot] Unsupported image type {} - no screenshot was taken.", imageType.toStdString());
        return;
    }

    //
    // Capture the plot the user is actually looking at. The 2D layout branch
    // built a Layout file name but still screenshotted the butterfly view.
    //
    const bool bIsLayout = m_pToolBox->itemText(m_pToolBox->currentIndex()) == "2D Layout plot";

    const QString fileName = QString("./Screenshots/%1-%2-%3Screenshot.%4")
                                 .arg(sDate,
                                      sTime,
                                      bIsLayout ? "Layout" : "Butterfly",
                                      sSuffix);

    if (bIsLayout) {
        SceneViewport* viewport = m_pAverageLayoutView->figureViewport();
        if (fileName.endsWith(".svg", Qt::CaseInsensitive)) {
            if (const auto written = viz::write_svg(viewport->picture(viewport->size()), fileName.toStdU16String()); !written) {
                mne::Log::warning("{}", written.error().what());
            }
        } else {
            viewport->grab().save(fileName);
        }
    } else {
        m_pButterflyView->takeScreenshot(fileName);
    }
}

//=============================================================================================================

bool RealTimeEvokedSetWidget::eventFilter(QObject* object, QEvent* event)
{
    if ((object == m_pButterflyView || object == m_pAverageLayoutView) && event->type() == QEvent::MouseButtonDblClick) {
        m_pEvokedSetModel->toggleFreeze();
    }
    return false;
}
