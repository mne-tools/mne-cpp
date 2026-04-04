//=============================================================================================================
/**
 * @file     averagewindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the AverageWindow class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagewindow.h"

#include <disp/viewers/helpers/averagesceneitem.h>

#include <cmath>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

namespace
{

bool matchesButterflySelection(const FIFFLIB::FiffChInfo& channelInfo,
                               const QString& selectionText)
{
    if(selectionText == QLatin1String("EEG")) {
        return channelInfo.kind == FIFFV_EEG_CH;
    }

    if(channelInfo.kind != FIFFV_MEG_CH) {
        return false;
    }

    if(selectionText == QLatin1String("MEG_mag")) {
        return channelInfo.unit == FIFF_UNIT_T;
    }

    return channelInfo.unit == FIFF_UNIT_T_M;
}

QStringList selectedButterflyChannels(const FIFFLIB::FiffInfo& info,
                                      const QString& selectionText)
{
    QStringList channelNames;

    for(const auto& channelInfo : info.chs) {
        if(info.bads.contains(channelInfo.ch_name)) {
            continue;
        }

        if(matchesButterflySelection(channelInfo, selectionText)) {
            channelNames << channelInfo.ch_name;
        }
    }

    return channelNames;
}

Eigen::MatrixXd createWhitener(const FIFFLIB::FiffCov& covariance)
{
    if(covariance.dim <= 0) {
        return Eigen::MatrixXd();
    }

    if(covariance.diag) {
        Eigen::MatrixXd whitener = Eigen::MatrixXd::Zero(covariance.dim, covariance.dim);
        for(int i = 0; i < covariance.dim; ++i) {
            const double variance = covariance.data(i, 0);
            if(variance > 1e-30) {
                whitener(i, i) = 1.0 / std::sqrt(variance);
            }
        }

        return whitener;
    }

    if(covariance.eig.size() == 0 || covariance.eigvec.size() == 0) {
        return Eigen::MatrixXd();
    }

    Eigen::MatrixXd whitener = Eigen::MatrixXd::Zero(covariance.dim, covariance.dim);
    for(int i = 0; i < covariance.eig.size(); ++i) {
        if(covariance.eig(i) > 1e-30) {
            whitener(i, i) = 1.0 / std::sqrt(covariance.eig(i));
        }
    }

    whitener *= covariance.eigvec;
    return whitener;
}

bool whitenButterflyEvoked(FIFFLIB::FiffEvoked& evoked,
                           const FIFFLIB::FiffCov& covariance,
                           const WhiteningSettings& settings)
{
    if(covariance.isEmpty() || evoked.isEmpty() || evoked.data.rows() == 0) {
        return false;
    }

    const FIFFLIB::FiffCov regularizedCov =
        covariance.regularize(evoked.info,
                              settings.regMag,
                              settings.regGrad,
                              settings.regEeg,
                              settings.useProj);

    const FIFFLIB::FiffCov preparedCov =
        regularizedCov.prepare_noise_cov(evoked.info, evoked.info.ch_names);

    if(preparedCov.isEmpty() || preparedCov.dim != evoked.data.rows()) {
        return false;
    }

    const Eigen::MatrixXd whitener = createWhitener(preparedCov);
    if(whitener.rows() != evoked.data.rows() || whitener.cols() != evoked.data.rows()) {
        return false;
    }

    evoked.data = whitener * evoked.data;
    return true;
}

FIFFLIB::FiffEvoked buildButterflyEvoked(const FIFFLIB::FiffEvoked& sourceEvoked,
                                         const QString& selectionText,
                                         const FIFFLIB::FiffCov& covariance,
                                         const WhiteningSettings& settings,
                                         bool whiten,
                                         bool* whiteningApplied)
{
    const QStringList includeChannels = selectedButterflyChannels(sourceEvoked.info, selectionText);

    FIFFLIB::FiffEvoked displayEvoked =
        sourceEvoked.pick_channels(includeChannels, sourceEvoked.info.bads);

    bool didApplyWhitening = false;
    if(whiten) {
        didApplyWhitening = whitenButterflyEvoked(displayEvoked, covariance, settings);
    }

    if(whiteningApplied) {
        *whiteningApplied = didApplyWhitening;
    }

    return displayEvoked;
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageWindow::AverageWindow(QWidget *parent, QFile &file)
: QDockWidget(parent)
, ui(new Ui::AverageWindow)
{
    ui->setupUi(this);
    initMVC(file);
    init();
}


//*************************************************************************************************************

AverageWindow::AverageWindow(QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::AverageWindow)
{
    ui->setupUi(this);
    initMVC();
    init();
}


//*************************************************************************************************************

//AverageWindow::AverageWindow()
//: AverageWindow(0)
//{
//}


//*************************************************************************************************************

AverageWindow::~AverageWindow()
{
}


//*************************************************************************************************************

AverageModel* AverageWindow::getAverageModel()
{
    return m_pAverageModel;
}


//*************************************************************************************************************

void AverageWindow::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    //Repaint the average items in the average scene based on the input parameter
    m_pAverageScene->repaintItems(selectedChannelItems);

    //call the onSelection function manually to replot the data for the givven average items
    onSelectionChanged(ui->m_tableView_loadedSets->selectionModel()->selection(), QItemSelection());

    //fit everything in the view and update the scene
    ui->m_graphicsView_layout->fitInView(m_pAverageScene->sceneRect(), Qt::KeepAspectRatio);
    m_pAverageScene->update(m_pAverageScene->sceneRect());
}


//*************************************************************************************************************

void AverageWindow::scaleAveragedData(const QMap<QString,double> &scaleMap)
{
    //Set the scale map received from the scale window
    QMap<qint32,float> newScaleMapIdx;

    newScaleMapIdx[FIFF_UNIT_T_M] = scaleMap["MEG_grad"];
    newScaleMapIdx[FIFF_UNIT_T] = scaleMap["MEG_mag"];
    newScaleMapIdx[FIFFV_REF_MEG_CH] = scaleMap["MEG_mag"];
    newScaleMapIdx[FIFFV_EEG_CH] = scaleMap["MEG_EEG"];
    newScaleMapIdx[FIFFV_EOG_CH] = scaleMap["MEG_EOG"];
    newScaleMapIdx[FIFFV_EMG_CH] = scaleMap["MEG_EMG"];
    newScaleMapIdx[FIFFV_STIM_CH] = scaleMap["MEG_STIM"];
    newScaleMapIdx[FIFFV_MISC_CH] = scaleMap["MEG_MISC"];

    m_pAverageScene->setScaleMap(newScaleMapIdx);
    m_pButterflyScene->setScaleMap(scaleMap);

    // Sync spinboxes with external scale changes
    if(m_pSpinGrad) m_pSpinGrad->setValue(scaleMap.value("MEG_grad", 1e-10) / 1e-10);
    if(m_pSpinMag)  m_pSpinMag->setValue(scaleMap.value("MEG_mag", 1e-11) / 1e-11);
    if(m_pSpinEEG)  m_pSpinEEG->setValue(scaleMap.value("MEG_EEG", 1e-4) / 1e-4);
}


//*************************************************************************************************************

void AverageWindow::setMappedChannelNames(QStringList mappedChannelNames)
{
    m_mappedChannelNames = mappedChannelNames;
}


//*************************************************************************************************************

void AverageWindow::setNoiseCovariance(const FIFFLIB::FiffCov& covariance)
{
    m_noiseCovariance = covariance;
    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::clearNoiseCovariance()
{
    m_noiseCovariance.clear();
    m_whiteningSettings.enableButterfly = false;
    m_whiteningSettings.enableLayout = false;
    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::setWhiteningSettings(const WhiteningSettings& settings)
{
    const bool layoutChanged = (settings.enableLayout != m_whiteningSettings.enableLayout);
    const bool butterflyChanged = (settings.enableButterfly != m_whiteningSettings.enableButterfly);
    const bool regChanged = (settings.regMag != m_whiteningSettings.regMag ||
                             settings.regGrad != m_whiteningSettings.regGrad ||
                             settings.regEeg != m_whiteningSettings.regEeg ||
                             settings.useProj != m_whiteningSettings.useProj);
    m_whiteningSettings = settings;
    if(layoutChanged || butterflyChanged || regChanged) {
        m_bAutoScaled = false;  // re-compute scale with new whitened amplitudes
    }
    refreshPlots();
}


//*************************************************************************************************************

WhiteningSettings AverageWindow::whiteningSettings() const
{
    return m_whiteningSettings;
}


//*************************************************************************************************************

void AverageWindow::setButterflyWhiteningEnabled(bool enabled)
{
    if(enabled && m_noiseCovariance.isEmpty()) {
        return;
    }

    m_whiteningSettings.enableButterfly = enabled;
    m_bAutoScaled = false;  // re-compute scale with whitened amplitudes
    refreshPlots();
}


//*************************************************************************************************************

bool AverageWindow::isButterflyWhiteningEnabled() const
{
    return m_whiteningSettings.enableButterfly;
}

//*************************************************************************************************************

QList<int> AverageWindow::selectedSetRows() const
{
    QList<int> rows;

    if(!ui->m_tableView_loadedSets->selectionModel()) {
        return rows;
    }

    const QModelIndexList selectedRows =
        ui->m_tableView_loadedSets->selectionModel()->selectedRows(0);

    rows.reserve(selectedRows.size());
    for(const QModelIndex& index : selectedRows) {
        rows.append(index.row());
    }

    return rows;
}

//*************************************************************************************************************

void AverageWindow::setRecomputeAvailable(bool available)
{
    if(m_pRecomputeAverageAction) {
        m_pRecomputeAverageAction->setEnabled(available);
    }
}


//*************************************************************************************************************

void AverageWindow::init()
{
    initAverageSceneView();
    initTableViewWidgets();
    initButtons();
    initComboBoxes();

    // Allow the dock widget to be resized freely when docked
    setMinimumHeight(100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Re-fit views when switching tabs
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        QTimer::singleShot(0, this, [this]() {
            updateButterflySize();
            if(m_pAverageScene && !m_pAverageScene->items().isEmpty()) {
                ui->m_graphicsView_layout->fitInView(m_pAverageScene->itemsBoundingRect(), Qt::KeepAspectRatio);
            }
        });
    });
}


//*************************************************************************************************************

void AverageWindow::initMVC(QFile &file)
{
    //Setup average model
    if(file.exists())
        m_pAverageModel = new AverageModel(file, this);
    else
        m_pAverageModel = new AverageModel(this);

    //Setup average delegate
    m_pAverageDelegate = new AverageDelegate(this);
}


//*************************************************************************************************************

void AverageWindow::initMVC()
{
    m_pAverageModel = new AverageModel(this);

    //Setup average delegate
    m_pAverageDelegate = new AverageDelegate(this);
}


//*************************************************************************************************************

void AverageWindow::initTableViewWidgets()
{
    //Set average model to list widget
    ui->m_tableView_loadedSets->setModel(m_pAverageModel);
    ui->m_tableView_loadedSets->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->m_tableView_loadedSets->setColumnHidden(5,true); //hide last column because the average model holds internal data handles here
    ui->m_tableView_loadedSets->resizeColumnsToContents();

    ui->m_tableView_loadedSets->adjustSize();

    //Connect selection of the loaded evoked files
    connect(ui->m_tableView_loadedSets->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AverageWindow::onSelectionChanged);

    connect(m_pAverageModel, &AverageModel::fileLoaded,
            this, [this](bool loaded) {
        if(loaded) {
            ui->m_tableView_loadedSets->resizeColumnsToContents();
            m_pAverageScene->clear();
            m_bAutoScaled = false;
            selectLoadedSets();
            refreshPlots();
        } else {
            m_pAverageScene->update();
            m_pButterflyScene->clear();
            m_pButterflyScene->update();
        }
    });

    selectLoadedSets();
}


//*************************************************************************************************************

void AverageWindow::initAverageSceneView()
{
    //Create average scene and set view
    m_pAverageScene = new AverageScene(ui->m_graphicsView_layout, this);
    ui->m_graphicsView_layout->setScene(m_pAverageScene);

    // Set sensible default scale map so traces are visible before the user
    // opens the Scale window.  The raw defaults in getDefaultScalingValue
    // (MAG=1e-12, GRAD=1e-15) are the unit amplitude, producing extreme
    // dScaleY values.  Here we use the same multiples that ScalingView
    // applies to its default slider positions.
    QMap<qint32,float> defaultScaleMap;
    defaultScaleMap[FIFF_UNIT_T]   = 1e-11f;   // MAG
    defaultScaleMap[FIFF_UNIT_T_M] = 1e-10f;   // GRAD
    defaultScaleMap[FIFFV_EEG_CH]  = 1e-4f;    // EEG
    m_pAverageScene->setScaleMap(defaultScaleMap);

    // --- Scale controls for the 2D Layout tab ---
    // The graphics view sits in gridLayout (inside gridLayout_2 on the tab page).
    // We add scale spinboxes to gridLayout row 2.
    QGridLayout* layoutGrid = nullptr;
    if(auto* item = ui->m_graphicsView_layout->parentWidget()->layout()) {
        // gridLayout_2 wraps gridLayout; find the inner one containing the view
        for(int i = 0; i < item->count(); ++i) {
            if(auto* nested = qobject_cast<QGridLayout*>(item->itemAt(i)->layout())) {
                // Check if this grid contains our graphics view
                for(int j = 0; j < nested->count(); ++j) {
                    if(nested->itemAt(j)->widget() == ui->m_graphicsView_layout) {
                        layoutGrid = nested;
                        break;
                    }
                }
                if(layoutGrid) break;
            }
        }
    }
    if(layoutGrid) {
        QWidget* tabPage = ui->m_graphicsView_layout->parentWidget();

        auto makeSpinBox = [&](const QString& labelText, double defaultVal) -> QDoubleSpinBox* {
            QDoubleSpinBox* spin = new QDoubleSpinBox(tabPage);
            spin->setDecimals(2);
            spin->setRange(0.01, 100.0);
            spin->setSingleStep(0.1);
            spin->setValue(defaultVal);
            spin->setPrefix(labelText + " ×");
            spin->setFixedWidth(120);
            return spin;
        };

        m_pSpinGrad = makeSpinBox("Grad", 1.0);
        m_pSpinMag  = makeSpinBox("Mag", 1.0);
        m_pSpinEEG  = makeSpinBox("EEG", 1.0);

        QHBoxLayout* scaleLayout = new QHBoxLayout;
        scaleLayout->addStretch();
        scaleLayout->addWidget(m_pSpinGrad);
        scaleLayout->addWidget(m_pSpinMag);
        scaleLayout->addWidget(m_pSpinEEG);

        layoutGrid->addLayout(scaleLayout, 2, 0, 1, 3);

        auto updateScales = [this]() {
            QMap<qint32,float> scaleMap;
            scaleMap[FIFF_UNIT_T]   = static_cast<float>(m_pSpinMag->value()  * 1e-11);
            scaleMap[FIFF_UNIT_T_M] = static_cast<float>(m_pSpinGrad->value() * 1e-10);
            scaleMap[FIFFV_EEG_CH]  = static_cast<float>(m_pSpinEEG->value()  * 1e-4);
            m_pAverageScene->setScaleMap(scaleMap);
        };

        connect(m_pSpinGrad, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, updateScales);
        connect(m_pSpinMag, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, updateScales);
        connect(m_pSpinEEG, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, updateScales);
    }

    //Create butterfly average scene and set view
    m_pButterflyScene = new ButterflyScene(ui->m_graphicsView_butterflyPlot, this);
    ui->m_graphicsView_butterflyPlot->setScene(m_pButterflyScene);
    ui->m_graphicsView_butterflyPlot->setMouseTracking(true);
    ui->m_graphicsView_butterflyPlot->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->m_graphicsView_butterflyPlot->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //Generate random colors for plotting
    for(int i = 0; i<500; i++)
        m_lButterflyColors.append(QColor(QRandomGenerator::global()->generate()%256, QRandomGenerator::global()->generate()%256, QRandomGenerator::global()->generate()%256));
}


//*************************************************************************************************************

void AverageWindow::initButtons()
{
    m_pComputeAverageAction = new QAction(tr("Compute evoked..."), this);
    connect(m_pComputeAverageAction, &QAction::triggered,
            this, &AverageWindow::addAverageRequested);

    m_pRecomputeAverageAction = new QAction(tr("Recompute last settings"), this);
    m_pRecomputeAverageAction->setEnabled(false);
    connect(m_pRecomputeAverageAction, &QAction::triggered,
            this, &AverageWindow::recomputeAverageRequested);

    QMenu* averageMenu = new QMenu(ui->m_toolbutton_addAverage);
    averageMenu->addAction(m_pComputeAverageAction);
    averageMenu->addAction(m_pRecomputeAverageAction);

    ui->m_toolbutton_addAverage->setMenu(averageMenu);
    ui->m_toolbutton_addAverage->setPopupMode(QToolButton::MenuButtonPopup);
    ui->m_toolbutton_addAverage->setText("+");
    ui->m_toolbutton_addAverage->setToolTip("Compute evoked responses from the current raw file, or rerun the last evoked setup.");
    connect(ui->m_toolbutton_addAverage, &QToolButton::clicked,
            this, &AverageWindow::addAverageRequested);

    connect(ui->m_pushButton_exportLayoutPlot, &QPushButton::released,
            this, &AverageWindow::exportAverageLayoutPlot);

    connect(ui->m_pushButton_exportButterflyPlot, &QPushButton::released,
            this, &AverageWindow::exportAverageButterflyPlot);
}


//*************************************************************************************************************

void AverageWindow::initComboBoxes()
{
    // Butterfly tab combo
    connect(ui->m_comboBox_channelKind, &QComboBox::currentTextChanged,
            this, [this](){
        refreshPlots();
    });

    // 2D Layout tab combo — clear scene so it re-populates with the new channel type
    connect(ui->m_comboBox_layoutChannelKind, &QComboBox::currentTextChanged,
            this, [this](){
        m_pAverageScene->clear();
        refreshPlots();
    });
}


//*************************************************************************************************************

void AverageWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    Q_UNUSED(selected);

    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::refreshPlots()
{
    QModelIndexList selectedRows;
    if(ui->m_tableView_loadedSets->selectionModel()) {
        selectedRows = ui->m_tableView_loadedSets->selectionModel()->selectedRows(0);
    }

    m_layoutDisplayEvokeds.clear();
    m_layoutDisplayEvokeds.reserve(selectedRows.size());

    for(int rowIndex = 0; rowIndex < selectedRows.size(); ++rowIndex) {
        const QModelIndex index = selectedRows.at(rowIndex);
        const FIFFLIB::FiffEvoked* sourceEvoked = m_pAverageModel->getEvoked(index.row());

        if(!sourceEvoked) {
            m_layoutDisplayEvokeds.append(FIFFLIB::FiffEvoked());
            continue;
        }

        FIFFLIB::FiffEvoked displayEvoked(*sourceEvoked);
        if(m_whiteningSettings.enableLayout) {
            whitenButterflyEvoked(displayEvoked, m_noiseCovariance, m_whiteningSettings);
        }

        m_layoutDisplayEvokeds.append(displayEvoked);
    }

    // Auto-populate layout scene if empty but evoked data is available
    if(m_pAverageScene->items().isEmpty() && !selectedRows.isEmpty()) {
        populateLayoutFromEvoked(selectedRows);
    }

    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Do for all selected evoked sets
        for(int u = 0; u < selectedRows.size(); ++u) {
            if(u >= m_layoutDisplayEvokeds.size()) {
                continue;
            }

            const QModelIndex index = selectedRows.at(u);
            const FIFFLIB::FiffEvoked& displayEvoked = m_layoutDisplayEvokeds.at(u);
            if(displayEvoked.isEmpty() || displayEvoked.data.rows() == 0) {
                continue;
            }

            const FiffInfo* fiffInfo = &displayEvoked.info;
            RowVectorPair averageData;
            averageData.first = displayEvoked.data.data();
            averageData.second = displayEvoked.data.cols();
            const int first = displayEvoked.first;
            const int last = displayEvoked.last;

            //Get the averageScenItem specific data row
            int channelNumber = fiffInfo->ch_names.indexOf(averageSceneItemTemp->m_sChannelName);

            if(channelNumber != -1) {
                averageSceneItemTemp->m_firstLastSample.first = first;
                averageSceneItemTemp->m_firstLastSample.second = last;
                averageSceneItemTemp->m_iChannelKind = fiffInfo->chs.at(channelNumber).kind;
                averageSceneItemTemp->m_iChannelUnit = fiffInfo->chs.at(channelNumber).unit;;
                averageSceneItemTemp->m_iChannelNumber = channelNumber;
                averageSceneItemTemp->m_iTotalNumberChannels = fiffInfo->ch_names.size();
                averageSceneItemTemp->m_lAverageData.append(QPair<QString, RowVectorPair>(index.data(Qt::DisplayRole).toString(),
                                                                                           averageData));
            }
        }
    }

    // Auto-scale: compute optimal scale from data on first load
    if(!m_bAutoScaled && !m_layoutDisplayEvokeds.isEmpty()) {
        double maxGrad = 0, maxMag = 0, maxEEG = 0;

        for(const auto& evoked : m_layoutDisplayEvokeds) {
            if(evoked.isEmpty() || evoked.data.rows() == 0)
                continue;
            const auto& info = evoked.info;
            for(int ch = 0; ch < info.chs.size() && ch < evoked.data.rows(); ++ch) {
                if(info.bads.contains(info.chs[ch].ch_name))
                    continue;
                double maxVal = evoked.data.row(ch).cwiseAbs().maxCoeff();
                if(info.chs[ch].kind == FIFFV_MEG_CH) {
                    if(info.chs[ch].unit == FIFF_UNIT_T_M)
                        maxGrad = qMax(maxGrad, maxVal);
                    else
                        maxMag = qMax(maxMag, maxVal);
                } else if(info.chs[ch].kind == FIFFV_EEG_CH) {
                    maxEEG = qMax(maxEEG, maxVal);
                }
            }
        }

        // Set scale to ~1.4× the max to fill ~70% of item height
        QMap<qint32,float> autoScale;
        autoScale[FIFF_UNIT_T_M] = maxGrad > 0 ? static_cast<float>(maxGrad * 1.4) : 1e-10f;
        autoScale[FIFF_UNIT_T]   = maxMag  > 0 ? static_cast<float>(maxMag  * 1.4) : 1e-11f;
        autoScale[FIFFV_EEG_CH]  = maxEEG  > 0 ? static_cast<float>(maxEEG  * 1.4) : 1e-4f;
        m_pAverageScene->setScaleMap(autoScale);

        // Update spinboxes to reflect auto-scaled values
        if(m_pSpinGrad) m_pSpinGrad->setValue(autoScale[FIFF_UNIT_T_M] / 1e-10);
        if(m_pSpinMag)  m_pSpinMag->setValue(autoScale[FIFF_UNIT_T] / 1e-11);
        if(m_pSpinEEG)  m_pSpinEEG->setValue(autoScale[FIFFV_EEG_CH] / 1e-4);

        m_bAutoScaled = true;
    }

    m_pAverageScene->update();

    if(!m_pAverageScene->items().isEmpty()) {
        ui->m_graphicsView_layout->fitInView(m_pAverageScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    //Draw butterfly plot
    m_pButterflyScene->clear();

    for(int i = 0; i < selectedRows.size(); ++i) {
        QModelIndex index = selectedRows.at(i);
        QString setName = m_pAverageModel->data(m_pAverageModel->index(index.row(), 0), Qt::DisplayRole).toString();
        const FIFFLIB::FiffEvoked* sourceEvoked = m_pAverageModel->getEvoked(index.row());

        if(!sourceEvoked) {
            continue;
        }

        fiff_int_t setUnit, setKind;
        if(ui->m_comboBox_channelKind->currentText() == "MEG_grad")
            setUnit = FIFF_UNIT_T_M;
        else
            setUnit = FIFF_UNIT_T;

        if(ui->m_comboBox_channelKind->currentText() == "EEG")
            setKind = FIFFV_EEG_CH;
        else
            setKind = FIFFV_MEG_CH;

        bool whiteningApplied = false;
        FIFFLIB::FiffEvoked displayEvoked = buildButterflyEvoked(*sourceEvoked,
                                                                 ui->m_comboBox_channelKind->currentText(),
                                                                 m_noiseCovariance,
                                                                 m_whiteningSettings,
                                                                 m_whiteningSettings.enableButterfly,
                                                                 &whiteningApplied);

        if(displayEvoked.data.rows() == 0 || displayEvoked.data.cols() == 0) {
            continue;
        }

        if(whiteningApplied) {
            setName.append(" [whitened]");
        }

        ButterflySceneItem* butterflySceneItemTemp = new ButterflySceneItem(setName,
                                                                            setKind,
                                                                            setUnit,
                                                                            m_lButterflyColors);

        butterflySceneItemTemp->setEvokedData(displayEvoked);

        m_pButterflyScene->addItem(butterflySceneItemTemp);
    }

    m_pButterflyScene->update();

    //Size butterfly items to fill the view at 1:1 scale
    updateButterflySize();
}


//*************************************************************************************************************

void AverageWindow::selectLoadedSets()
{
    if(!ui->m_tableView_loadedSets->selectionModel() || m_pAverageModel->rowCount() <= 0) {
        return;
    }

    const QModelIndex first = m_pAverageModel->index(0, 0, QModelIndex());
    const QModelIndex last = m_pAverageModel->index(m_pAverageModel->rowCount() - 1, 0, QModelIndex());
    const QItemSelection selection(first, last);

    ui->m_tableView_loadedSets->selectionModel()->clearSelection();
    ui->m_tableView_loadedSets->selectionModel()->select(selection,
                                                         QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui->m_tableView_loadedSets->selectionModel()->setCurrentIndex(first,
                                                                  QItemSelectionModel::Current | QItemSelectionModel::Rows);
}


//*************************************************************************************************************

bool AverageWindow::populateLayoutFromEvoked(const QModelIndexList& selectedRows)
{
    const FIFFLIB::FiffEvoked* firstEvoked = nullptr;
    for(int i = 0; i < selectedRows.size(); ++i) {
        firstEvoked = m_pAverageModel->getEvoked(selectedRows.at(i).row());
        if(firstEvoked && !firstEvoked->isEmpty())
            break;
    }
    if(!firstEvoked)
        return false;

    // Determine which channel type to show based on the layout combo box
    const QString selectionText = ui->m_comboBox_layoutChannelKind->currentText();
    const bool showAll = (selectionText == QLatin1String("All"));

    fiff_int_t filterKind = FIFFV_MEG_CH;
    fiff_int_t filterUnit = FIFF_UNIT_T_M; // gradiometers by default
    if(selectionText == QLatin1String("MEG_mag")) {
        filterKind = FIFFV_MEG_CH;
        filterUnit = FIFF_UNIT_T;
    } else if(selectionText == QLatin1String("EEG")) {
        filterKind = FIFFV_EEG_CH;
        filterUnit = 0; // not used for EEG
    }

    SelectionItem selItem;
    const FIFFLIB::FiffInfo& info = firstEvoked->info;
    for(int ch = 0; ch < info.chs.size(); ++ch) {
        const FIFFLIB::FiffChInfo& chInfo = info.chs[ch];

        if(!showAll) {
            if(chInfo.kind != filterKind)
                continue;
            // For MEG, also filter by unit (magnetometer vs gradiometer)
            if(filterKind == FIFFV_MEG_CH && chInfo.unit != filterUnit)
                continue;
        } else {
            // In "All" mode, accept MEG and EEG data channels
            if(chInfo.kind != FIFFV_MEG_CH && chInfo.kind != FIFFV_EEG_CH)
                continue;
        }

        selItem.m_sChannelName.append(chInfo.ch_name);
        selItem.m_iChannelNumber.append(ch);
        selItem.m_iChannelKind.append(chInfo.kind);
        selItem.m_iChannelUnit.append(chInfo.unit);
        // Project 3D coil/electrode position to 2D
        selItem.m_qpChannelPosition.append(QPointF(chInfo.chpos.r0(0), chInfo.chpos.r0(1)));
    }

    if(selItem.m_qpChannelPosition.isEmpty())
        return false;

    // ── Separate co-located channels (e.g. planar grad pairs, or
    //    mag+grad triplets in "All" mode) by applying small offsets ──
    // Group channels that share the same position (within 1mm tolerance)
    // The offset is applied AFTER normalization so it works in item-grid units.
    const double posTol = 1e-3;  // 1mm in metres
    struct ColocGroup { QVector<int> indices; };
    QVector<ColocGroup> colocGroups;
    QVector<bool> handled(selItem.m_qpChannelPosition.size(), false);
    for(int i = 0; i < selItem.m_qpChannelPosition.size(); ++i) {
        if(handled[i]) continue;

        ColocGroup group;
        group.indices.append(i);
        for(int j = i + 1; j < selItem.m_qpChannelPosition.size(); ++j) {
            if(handled[j]) continue;
            double dx = selItem.m_qpChannelPosition[j].x() - selItem.m_qpChannelPosition[i].x();
            double dy = selItem.m_qpChannelPosition[j].y() - selItem.m_qpChannelPosition[i].y();
            if(std::sqrt(dx*dx + dy*dy) < posTol) {
                group.indices.append(j);
                handled[j] = true;
            }
        }
        handled[i] = true;

        if(group.indices.size() > 1) {
            // Sort within group: magnetometer first, then grads by name
            std::sort(group.indices.begin(), group.indices.end(), [&](int a, int b) {
                int unitA = selItem.m_iChannelUnit[a];
                int unitB = selItem.m_iChannelUnit[b];
                if(unitA != unitB) return unitA < unitB;
                return selItem.m_sChannelName[a] < selItem.m_sChannelName[b];
            });
            colocGroups.append(group);
        }
    }

    // Normalize positions so that items don't overlap when
    // repaintSelectionItems multiplies by 160.  Raw r0 values are in
    // metres (range ~±0.12 for MEG), giving scene coords far smaller
    // than the 120-pixel item width.
    double xMin = 1e10, xMax = -1e10, yMin = 1e10, yMax = -1e10;
    for(const auto& p : selItem.m_qpChannelPosition) {
        if(p.x() < xMin) xMin = p.x();
        if(p.x() > xMax) xMax = p.x();
        if(p.y() < yMin) yMin = p.y();
        if(p.y() > yMax) yMax = p.y();
    }
    double xRange = xMax - xMin;
    double yRange = yMax - yMin;
    double maxRange = qMax(xRange, yRange);
    if(maxRange > 1e-10) {
        double nSide = std::ceil(std::sqrt(
            static_cast<double>(selItem.m_qpChannelPosition.size())));
        double targetRange = nSide * 1.5;
        double scale = targetRange / maxRange;
        double xCenter = (xMin + xMax) / 2.0;
        double yCenter = (yMin + yMax) / 2.0;
        for(int p = 0; p < selItem.m_qpChannelPosition.size(); ++p) {
            QPointF& pt = selItem.m_qpChannelPosition[p];
            pt = QPointF((pt.x() - xCenter) * scale,
                         (pt.y() - yCenter) * scale);
        }
    }

    // Apply co-location offsets in normalized coordinates
    // Offset of 0.5 units → 80px in scene (items are 120px wide, spaced ~160px)
    for(const auto& group : colocGroups) {
        const double spacing = 0.5;
        int n = group.indices.size();
        for(int k = 0; k < n; ++k) {
            double offset = (k - (n - 1) / 2.0) * spacing;
            selItem.m_qpChannelPosition[group.indices[k]].rx() += offset;
        }
    }

    selItem.m_bShowAll = true;
    m_pAverageScene->repaintSelectionItems(selItem);
    return true;
}


//*************************************************************************************************************

void AverageWindow::exportAverageLayoutPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save average plot",
                                                    QString("%1/%2_%3_%4_AveragePlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pAverageScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pAverageScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pAverageScene->setSceneRect(m_pAverageScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pAverageScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pAverageScene->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void AverageWindow::exportAverageButterflyPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save butterfly plot",
                                                    QString("%1/%2_%3_%4_ButterflyPlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pButterflyScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pButterflyScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pButterflyScene->setSceneRect(m_pButterflyScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pButterflyScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pButterflyScene->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void AverageWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    if(m_pAverageScene && !m_pAverageScene->items().isEmpty()) {
        ui->m_graphicsView_layout->fitInView(m_pAverageScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
    updateButterflySize();
}


//*************************************************************************************************************

void AverageWindow::updateButterflySize()
{
    if(!m_pButterflyScene || m_pButterflyScene->items().isEmpty())
        return;

    // Calculate plot dimensions to fill the viewport exactly
    QSize vp = ui->m_graphicsView_butterflyPlot->viewport()->size();
    int plotW = vp.width()  - ButterflySceneItem::kMarginLeft - ButterflySceneItem::kMarginRight;
    int plotH = vp.height() - ButterflySceneItem::kMarginTop  - ButterflySceneItem::kMarginBottom;

    for(auto* gi : m_pButterflyScene->items()) {
        if(auto* bsi = dynamic_cast<ButterflySceneItem*>(gi))
            bsi->setPlotSize(plotW, plotH);
    }

    // Set scene rect to match viewport so items render at 1:1 pixel mapping
    m_pButterflyScene->setSceneRect(0, 0, vp.width(), vp.height());
    ui->m_graphicsView_butterflyPlot->resetTransform();
    m_pButterflyScene->update();
}
