//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Inspect MainWindow class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include <disp3D_rhi/view/brainview.h>
#include <disp3D_rhi/model/braintreemodel.h>
#include <disp3D_rhi/core/viewstate.h>

#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <fs/annotation.h>
#include <mne/mne_bem.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QScrollArea>
#include <QProgressBar>
#include <QGridLayout>
#include <QTimer>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("MNE Inspect");

    setupUI();
    setupConnections();

    resize(1200, 800);
}

//=============================================================================================================

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Side Panel (Controls) with Scroll Area
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFixedWidth(290);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *sidePanel = new QWidget;
    sidePanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidePanel->setMaximumWidth(280);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(4, 4, 4, 4);
    sideLayout->setSpacing(5);
    scrollArea->setWidget(sidePanel);

    // (Old controls moved to surfGroup/viewGroup)

    // ===== Brain Surface Group =====
    QGroupBox *surfGroup = new QGroupBox("Brain Surface");
    QVBoxLayout *surfLayout = new QVBoxLayout(surfGroup);
    surfLayout->setContentsMargins(6, 12, 6, 6);
    surfLayout->setSpacing(8);

    // Surface Selector
    m_loadSurfaceBtn = new QPushButton("Load Surface...");
    m_loadAtlasBtn = new QPushButton("Load Atlas...");

    QLabel *surfLabel = new QLabel("Surface Type:");
    m_surfCombo = new QComboBox;
    m_surfCombo->addItems({"pial", "inflated", "white"});

    // Shader Mode
    QLabel *shaderLabel = new QLabel("Brain Shader:");
    m_shaderCombo = new QComboBox;
    m_shaderCombo->addItems({"Standard", "Holographic", "Anatomical"});

    QLabel *bemShaderLabel = new QLabel("Head Shader:");
    m_bemShaderCombo = new QComboBox;
    m_bemShaderCombo->addItems({"Standard", "Holographic", "Anatomical"});
    m_bemShaderCombo->setEnabled(false);

    m_linkShadersCheck = new QCheckBox("Link Shaders to Brain Surface");
    m_linkShadersCheck->setChecked(true);
    m_linkShadersCheck->setToolTip("When checked, Head Shader follows Brain Shader selection.");

    m_bemColorCheck = new QCheckBox("Show BEM Colors");
    m_bemColorCheck->setChecked(true);
    m_bemColorCheck->setToolTip("Toggle between standard Red/Green/Blue colors and White/Overlay colors.");

    // Overlay
    QLabel *overlayLabel = new QLabel("Overlay:");
    m_overlayCombo = new QComboBox;
    m_overlayCombo->addItems({"Surface", "Annotation", "Scientific"});

    m_lhCheck = new QCheckBox("Left Hemisphere");
    m_lhCheck->setChecked(true);

    m_rhCheck = new QCheckBox("Right Hemisphere");
    m_rhCheck->setChecked(true);

    // BEM Checks
    m_headCheck = new QCheckBox("Head Surface");
    m_headCheck->setChecked(true);
    m_outerCheck = new QCheckBox("Outer Skull");
    m_outerCheck->setChecked(true);
    m_innerCheck = new QCheckBox("Inner Skull");
    m_innerCheck->setChecked(true);

    surfLayout->addWidget(m_loadSurfaceBtn);
    surfLayout->addWidget(m_loadAtlasBtn);
    surfLayout->addWidget(surfLabel);
    surfLayout->addWidget(m_surfCombo);
    surfLayout->addWidget(overlayLabel);
    surfLayout->addWidget(m_overlayCombo);
    surfLayout->addWidget(shaderLabel);
    surfLayout->addWidget(m_shaderCombo);
    surfLayout->addWidget(m_lhCheck);
    surfLayout->addWidget(m_rhCheck);

    // ===== BEM Surface Group =====
    QGroupBox *bemGroup = new QGroupBox("BEM Surface");
    QVBoxLayout *bemLayout = new QVBoxLayout(bemGroup);
    bemLayout->setContentsMargins(6, 12, 6, 6);
    bemLayout->setSpacing(8);

    m_loadBemBtn = new QPushButton("Load BEM...");

    bemLayout->addWidget(m_loadBemBtn);
    bemLayout->addWidget(m_headCheck);
    bemLayout->addWidget(m_outerCheck);
    bemLayout->addWidget(m_innerCheck);
    bemLayout->addWidget(m_bemColorCheck);
    bemLayout->addWidget(bemShaderLabel);
    bemLayout->addWidget(m_bemShaderCombo);
    bemLayout->addWidget(m_linkShadersCheck);

    // ===== Source Estimate Group =====
    QGroupBox *stcGroup = new QGroupBox("Source Estimate");
    QVBoxLayout *stcLayout = new QVBoxLayout(stcGroup);
    stcLayout->setContentsMargins(6, 12, 6, 6);
    stcLayout->setSpacing(8);

    m_loadStcBtn = new QPushButton("Load STC...");

    QLabel *stcSelectLabel = new QLabel("Source Estimate:");
    m_stcCombo = new QComboBox;
    m_stcCombo->setEnabled(false);
    m_stcCombo->setToolTip("Select which loaded source estimate to display");

    QLabel *colormapLabel = new QLabel("Colormap:");
    m_colormapCombo = new QComboBox;
    m_colormapCombo->addItems({"Hot", "Jet", "Viridis", "Cool", "RedBlue", "Bone"});

    m_timeLabel = new QLabel("Time: 0.000 s");
    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setEnabled(false);

    QLabel *threshLabel = new QLabel("Thresholds:");
    m_minThresh = new QDoubleSpinBox;
    m_midThresh = new QDoubleSpinBox;
    m_maxThresh = new QDoubleSpinBox;
    m_minThresh->setDecimals(6);
    m_midThresh->setDecimals(6);
    m_maxThresh->setDecimals(6);
    m_minThresh->setRange(0, 1e12);
    m_midThresh->setRange(0, 1e12);
    m_maxThresh->setRange(0, 1e12);
    m_minThresh->setValue(0.0);
    m_midThresh->setValue(0.5);
    m_maxThresh->setValue(10.0);

    QGridLayout *threshGrid = new QGridLayout;
    threshGrid->addWidget(new QLabel("Min"), 0, 0);
    threshGrid->addWidget(m_minThresh, 0, 1);
    threshGrid->addWidget(new QLabel("Mid"), 1, 0);
    threshGrid->addWidget(m_midThresh, 1, 1);
    threshGrid->addWidget(new QLabel("Max"), 2, 0);
    threshGrid->addWidget(m_maxThresh, 2, 1);

    QLabel *playbackLabel = new QLabel("Playback:");
    QHBoxLayout *playbackLayout = new QHBoxLayout;
    m_playButton = new QPushButton("Play");
    m_speedCombo = new QComboBox;
    m_speedCombo->addItem("0.1x", 0.1);
    m_speedCombo->addItem("0.25x", 0.25);
    m_speedCombo->addItem("0.5x", 0.5);
    m_speedCombo->addItem("1.0x", 1.0);
    m_speedCombo->addItem("2.0x", 2.0);
    m_speedCombo->setCurrentIndex(3);
    playbackLayout->addWidget(m_playButton);
    playbackLayout->addWidget(m_speedCombo);

    m_realtimeCheck = new QCheckBox("Realtime Streaming");
    m_realtimeCheck->setToolTip("Use threaded real-time streaming pipeline instead of simple timer playback");
    m_loopCheck = new QCheckBox("Loop");
    m_loopCheck->setChecked(true);
    m_loopCheck->setToolTip("Loop data when the end of the stream is reached");
    QHBoxLayout *rtLayout = new QHBoxLayout;
    rtLayout->addWidget(m_realtimeCheck);
    rtLayout->addWidget(m_loopCheck);

    m_stcTimer = new QTimer(this);

    m_stcStatusLabel = new QLabel("");
    m_stcStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    m_stcStatusLabel->setWordWrap(true);
    m_stcStatusLabel->hide();

    m_stcProgressBar = new QProgressBar;
    m_stcProgressBar->setRange(0, 100);
    m_stcProgressBar->setValue(0);
    m_stcProgressBar->hide();

    stcLayout->addWidget(m_loadStcBtn);
    stcLayout->addWidget(stcSelectLabel);
    stcLayout->addWidget(m_stcCombo);
    stcLayout->addWidget(m_stcStatusLabel);
    stcLayout->addWidget(m_stcProgressBar);
    stcLayout->addWidget(colormapLabel);
    stcLayout->addWidget(m_colormapCombo);
    stcLayout->addWidget(threshLabel);
    stcLayout->addLayout(threshGrid);
    stcLayout->addWidget(playbackLabel);
    stcLayout->addLayout(playbackLayout);
    stcLayout->addLayout(rtLayout);
    stcLayout->addWidget(m_timeSlider);
    stcLayout->addWidget(m_timeLabel);
    stcLayout->addStretch();

    // ===== Dipole Group =====
    QGroupBox *dipoleGroup = new QGroupBox("Dipoles");
    QVBoxLayout *dipoleLayout = new QVBoxLayout(dipoleGroup);
    dipoleLayout->setContentsMargins(6, 12, 6, 6);
    dipoleLayout->setSpacing(8);

    m_loadDipoleBtn = new QPushButton("Load Dipoles...");
    m_showDipoleCheck = new QCheckBox("Show Dipoles");
    m_showDipoleCheck->setChecked(true);
    m_showDipoleCheck->setEnabled(false);

    dipoleLayout->addWidget(m_loadDipoleBtn);
    dipoleLayout->addWidget(m_showDipoleCheck);

    // ===== Source Space Group =====
    QGroupBox *srcSpaceGroup = new QGroupBox("Source Space");
    QVBoxLayout *srcSpaceLayout = new QVBoxLayout(srcSpaceGroup);
    srcSpaceLayout->setContentsMargins(6, 12, 6, 6);
    srcSpaceLayout->setSpacing(8);

    m_loadSrcSpaceBtn = new QPushButton("Load Source Space...");
    m_showSrcSpaceCheck = new QCheckBox("Show Source Space");
    m_showSrcSpaceCheck->setChecked(false);
    m_showSrcSpaceCheck->setEnabled(false);

    srcSpaceLayout->addWidget(m_loadSrcSpaceBtn);
    srcSpaceLayout->addWidget(m_showSrcSpaceCheck);

    // ===== Connectivity Network Group =====
    QGroupBox *networkGroup = new QGroupBox("Connectivity Network");
    QVBoxLayout *networkLayout = new QVBoxLayout(networkGroup);
    networkLayout->setContentsMargins(6, 12, 6, 6);
    networkLayout->setSpacing(8);

    m_showNetworkCheck = new QCheckBox("Show Network");
    m_showNetworkCheck->setChecked(false);
    m_showNetworkCheck->setEnabled(false);

    QLabel *netThreshLabel = new QLabel("Threshold:");
    m_networkThresholdSlider = new QSlider(Qt::Horizontal);
    m_networkThresholdSlider->setRange(0, 100);
    m_networkThresholdSlider->setValue(50);
    m_networkThresholdSlider->setEnabled(false);

    QLabel *netCmapLabel = new QLabel("Colormap:");
    m_networkColormapCombo = new QComboBox;
    m_networkColormapCombo->addItems({"Hot", "Jet", "Bone", "RedBlue", "Plasma"});
    m_networkColormapCombo->setEnabled(false);

    networkLayout->addWidget(m_showNetworkCheck);
    networkLayout->addWidget(netThreshLabel);
    networkLayout->addWidget(m_networkThresholdSlider);
    networkLayout->addWidget(netCmapLabel);
    networkLayout->addWidget(m_networkColormapCombo);

    // ===== Evoked Group =====
    QGroupBox *evokedGroup = new QGroupBox("Evoked");
    QVBoxLayout *evokedLayout = new QVBoxLayout(evokedGroup);
    evokedLayout->setContentsMargins(6, 12, 6, 6);
    evokedLayout->setSpacing(6);

    m_loadEvokedBtn = new QPushButton("Load Evoked...");

    QLabel *evokedSetLabel = new QLabel("Evoked Set:");
    m_evokedSetCombo = new QComboBox;
    m_evokedSetCombo->setEnabled(false);
    m_evokedSetCombo->setToolTip("Select which evoked/average data set to display");

    m_showMegFieldCheck = new QCheckBox("Show MEG Field Map");
    m_showMegFieldCheck->setChecked(false);
    m_showMegFieldCheck->setEnabled(false);

    m_showEegFieldCheck = new QCheckBox("Show EEG Potential Map");
    m_showEegFieldCheck->setChecked(false);
    m_showEegFieldCheck->setEnabled(false);

    m_showMegContourCheck = new QCheckBox("Show MEG Isofield Lines");
    m_showMegContourCheck->setChecked(false);
    m_showMegContourCheck->setEnabled(false);

    m_showEegContourCheck = new QCheckBox("Show EEG Equipotential Lines");
    m_showEegContourCheck->setChecked(false);
    m_showEegContourCheck->setEnabled(false);

    QLabel *megSurfLabel = new QLabel("MEG Surface:");
    m_megHelmetCombo = new QComboBox;
    m_megHelmetCombo->addItems({"Helmet", "Head"});
    m_megHelmetCombo->setToolTip("Choose whether MEG field mapping uses the helmet or head surface.");
    m_megHelmetCombo->setEnabled(false);

    m_sensorFieldTimeSlider = new QSlider(Qt::Horizontal);
    m_sensorFieldTimeSlider->setRange(0, 0);
    m_sensorFieldTimeSlider->setValue(0);
    m_sensorFieldTimeSlider->setEnabled(false);

    m_sensorFieldTimeLabel = new QLabel("Field Time: 0.000 s");

    m_syncTimesCheck = new QCheckBox("Sync STC \u2194 Evoked");
    m_syncTimesCheck->setChecked(true);
    m_syncTimesCheck->setToolTip("Synchronize STC and sensor field time points");
    m_syncTimesCheck->setEnabled(false);

    evokedLayout->addWidget(m_loadEvokedBtn);
    evokedLayout->addWidget(evokedSetLabel);
    evokedLayout->addWidget(m_evokedSetCombo);
    evokedLayout->addWidget(m_showMegFieldCheck);
    evokedLayout->addWidget(m_showEegFieldCheck);
    evokedLayout->addWidget(m_showMegContourCheck);
    evokedLayout->addWidget(m_showEegContourCheck);
    evokedLayout->addWidget(megSurfLabel);
    evokedLayout->addWidget(m_megHelmetCombo);
    evokedLayout->addWidget(m_sensorFieldTimeLabel);
    evokedLayout->addWidget(m_sensorFieldTimeSlider);
    evokedLayout->addWidget(m_syncTimesCheck);

    // ===== Sensor Streaming Group =====
    QGroupBox *sensorStreamGroup = new QGroupBox("Sensor Streaming");
    QVBoxLayout *sensorStreamLayout = new QVBoxLayout(sensorStreamGroup);
    sensorStreamLayout->setContentsMargins(6, 12, 6, 6);
    sensorStreamLayout->setSpacing(6);

    QLabel *ssModalityLabel = new QLabel("Modality:");
    m_sensorStreamModalityCombo = new QComboBox;
    m_sensorStreamModalityCombo->addItems({"MEG", "EEG"});
    m_sensorStreamModalityCombo->setEnabled(false);

    m_sensorStreamBtn = new QPushButton("Start Sensor Streaming");
    m_sensorStreamBtn->setEnabled(false);

    m_sensorStreamLoopCheck = new QCheckBox("Loop");
    m_sensorStreamLoopCheck->setChecked(true);
    m_sensorStreamLoopCheck->setEnabled(false);

    QLabel *ssAvgLabel = new QLabel("Averages:");
    m_sensorStreamAvgSpin = new QSpinBox;
    m_sensorStreamAvgSpin->setRange(1, 100);
    m_sensorStreamAvgSpin->setValue(1);
    m_sensorStreamAvgSpin->setEnabled(false);

    QLabel *ssCmapLabel = new QLabel("Colormap:");
    m_sensorStreamColormapCombo = new QComboBox;
    m_sensorStreamColormapCombo->addItems({"MNE", "Hot", "Jet", "Viridis", "Cool", "RedBlue"});
    m_sensorStreamColormapCombo->setEnabled(false);

    sensorStreamLayout->addWidget(ssModalityLabel);
    sensorStreamLayout->addWidget(m_sensorStreamModalityCombo);
    sensorStreamLayout->addWidget(m_sensorStreamBtn);
    sensorStreamLayout->addWidget(m_sensorStreamLoopCheck);
    sensorStreamLayout->addWidget(ssAvgLabel);
    sensorStreamLayout->addWidget(m_sensorStreamAvgSpin);
    sensorStreamLayout->addWidget(ssCmapLabel);
    sensorStreamLayout->addWidget(m_sensorStreamColormapCombo);

    // ===== Sensor Group =====
    QGroupBox *sensorGroup = new QGroupBox("Sensors");
    QVBoxLayout *sensorLayout = new QVBoxLayout(sensorGroup);
    sensorLayout->setContentsMargins(6, 12, 6, 6);
    sensorLayout->setSpacing(8);

    m_loadDigBtn = new QPushButton("Load Digitizer...");
    m_loadTransBtn = new QPushButton("Load Transformation...");

    m_showMegCheck = new QCheckBox("Show MEG");
    m_showMegCheck->setChecked(false);
    m_showMegCheck->setEnabled(false);

    m_showEegCheck = new QCheckBox("Show EEG");
    m_showEegCheck->setChecked(false);
    m_showEegCheck->setEnabled(false);

    m_showDigCheck = new QCheckBox("Show Digitizer");
    m_showDigCheck->setChecked(false);
    m_showDigCheck->setEnabled(false);

    // Per-category digitizer visibility
    m_showDigCardinalCheck = new QCheckBox("  Cardinal (NAS/LPA/RPA)");
    m_showDigCardinalCheck->setChecked(false);
    m_showDigCardinalCheck->setEnabled(false);

    m_showDigHpiCheck = new QCheckBox("  HPI Coils");
    m_showDigHpiCheck->setChecked(false);
    m_showDigHpiCheck->setEnabled(false);

    m_showDigEegCheck = new QCheckBox("  EEG Points");
    m_showDigEegCheck->setChecked(false);
    m_showDigEegCheck->setEnabled(false);

    m_showDigExtraCheck = new QCheckBox("  Head Shape");
    m_showDigExtraCheck->setChecked(false);
    m_showDigExtraCheck->setEnabled(false);

    m_showHelmetCheck = new QCheckBox("Show MEG Helmet");
    m_showHelmetCheck->setChecked(false);
    m_showHelmetCheck->setEnabled(false);

    m_applyTransCheck = new QCheckBox("Apply Transformation");
    m_applyTransCheck->setChecked(true);
    m_applyTransCheck->setToolTip("Apply Head-to-MRI transformation to sensors if available.");

    sensorLayout->addWidget(m_loadDigBtn);
    sensorLayout->addWidget(m_loadTransBtn);
    sensorLayout->addWidget(m_showMegCheck);
    sensorLayout->addWidget(m_showEegCheck);
    sensorLayout->addWidget(m_showDigCheck);
    sensorLayout->addWidget(m_showDigCardinalCheck);
    sensorLayout->addWidget(m_showDigHpiCheck);
    sensorLayout->addWidget(m_showDigEegCheck);
    sensorLayout->addWidget(m_showDigExtraCheck);
    sensorLayout->addWidget(m_showHelmetCheck);
    sensorLayout->addWidget(m_applyTransCheck);

    // ===== View Group =====
    QGroupBox *viewGroup = new QGroupBox("View");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewGroup);
    viewLayout->setContentsMargins(6, 12, 6, 6);
    viewLayout->setSpacing(8);

    // Info Panel Toggle
    m_showInfoCheck = new QCheckBox("Show Info Labels");
    m_showInfoCheck->setChecked(true);
    m_showInfoCheck->setToolTip("Show or hide overlay info labels (FPS, shader, surface info).");
    viewLayout->addWidget(m_showInfoCheck);

    // View Count
    QLabel *viewCountLabel = new QLabel("Layout:");
    m_viewCountCombo = new QComboBox();
    m_viewCountCombo->addItems({"Single View", "2 Views", "3 Views", "4 Views"});
    m_viewCountCombo->setCurrentIndex(0);
    m_viewCountCombo->setToolTip("Number of viewport panes (1 = single, 2 = side-by-side, 3 = two + one, 4 = 2×2 grid).");
    QHBoxLayout *viewCountRow = new QHBoxLayout();
    viewCountRow->addWidget(viewCountLabel);
    viewCountRow->addWidget(m_viewCountCombo);
    viewLayout->addLayout(viewCountRow);

    // Edit Target
    m_editTargetLabel = new QLabel("Edit Target:");
    m_editTargetCombo = new QComboBox();
    m_editTargetCombo->addItem("All Viewports");
    m_editTargetCombo->setToolTip("Select which viewport the surface, shader, and overlay controls apply to.");
    m_editTargetCombo->setEnabled(false);
    QHBoxLayout *editTargetRow = new QHBoxLayout();
    editTargetRow->addWidget(m_editTargetLabel);
    editTargetRow->addWidget(m_editTargetCombo);
    viewLayout->addLayout(editTargetRow);

    // Camera Preset (per-viewport)
    m_cameraPresetLabel = new QLabel("Camera:");
    m_cameraPresetCombo = new QComboBox();
    m_cameraPresetCombo->addItems({"Top", "Perspective", "Front", "Left", "Bottom", "Back", "Right"});
    m_cameraPresetCombo->setCurrentIndex(1);  // Perspective
    m_cameraPresetCombo->setToolTip("Camera orientation preset for the selected viewport.");
    m_cameraPresetCombo->setEnabled(false);
    QHBoxLayout *presetRow = new QHBoxLayout();
    presetRow->addWidget(m_cameraPresetLabel);
    presetRow->addWidget(m_cameraPresetCombo);
    viewLayout->addLayout(presetRow);

    // Assemble side panel
    sideLayout->addWidget(surfGroup);
    sideLayout->addWidget(bemGroup);
    sideLayout->addWidget(stcGroup);
    sideLayout->addWidget(dipoleGroup);
    sideLayout->addWidget(srcSpaceGroup);
    sideLayout->addWidget(networkGroup);
    sideLayout->addWidget(evokedGroup);
    sideLayout->addWidget(sensorStreamGroup);
    sideLayout->addWidget(sensorGroup);
    sideLayout->addWidget(viewGroup);
    sideLayout->addStretch();

    // ===== Brain View =====
    m_brainView = new BrainView;
    m_model = new BrainTreeModel(m_brainView);
    m_brainView->setModel(m_model);

    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(m_brainView);
}

//=============================================================================================================

void MainWindow::setupConnections()
{
    // Surface type
    connect(m_surfCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setActiveSurface);

    // Inflated surface logic
    connect(m_surfCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        bool isInflated = (text == "inflated");
        if (isInflated) {
            m_headCheck->setChecked(false);
            m_outerCheck->setChecked(false);
            m_innerCheck->setChecked(false);
        }
        m_headCheck->setEnabled(!isInflated);
        m_outerCheck->setEnabled(!isInflated);
        m_innerCheck->setEnabled(!isInflated);
    });

    // Shaders
    connect(m_shaderCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setShaderMode);
    connect(m_shaderCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        if (m_linkShadersCheck->isChecked()) {
            m_bemShaderCombo->setCurrentText(text);
        }
    });
    connect(m_bemShaderCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setBemShaderMode);
    connect(m_linkShadersCheck, &QCheckBox::toggled, [this](bool checked) {
        m_bemShaderCombo->setEnabled(!checked);
        if (checked) {
            m_bemShaderCombo->setCurrentText(m_shaderCombo->currentText());
        }
    });

    // Overlay
    connect(m_overlayCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setVisualizationMode(text);
        if (text != "Source Estimate" && m_stcTimer->isActive()) {
            m_stcTimer->stop();
            m_playButton->setText("Play");
        }
    });

    // Hemisphere visibility
    connect(m_lhCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setHemiVisible(0, checked); });
    connect(m_rhCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setHemiVisible(1, checked); });

    // BEM loading
    connect(m_loadBemBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select BEM Surface", "", "BEM Files (*-bem.fif *-bem-sol.fif);;FIF Files (*.fif);;All Files (*)");
        if (path.isEmpty()) return;
        loadBem("User", path);
    });

    // BEM visibility
    connect(m_headCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("head", checked); });
    connect(m_outerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("outer_skull", checked); });
    connect(m_innerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("inner_skull", checked); });
    connect(m_bemColorCheck, &QCheckBox::toggled, m_brainView, &BrainView::setBemHighContrast);

    // Multi-view: view count
    connect(m_viewCountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        const int count = index + 1;
        m_brainView->setViewCount(count);
        updateViewportCheckboxes(count);

        const bool isMulti = (count > 1);
        m_editTargetCombo->setEnabled(isMulti);
        m_cameraPresetCombo->setEnabled(isMulti);

        // Rebuild the edit-target combo items
        m_editTargetCombo->blockSignals(true);
        m_editTargetCombo->clear();
        if (isMulti) {
            for (int i = 0; i < count; ++i) {
                const int preset = i;  // defaultForIndex assigns preset = index
                m_editTargetCombo->addItem(multiViewPresetName(preset), i);
            }
            m_editTargetCombo->setCurrentIndex(0);
            m_brainView->setVisualizationEditTarget(0);
            syncUIToEditTarget(0);
        } else {
            m_editTargetCombo->addItem("All Viewports");
            m_brainView->setVisualizationEditTarget(-1);
            syncUIToEditTarget(-1);
        }
        m_editTargetCombo->blockSignals(false);
    });

    // Multi-view: edit target selection
    connect(m_editTargetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (m_brainView->viewMode() == BrainView::SingleView) return;
        const int target = m_editTargetCombo->itemData(index).toInt();
        m_brainView->setVisualizationEditTarget(target);
        syncUIToEditTarget(target);
    });

    // Multi-view: camera preset for the selected viewport
    connect(m_cameraPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int presetIndex) {
        if (m_brainView->viewMode() == BrainView::SingleView) return;
        const int target = m_brainView->visualizationEditTarget();
        if (target >= 0) {
            m_brainView->setViewportCameraPreset(target, presetIndex);
        }
    });

    // Multi-view: bidirectional sync when user clicks viewport labels inside BrainView
    connect(m_brainView, &BrainView::visualizationEditTargetChanged, [this](int target) {
        if (target < 0) return;
        // Update combo without re-triggering the connection
        m_editTargetCombo->blockSignals(true);
        for (int i = 0; i < m_editTargetCombo->count(); ++i) {
            if (m_editTargetCombo->itemData(i).toInt() == target) {
                m_editTargetCombo->setCurrentIndex(i);
                break;
            }
        }
        m_editTargetCombo->blockSignals(false);
        syncUIToEditTarget(target);
    });

    // Brain Surface
    connect(m_loadSurfaceBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Surface", "", "FreeSurfer Surface (*.pial *.inflated *.white *.orig);;All Files (*)");
        if (path.isEmpty()) return;

        // Guess hemi and type
        QString fileName = QFileInfo(path).fileName();
        QString hemi = fileName.contains("lh.") ? "lh" : (fileName.contains("rh.") ? "rh" : "lh");
        QString type = "pial";
        if (fileName.contains("inflated")) type = "inflated";
        else if (fileName.contains("white")) type = "white";
        else if (fileName.contains("orig")) type = "orig";

        Surface surf(path);
        if (!surf.isEmpty()) {
            m_model->addSurface("User", hemi, type, surf);
        }
    });

    connect(m_loadAtlasBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Atlas", "", "FreeSurfer Annotation (*.annot);;All Files (*)");
        if (path.isEmpty()) return;

        // Guess hemi
        QString fileName = QFileInfo(path).fileName();
        QString hemi = fileName.contains("lh.") ? "lh" : (fileName.contains("rh.") ? "rh" : "lh");

        Annotation annot(path);
        if (!annot.isEmpty()) {
            m_model->addAnnotation("User", hemi, annot);
        }
    });

    // STC loading – add file to combo, which triggers the actual load
    connect(m_loadStcBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Source Estimate", "", "STC Files (*-lh.stc *-rh.stc *.stc)");
        if (path.isEmpty()) return;
        addStcEntry(path, true);
    });

    // STC combo selection changed – load the selected STC pair
    connect(m_stcCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (index < 0) return;

        QStringList pair = m_stcCombo->itemData(index).toStringList();
        if (pair.size() < 2) return;
        QString lhPath = pair[0];
        QString rhPath = pair[1];

        m_loadStcBtn->setEnabled(false);
        m_stcStatusLabel->setText("Starting...");
        m_stcStatusLabel->show();
        m_stcProgressBar->setValue(0);
        m_stcProgressBar->show();

        m_brainView->loadSourceEstimate(lhPath, rhPath);
    });

    connect(m_brainView, &BrainView::stcLoadingProgress, [this](int percent, const QString &message) {
        m_stcProgressBar->setValue(percent);
        m_stcStatusLabel->setText(message);
    });

    connect(m_brainView, &BrainView::sourceEstimateLoaded, [this](int numPoints) {
        m_stcStatusLabel->hide();
        m_stcProgressBar->hide();
        m_loadStcBtn->setEnabled(true);

        m_timeSlider->setEnabled(true);
        m_timeSlider->setRange(0, numPoints - 1);
        m_timeSlider->setValue(0);

        // Add Source Estimate option if not present
        if (m_overlayCombo->findText("Source Estimate") == -1) {
            m_overlayCombo->addItem("Source Estimate");
        }
        m_overlayCombo->setCurrentText("Source Estimate");

        m_playButton->setText("Play");
        m_stcTimer->stop();

        float tstep = m_brainView->stcStep();
        if (tstep > 0) {
            double factor = m_speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            m_stcTimer->setInterval(interval);
        }
    });

    connect(m_timeSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setTimePoint(value);
    });
    connect(m_brainView, &BrainView::timePointChanged, [this](int /*index*/, float time) {
        m_timeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));

        // Sync sensor field time if enabled (skip if already syncing from evoked)
        if (!m_isSyncing && m_syncTimesCheck->isChecked() && m_sensorFieldTimeSlider->isEnabled()) {
            m_isSyncing = true;
            int sfIdx = m_brainView->closestSensorFieldIndex(time);
            if (sfIdx >= 0) {
                m_sensorFieldTimeSlider->blockSignals(true);
                m_sensorFieldTimeSlider->setValue(sfIdx);
                m_sensorFieldTimeSlider->blockSignals(false);
                m_brainView->setSensorFieldTimePoint(sfIdx);
            }
            m_isSyncing = false;
        }
    });

    connect(m_colormapCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setSourceColormap);

    auto updateThresholds = [this]() {
        m_brainView->setSourceThresholds(
            static_cast<float>(m_minThresh->value()),
            static_cast<float>(m_midThresh->value()),
            static_cast<float>(m_maxThresh->value())
        );
    };
    connect(m_minThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    connect(m_midThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    connect(m_maxThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);

    // Auto-thresholds from loaded STC data
    connect(m_brainView, &BrainView::sourceThresholdsUpdated, [this](float min, float mid, float max) {
        m_minThresh->blockSignals(true);
        m_midThresh->blockSignals(true);
        m_maxThresh->blockSignals(true);
        m_minThresh->setValue(static_cast<double>(min));
        m_midThresh->setValue(static_cast<double>(mid));
        m_maxThresh->setValue(static_cast<double>(max));
        m_minThresh->blockSignals(false);
        m_midThresh->blockSignals(false);
        m_maxThresh->blockSignals(false);
    });

    // Playback
    connect(m_playButton, &QPushButton::clicked, [this]() {
        if (m_realtimeCheck->isChecked()) {
            // Real-time streaming mode
            if (m_brainView->isRealtimeStreaming()) {
                m_brainView->stopRealtimeStreaming();
                m_playButton->setText("Play");
            } else {
                if (m_overlayCombo->currentText() != "Source Estimate") {
                    m_overlayCombo->setCurrentText("Source Estimate");
                }
                m_brainView->startRealtimeStreaming();
                m_playButton->setText("Pause");
            }
        } else {
            // Simple timer mode
            if (m_stcTimer->isActive()) {
                m_stcTimer->stop();
                m_playButton->setText("Play");
            } else {
                if (m_overlayCombo->currentText() != "Source Estimate") {
                    m_overlayCombo->setCurrentText("Source Estimate");
                }
                m_stcTimer->start();
                m_playButton->setText("Pause");
            }
        }
    });

    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        float tstep = m_brainView->stcStep();
        if (tstep > 0) {
            double factor = m_speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            m_stcTimer->setInterval(interval);
            // Also update real-time streaming interval
            m_brainView->setRealtimeInterval(interval);
        }
    });

    connect(m_loopCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setRealtimeLooping(checked);
    });

    connect(m_realtimeCheck, &QCheckBox::toggled, [this](bool checked) {
        // If switching modes while playing, stop active playback
        if (checked && m_stcTimer->isActive()) {
            m_stcTimer->stop();
            m_playButton->setText("Play");
        } else if (!checked && m_brainView->isRealtimeStreaming()) {
            m_brainView->stopRealtimeStreaming();
            m_playButton->setText("Play");
        }
    });

    connect(m_stcTimer, &QTimer::timeout, [this]() {
        int next = m_timeSlider->value() + 1;
        if (next >= m_timeSlider->maximum()) next = 0;
        m_timeSlider->setValue(next);
    });

    // Sensors
    connect(m_loadDigBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Sensor/Digitizer File", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;

        if (m_brainView->loadSensors(path)) {
            m_showMegCheck->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            m_showHelmetCheck->setEnabled(true);
            // Sub-category checkboxes stay disabled until master is toggled on
        }
    });

    connect(m_loadTransBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Transformation", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        m_brainView->loadTransformation(path);
    });

    connect(m_showMegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG", checked); });
    connect(m_showEegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("EEG", checked); });
    connect(m_showDigCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorVisible("Digitizer", checked);
        // Enable/disable sub-category checkboxes based on master toggle
        m_showDigCardinalCheck->setEnabled(checked);
        m_showDigHpiCheck->setEnabled(checked);
        m_showDigEegCheck->setEnabled(checked);
        m_showDigExtraCheck->setEnabled(checked);
        // When master is turned on, check all sub-categories; when off, uncheck them
        m_showDigCardinalCheck->setChecked(checked);
        m_showDigHpiCheck->setChecked(checked);
        m_showDigEegCheck->setChecked(checked);
        m_showDigExtraCheck->setChecked(checked);
    });
    connect(m_showDigCardinalCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/Cardinal", checked); });
    connect(m_showDigHpiCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/HPI", checked); });
    connect(m_showDigEegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/EEG", checked); });
    connect(m_showDigExtraCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/Extra", checked); });
    connect(m_showHelmetCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG Helmet", checked); });
    connect(m_applyTransCheck, &QCheckBox::toggled, m_brainView, &BrainView::setSensorTransEnabled);

    // Dipoles
    connect(m_loadDipoleBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Dipoles", "", "Dipole Files (*.dip *.bdip)");
        if (path.isEmpty()) return;
        if (m_brainView->loadDipoles(path)) {
            m_showDipoleCheck->setEnabled(true);
        }
    });

    connect(m_showDipoleCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setDipoleVisible(checked); });

    // Source Space
    connect(m_loadSrcSpaceBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Source Space / Forward Solution", "",
            "Source Space Files (*-src.fif *-fwd.fif);;All FIF Files (*.fif)");
        if (path.isEmpty()) return;
        if (m_brainView->loadSourceSpace(path)) {
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
    });

    connect(m_showSrcSpaceCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSourceSpaceVisible(checked); });

    // Connectivity Network
    connect(m_showNetworkCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setNetworkVisible(checked); });
    connect(m_networkThresholdSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setNetworkThreshold(value / 100.0);
    });
    connect(m_networkColormapCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setNetworkColormap(text);
    });

    // ── Evoked ─────────────────────────────────────────────────────────

    // Load Evoked (average FIF) – probe for evoked sets first
    connect(m_loadEvokedBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this,
            "Select Evoked / Average", "",
            "Average FIF Files (*-ave.fif);;All Files (*)");
        if (path.isEmpty()) return;

        // Probe available evoked sets and populate combo
        QStringList sets = BrainView::probeEvokedSets(path);
        m_evokedSetCombo->blockSignals(true);
        m_evokedSetCombo->clear();
        if (sets.isEmpty()) {
            m_evokedSetCombo->addItem("(no evoked sets found)");
            m_evokedSetCombo->setEnabled(false);
            m_evokedSetCombo->blockSignals(false);
            return;
        }
        m_evokedSetCombo->addItems(sets);
        m_evokedSetCombo->setEnabled(sets.size() > 1);
        m_evokedSetCombo->setCurrentIndex(0);
        m_evokedSetCombo->blockSignals(false);

        // Store the path for re-loading when the combo changes
        m_evokedSetCombo->setProperty("evokedPath", path);

        // Load the first evoked set
        m_brainView->loadSensorField(path, 0);
    });

    // Evoked set selection changed – reload with new index
    connect(m_evokedSetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (index < 0) return;
        QString path = m_evokedSetCombo->property("evokedPath").toString();
        if (path.isEmpty()) return;
        m_brainView->loadSensorField(path, index);
    });

    connect(m_brainView, &BrainView::sensorFieldLoaded, [this](int numTimePoints, int initialTimePoint) {
        // Enable field-map controls
        m_showMegFieldCheck->setEnabled(true);
        m_showEegFieldCheck->setEnabled(true);
        m_showMegContourCheck->setEnabled(true);
        m_showEegContourCheck->setEnabled(true);
        m_megHelmetCombo->setEnabled(true);
        m_syncTimesCheck->setEnabled(true);

        // Enable time slider, preserving position when switching evoked sets
        m_sensorFieldTimeSlider->setEnabled(true);
        m_sensorFieldTimeSlider->blockSignals(true);
        m_sensorFieldTimeSlider->setRange(0, numTimePoints - 1);
        m_sensorFieldTimeSlider->setValue(qBound(0, initialTimePoint, numTimePoints - 1));
        m_sensorFieldTimeSlider->blockSignals(false);

        // Enable streaming controls
        m_sensorStreamBtn->setEnabled(true);
        m_sensorStreamModalityCombo->setEnabled(true);
        m_sensorStreamLoopCheck->setEnabled(true);
        m_sensorStreamAvgSpin->setEnabled(true);
        m_sensorStreamColormapCombo->setEnabled(true);

        // Auto-show MEG field map
        m_showMegFieldCheck->setChecked(true);
    });

    // Field map visibility
    connect(m_showMegFieldCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldVisible("MEG", checked);
    });
    connect(m_showEegFieldCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldVisible("EEG", checked);
    });

    // Contour visibility
    connect(m_showMegContourCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldContourVisible("MEG", checked);
    });
    connect(m_showEegContourCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldContourVisible("EEG", checked);
    });

    // Info panel
    connect(m_showInfoCheck, &QCheckBox::toggled, m_brainView, &BrainView::setInfoPanelVisible);

    // MEG helmet surface selection
    connect(m_megHelmetCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setMegFieldMapOnHead(text == "Head");
    });

    // Sensor field time slider
    connect(m_sensorFieldTimeSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setSensorFieldTimePoint(value);

        // Sync STC time if enabled (skip if already syncing from STC)
        if (!m_isSyncing && m_syncTimesCheck->isChecked() && m_timeSlider->isEnabled()) {
            m_isSyncing = true;
            float tmin = 0, tmax = 0;
            if (m_brainView->sensorFieldTimeRange(tmin, tmax)) {
                int maxIdx = m_sensorFieldTimeSlider->maximum();
                float timeSec = (maxIdx > 0) ?
                    tmin + (tmax - tmin) * static_cast<float>(value) / static_cast<float>(maxIdx) :
                    tmin;
                int stcIdx = m_brainView->closestStcIndex(timeSec);
                if (stcIdx >= 0) {
                    m_timeSlider->blockSignals(true);
                    m_timeSlider->setValue(stcIdx);
                    m_timeSlider->blockSignals(false);
                    m_brainView->setTimePoint(stcIdx);
                }
            }
            m_isSyncing = false;
        }
    });

    // Update sensor field time label
    connect(m_brainView, &BrainView::sensorFieldTimePointChanged, [this](int /*index*/, float time) {
        m_sensorFieldTimeLabel->setText(QString("Field Time: %1 s").arg(time, 0, 'f', 3));
    });

    // ── Sensor Streaming ────────────────────────────────────────────────
    connect(m_sensorStreamBtn, &QPushButton::clicked, [this]() {
        if (m_brainView->isRealtimeSensorStreaming()) {
            m_brainView->stopRealtimeSensorStreaming();
            m_sensorStreamBtn->setText("Start Sensor Streaming");
        } else {
            QString modality = m_sensorStreamModalityCombo->currentText();
            m_brainView->startRealtimeSensorStreaming(modality);
            if (m_brainView->isRealtimeSensorStreaming()) {
                m_sensorStreamBtn->setText("Stop Sensor Streaming");
            }
        }
    });

    connect(m_sensorStreamLoopCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setRealtimeSensorLooping(checked);
    });

    connect(m_sensorStreamAvgSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        m_brainView->setRealtimeSensorAverages(value);
    });

    connect(m_sensorStreamColormapCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setRealtimeSensorColormap(text);
    });

    // ── Sync UI widgets to saved BrainView state ──
    // BrainView::loadMultiViewSettings() has already restored the persisted
    // view state (surface type, visibility, shaders, overlay).  Sync the
    // MainWindow controls to match so that there is no mismatch between
    // what the user sees in the controls and what the renderer uses.
    const int target = m_brainView->visualizationEditTarget();

    // Surface type
    const QString savedSurf = m_brainView->activeSurfaceForTarget(target);
    if (m_surfCombo->findText(savedSurf) >= 0) {
        m_surfCombo->blockSignals(true);
        m_surfCombo->setCurrentText(savedSurf);
        m_surfCombo->blockSignals(false);
    }

    // Hemisphere visibility
    m_lhCheck->blockSignals(true);
    m_rhCheck->blockSignals(true);
    m_lhCheck->setChecked(m_brainView->objectVisibleForTarget("lh", target));
    m_rhCheck->setChecked(m_brainView->objectVisibleForTarget("rh", target));
    m_lhCheck->blockSignals(false);
    m_rhCheck->blockSignals(false);

    // BEM visibility
    const bool isInflated = (savedSurf == "inflated");
    m_headCheck->blockSignals(true);
    m_outerCheck->blockSignals(true);
    m_innerCheck->blockSignals(true);
    m_headCheck->setChecked(m_brainView->objectVisibleForTarget("bem_head", target));
    m_outerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_outer_skull", target));
    m_innerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_inner_skull", target));
    m_headCheck->setEnabled(!isInflated);
    m_outerCheck->setEnabled(!isInflated);
    m_innerCheck->setEnabled(!isInflated);
    m_headCheck->blockSignals(false);
    m_outerCheck->blockSignals(false);
    m_innerCheck->blockSignals(false);

    // Shader
    const QString savedShader = m_brainView->shaderModeForTarget(target);
    if (m_shaderCombo->findText(savedShader) >= 0) {
        m_shaderCombo->blockSignals(true);
        m_shaderCombo->setCurrentText(savedShader);
        m_shaderCombo->blockSignals(false);
    }

    const QString savedBemShader = m_brainView->bemShaderModeForTarget(target);
    if (m_bemShaderCombo->findText(savedBemShader) >= 0) {
        m_bemShaderCombo->blockSignals(true);
        m_bemShaderCombo->setCurrentText(savedBemShader);
        m_bemShaderCombo->blockSignals(false);
    }

    // Overlay
    const QString savedOverlay = m_brainView->overlayModeForTarget(target);
    if (m_overlayCombo->findText(savedOverlay) >= 0) {
        m_overlayCombo->blockSignals(true);
        m_overlayCombo->setCurrentText(savedOverlay);
        m_overlayCombo->blockSignals(false);
    }

    // Sensor visibility (checked state synced; enabled state stays as-is
    // until data is loaded at runtime)
    m_showMegCheck->blockSignals(true);
    m_showMegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg", target));
    m_showMegCheck->blockSignals(false);

    m_showEegCheck->blockSignals(true);
    m_showEegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_eeg", target));
    m_showEegCheck->blockSignals(false);

    m_showDigCheck->blockSignals(true);
    m_showDigCheck->setChecked(m_brainView->objectVisibleForTarget("dig", target));
    m_showDigCheck->blockSignals(false);

    m_showHelmetCheck->blockSignals(true);
    m_showHelmetCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg_helmet", target));
    m_showHelmetCheck->blockSignals(false);

    // Field map / contour visibility
    m_showMegFieldCheck->blockSignals(true);
    m_showMegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_meg", target));
    m_showMegFieldCheck->blockSignals(false);

    m_showEegFieldCheck->blockSignals(true);
    m_showEegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_eeg", target));
    m_showEegFieldCheck->blockSignals(false);

    m_showMegContourCheck->blockSignals(true);
    m_showMegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_meg", target));
    m_showMegContourCheck->blockSignals(false);

    m_showEegContourCheck->blockSignals(true);
    m_showEegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_eeg", target));
    m_showEegContourCheck->blockSignals(false);

    // MEG field map surface
    m_megHelmetCombo->blockSignals(true);
    m_megHelmetCombo->setCurrentText(m_brainView->megFieldMapOnHeadForTarget(target) ? "Head" : "Helmet");
    m_megHelmetCombo->blockSignals(false);

    // Info panel visibility
    m_showInfoCheck->blockSignals(true);
    m_showInfoCheck->setChecked(m_brainView->isInfoPanelVisible());
    m_showInfoCheck->blockSignals(false);
}

//=============================================================================================================

void MainWindow::loadInitialData(const QString &subjectPath,
                                  const QString &subjectName,
                                  const QString &bemPath,
                                  const QString &transPath,
                                  const QStringList &stcPaths,
                                  const QString &digitizerPath,
                                  const QString &srcSpacePath,
                                  const QString &atlasPath,
                                  const QString &evokedPath)
{
    // Check if the subject directory actually exists before attempting to load
    QString subjectDir = subjectPath + "/" + subjectName + "/surf";
    QDir surfDir(subjectDir);
    if (!surfDir.exists()) {
        qDebug() << "No surface data found. Subject directory does not exist:" << subjectDir;
        qDebug() << "Use --subjectPath and --subject options to specify data location,";
        qDebug() << "or load data interactively through the UI.";
    } else {
        qDebug() << "Loading surfaces...";
        loadHemisphere(subjectPath, subjectName, "lh");
        loadHemisphere(subjectPath, subjectName, "rh");
        qDebug() << "Surfaces loaded.";
    }

    // Auto-load atlas annotation (explicit path overrides auto-discovery in loadHemisphere)
    if (!atlasPath.isEmpty() && QFile::exists(atlasPath)) {
        qDebug() << "Auto-loading atlas from:" << atlasPath;
        QString fileName = QFileInfo(atlasPath).fileName();
        QString lhAnnotPath, rhAnnotPath;

        if (fileName.startsWith("lh.")) {
            lhAnnotPath = atlasPath;
            rhAnnotPath = QFileInfo(atlasPath).absolutePath() + "/" + fileName.mid(3).prepend("rh.");
        } else if (fileName.startsWith("rh.")) {
            rhAnnotPath = atlasPath;
            lhAnnotPath = QFileInfo(atlasPath).absolutePath() + "/" + fileName.mid(3).prepend("lh.");
        } else {
            lhAnnotPath = atlasPath;
        }

        if (!lhAnnotPath.isEmpty() && QFile::exists(lhAnnotPath)) {
            Annotation annot(lhAnnotPath);
            if (!annot.isEmpty()) {
                m_model->addAnnotation(subjectName, "lh", annot);
                qDebug() << "Added atlas annotation for lh";
            }
        }
        if (!rhAnnotPath.isEmpty() && QFile::exists(rhAnnotPath)) {
            Annotation annot(rhAnnotPath);
            if (!annot.isEmpty()) {
                m_model->addAnnotation(subjectName, "rh", annot);
                qDebug() << "Added atlas annotation for rh";
            }
        }
    }

    if (!bemPath.isEmpty()) {
        loadBem(subjectName, bemPath);
    }

    // Auto-load digitizer
    if (!digitizerPath.isEmpty() && QFile::exists(digitizerPath)) {
        qDebug() << "Auto-loading digitizer from:" << digitizerPath;
        if (m_brainView->loadSensors(digitizerPath)) {
            m_showMegCheck->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            m_showHelmetCheck->setEnabled(true);
            // Sub-category checkboxes stay disabled until master is toggled on
            m_brainView->setSensorVisible("MEG", m_showMegCheck->isChecked());
            m_brainView->setSensorVisible("EEG", m_showEegCheck->isChecked());
            m_brainView->setSensorVisible("Digitizer", m_showDigCheck->isChecked());
        }
    }

    // Auto-load transformation
    if (!transPath.isEmpty() && QFile::exists(transPath)) {
        qDebug() << "Auto-loading transformation from:" << transPath;
        m_brainView->loadTransformation(transPath);
    }

    // Auto-load source space
    if (!srcSpacePath.isEmpty() && QFile::exists(srcSpacePath)) {
        qDebug() << "Auto-loading source space from:" << srcSpacePath;
        if (m_brainView->loadSourceSpace(srcSpacePath)) {
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
    }

    // Auto-load STC(s) – add all provided paths to the combo; activate only the first
    for (int i = 0; i < stcPaths.size(); ++i) {
        const QString &stcPath = stcPaths[i];
        if (!stcPath.isEmpty() && QFile::exists(stcPath)) {
            qDebug() << "Auto-loading source estimate from:" << stcPath;
            addStcEntry(stcPath, /*activate=*/ (i == 0));
        }
    }

    // Auto-load evoked
    if (!evokedPath.isEmpty() && QFile::exists(evokedPath)) {
        qDebug() << "Auto-loading evoked from:" << evokedPath;

        QStringList sets = BrainView::probeEvokedSets(evokedPath);
        m_evokedSetCombo->blockSignals(true);
        m_evokedSetCombo->clear();
        if (!sets.isEmpty()) {
            m_evokedSetCombo->addItems(sets);
            m_evokedSetCombo->setEnabled(sets.size() > 1);
            m_evokedSetCombo->setCurrentIndex(0);
        }
        m_evokedSetCombo->blockSignals(false);
        m_evokedSetCombo->setProperty("evokedPath", evokedPath);

        m_brainView->loadSensorField(evokedPath, 0);
    }
}

//=============================================================================================================

void MainWindow::addStcEntry(const QString &stcPath, bool activate)
{
    // Resolve the LH/RH pair
    QString lhPath, rhPath;
    if (stcPath.contains("-lh.stc")) {
        lhPath = stcPath;
        QString sibling = stcPath; sibling.replace("-lh.stc", "-rh.stc");
        if (QFile::exists(sibling)) rhPath = sibling;
    } else if (stcPath.contains("-rh.stc")) {
        rhPath = stcPath;
        QString sibling = stcPath; sibling.replace("-rh.stc", "-lh.stc");
        if (QFile::exists(sibling)) lhPath = sibling;
    } else {
        lhPath = stcPath;
    }

    // Derive a display name from the filename:
    // e.g. "sample_audvis-ave-spm-left_auditory-lh.stc" → "sample_audvis-ave-spm-left_auditory"
    QString displayName;
    QString refPath = !lhPath.isEmpty() ? lhPath : rhPath;
    QFileInfo fi(refPath);
    displayName = fi.fileName();
    displayName.replace("-lh.stc", "").replace("-rh.stc", "");

    // Avoid duplicates
    for (int i = 0; i < m_stcCombo->count(); ++i) {
        if (m_stcCombo->itemText(i) == displayName) {
            if (activate) {
                m_stcCombo->setCurrentIndex(i);
            }
            return;
        }
    }

    // Store the LH/RH paths as item data
    QStringList pair;
    pair << lhPath << rhPath;

    m_stcCombo->blockSignals(true);
    m_stcCombo->addItem(displayName, pair);
    m_stcCombo->setEnabled(m_stcCombo->count() > 1);
    m_stcCombo->blockSignals(false);

    if (activate) {
        m_stcCombo->setCurrentIndex(m_stcCombo->count() - 1);
        // If this is the first entry, combo didn't fire currentIndexChanged (was -1 → 0),
        // so we need to trigger the load manually via the signal
        if (m_stcCombo->count() == 1) {
            emit m_stcCombo->currentIndexChanged(0);
        }
    }
}

//=============================================================================================================

void MainWindow::loadHemisphere(const QString &subjectPath, const QString &subjectName, const QString &hemi)
{
    QStringList types = {"pial", "inflated", "white"};
    for (const auto &type : types) {
        QString surfPath = subjectPath + "/" + subjectName + "/surf/" + hemi + "." + type;
        if (!QFile::exists(surfPath)) {
            continue;
        }
        Surface surf(surfPath);
        if (!surf.isEmpty()) {
            m_model->addSurface(subjectName, hemi, type, surf);
            qDebug() << "Added" << hemi << type;
        }
    }

    // Load Atlas (Annotation)
    QString annotPath = subjectPath + "/" + subjectName + "/label/" + hemi + ".aparc.annot";
    if (QFile::exists(annotPath)) {
        Annotation annot(annotPath);
        if (!annot.isEmpty()) {
            m_model->addAnnotation(subjectName, hemi, annot);
            qDebug() << "Added annotation for" << hemi;
        }
    }
}

//=============================================================================================================

void MainWindow::loadBem(const QString &subjectName, const QString &bemPath)
{
    QFile bemFile(bemPath);
    if (bemFile.exists()) {
        MNEBem bem(bemFile);
        for (int i = 0; i < bem.size(); ++i) {
            QString name;
            switch (bem[i].id) {
                case 4: name = "head"; break;
                case 3: name = "outer_skull"; break;
                case 1: name = "inner_skull"; break;
                default: name = QString("%1").arg(i); break;
            }
            m_model->addBemSurface(subjectName, name, bem[i]);
            qDebug() << "Added BEM:" << name;
        }
    } else {
        qDebug() << "BEM path provided but file not found:" << bemPath;
    }
}

//=============================================================================================================

void MainWindow::enableNetworkControls()
{
    m_showNetworkCheck->setEnabled(true);
    m_showNetworkCheck->setChecked(true);
    m_networkThresholdSlider->setEnabled(true);
    m_networkColormapCombo->setEnabled(true);
}
//=============================================================================================================

void MainWindow::syncUIToEditTarget(int target)
{
    // Sync the surface, shader, overlay, and BEM shader combos to reflect
    // the settings of the currently selected edit-target viewport.
    // Block signals to avoid recursive updates.

    m_surfCombo->blockSignals(true);
    m_surfCombo->setCurrentText(m_brainView->activeSurfaceForTarget(target));
    m_surfCombo->blockSignals(false);

    m_shaderCombo->blockSignals(true);
    m_shaderCombo->setCurrentText(m_brainView->shaderModeForTarget(target));
    m_shaderCombo->blockSignals(false);

    m_bemShaderCombo->blockSignals(true);
    m_bemShaderCombo->setCurrentText(m_brainView->bemShaderModeForTarget(target));
    m_bemShaderCombo->blockSignals(false);

    m_overlayCombo->blockSignals(true);
    m_overlayCombo->setCurrentText(m_brainView->overlayModeForTarget(target));
    m_overlayCombo->blockSignals(false);

    // Sync camera preset combo
    if (target >= 0) {
        m_cameraPresetCombo->blockSignals(true);
        m_cameraPresetCombo->setCurrentIndex(m_brainView->viewportCameraPreset(target));
        m_cameraPresetCombo->blockSignals(false);
    }

    // Sync hemisphere/BEM/dipole/network visibility from the target viewport
    m_lhCheck->blockSignals(true);
    m_lhCheck->setChecked(m_brainView->objectVisibleForTarget("lh", target));
    m_lhCheck->blockSignals(false);

    m_rhCheck->blockSignals(true);
    m_rhCheck->setChecked(m_brainView->objectVisibleForTarget("rh", target));
    m_rhCheck->blockSignals(false);

    m_headCheck->blockSignals(true);
    m_headCheck->setChecked(m_brainView->objectVisibleForTarget("bem_head", target));
    m_headCheck->blockSignals(false);

    m_outerCheck->blockSignals(true);
    m_outerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_outer_skull", target));
    m_outerCheck->blockSignals(false);

    m_innerCheck->blockSignals(true);
    m_innerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_inner_skull", target));
    m_innerCheck->blockSignals(false);

    // Sync sensor visibility
    m_showMegCheck->blockSignals(true);
    m_showMegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg", target));
    m_showMegCheck->blockSignals(false);

    m_showEegCheck->blockSignals(true);
    m_showEegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_eeg", target));
    m_showEegCheck->blockSignals(false);

    m_showHelmetCheck->blockSignals(true);
    m_showHelmetCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg_helmet", target));
    m_showHelmetCheck->blockSignals(false);

    // Sync digitizer visibility
    m_showDigCheck->blockSignals(true);
    m_showDigCheck->setChecked(m_brainView->objectVisibleForTarget("dig", target));
    m_showDigCheck->blockSignals(false);

    m_showDigCardinalCheck->blockSignals(true);
    m_showDigCardinalCheck->setChecked(m_brainView->objectVisibleForTarget("dig_cardinal", target));
    m_showDigCardinalCheck->blockSignals(false);

    m_showDigHpiCheck->blockSignals(true);
    m_showDigHpiCheck->setChecked(m_brainView->objectVisibleForTarget("dig_hpi", target));
    m_showDigHpiCheck->blockSignals(false);

    m_showDigEegCheck->blockSignals(true);
    m_showDigEegCheck->setChecked(m_brainView->objectVisibleForTarget("dig_eeg", target));
    m_showDigEegCheck->blockSignals(false);

    m_showDigExtraCheck->blockSignals(true);
    m_showDigExtraCheck->setChecked(m_brainView->objectVisibleForTarget("dig_extra", target));
    m_showDigExtraCheck->blockSignals(false);

    // Sync field map / contour visibility
    m_showMegFieldCheck->blockSignals(true);
    m_showMegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_meg", target));
    m_showMegFieldCheck->blockSignals(false);

    m_showEegFieldCheck->blockSignals(true);
    m_showEegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_eeg", target));
    m_showEegFieldCheck->blockSignals(false);

    m_showMegContourCheck->blockSignals(true);
    m_showMegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_meg", target));
    m_showMegContourCheck->blockSignals(false);

    m_showEegContourCheck->blockSignals(true);
    m_showEegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_eeg", target));
    m_showEegContourCheck->blockSignals(false);

    // Sync MEG field map surface (helmet vs head)
    m_megHelmetCombo->blockSignals(true);
    m_megHelmetCombo->setCurrentIndex(m_brainView->megFieldMapOnHeadForTarget(target) ? 1 : 0);
    m_megHelmetCombo->blockSignals(false);

    // Sync dipole, source space, network visibility
    m_showDipoleCheck->blockSignals(true);
    m_showDipoleCheck->setChecked(m_brainView->objectVisibleForTarget("dipoles", target));
    m_showDipoleCheck->blockSignals(false);

    m_showSrcSpaceCheck->blockSignals(true);
    m_showSrcSpaceCheck->setChecked(m_brainView->objectVisibleForTarget("source_space", target));
    m_showSrcSpaceCheck->blockSignals(false);

    m_showNetworkCheck->blockSignals(true);
    m_showNetworkCheck->setChecked(m_brainView->objectVisibleForTarget("network", target));
    m_showNetworkCheck->blockSignals(false);
}

//=============================================================================================================

void MainWindow::updateViewportCheckboxes(int count)
{
    Q_UNUSED(count);
}