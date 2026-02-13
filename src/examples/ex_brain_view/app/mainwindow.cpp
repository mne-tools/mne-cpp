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
 * @brief    MainWindow class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include "../view/brainview.h"
#include "../model/braintreemodel.h"

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
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QScrollArea>
#include <QProgressBar>
#include <QGridLayout>
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSettings>
#include <QRadioButton>
#include <QButtonGroup>
#include <QAbstractButton>

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
    setWindowTitle("MNE-CPP Brain View");

    loadDataPathSettings();

    setupUI();
    setupConnections();

    resize(1200, 800);
}

//=============================================================================================================

void MainWindow::loadDataPathSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup("ex_brain_view/MainWindow");

    const QString defaultPath = settings.value("lastDataPath/default", QString()).toString();
    const QString legacyPath = settings.value("lastDataPath", QString()).toString();

    if (!defaultPath.isEmpty() && QDir(defaultPath).exists()) {
        m_lastDataPath = defaultPath;
    } else if (!legacyPath.isEmpty() && QDir(legacyPath).exists()) {
        m_lastDataPath = legacyPath;
    }
    settings.endGroup();

    if (!m_lastDataPath.isEmpty() && !QDir(m_lastDataPath).exists()) {
        m_lastDataPath.clear();
    }
}

//=============================================================================================================

void MainWindow::rememberDataPath(const QString &category, const QString &filePath)
{
    if (category.isEmpty() || filePath.isEmpty()) {
        return;
    }

    const QString dirPath = QFileInfo(filePath).absolutePath();
    if (dirPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    settings.beginGroup("ex_brain_view/MainWindow");
    settings.setValue(QString("lastDataPath/%1").arg(category), dirPath);
    settings.setValue("lastDataPath/default", dirPath);
    settings.endGroup();

    m_lastDataPath = dirPath;
}

//=============================================================================================================

QString MainWindow::dataDialogStartPath(const QString &category) const
{
    QSettings settings("MNECPP");
    settings.beginGroup("ex_brain_view/MainWindow");
    const QString categoryPath = settings.value(QString("lastDataPath/%1").arg(category), QString()).toString();
    const QString defaultPath = settings.value("lastDataPath/default", QString()).toString();
    settings.endGroup();

    if (!categoryPath.isEmpty() && QDir(categoryPath).exists()) {
        return categoryPath;
    }

    if (!defaultPath.isEmpty() && QDir(defaultPath).exists()) {
        return defaultPath;
    }

    if (!m_lastDataPath.isEmpty() && QDir(m_lastDataPath).exists()) {
        return m_lastDataPath;
    }

    return QDir::homePath();
}

//=============================================================================================================

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Side Panel (Controls) with Scroll Area
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFixedWidth(270);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *sidePanel = new QWidget;
    sidePanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(5, 5, 5, 5);
    sideLayout->setSpacing(5);
    scrollArea->setWidget(sidePanel);

    // (Old controls moved to surfGroup/viewGroup)

    // ===== Brain Surface Group =====
    QGroupBox *surfGroup = new QGroupBox("Brain Surface");
    QVBoxLayout *surfLayout = new QVBoxLayout(surfGroup);
    surfLayout->setContentsMargins(10, 15, 10, 10);
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
    bemLayout->setContentsMargins(10, 15, 10, 10);
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
    stcLayout->setContentsMargins(10, 15, 10, 10);
    stcLayout->setSpacing(8);

    m_loadStcBtn = new QPushButton("Load STC...");

    QLabel *colormapLabel = new QLabel("Colormap:");
    m_colormapCombo = new QComboBox;
    m_colormapCombo->addItems({"MNE", "Hot", "Jet", "Viridis", "Cool", "RedBlue", "Bone"});

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
    stcLayout->addWidget(m_stcStatusLabel);
    stcLayout->addWidget(m_stcProgressBar);
    stcLayout->addWidget(colormapLabel);
    stcLayout->addWidget(m_colormapCombo);
    stcLayout->addWidget(threshLabel);
    stcLayout->addLayout(threshGrid);
    stcLayout->addWidget(playbackLabel);
    stcLayout->addLayout(playbackLayout);
    stcLayout->addWidget(m_timeSlider);
    stcLayout->addWidget(m_timeLabel);
    stcLayout->addStretch();

    // ===== Dipole Group =====
    QGroupBox *dipoleGroup = new QGroupBox("Dipoles");
    QVBoxLayout *dipoleLayout = new QVBoxLayout(dipoleGroup);
    dipoleLayout->setContentsMargins(10, 15, 10, 10);
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
    srcSpaceLayout->setContentsMargins(10, 15, 10, 10);
    srcSpaceLayout->setSpacing(8);

    m_loadSrcSpaceBtn = new QPushButton("Load Source Space...");
    m_showSrcSpaceCheck = new QCheckBox("Show Source Space");
    m_showSrcSpaceCheck->setChecked(false);
    m_showSrcSpaceCheck->setEnabled(false);

    srcSpaceLayout->addWidget(m_loadSrcSpaceBtn);
    srcSpaceLayout->addWidget(m_showSrcSpaceCheck);

    // ===== Sensor Group =====
    QGroupBox *sensorGroup = new QGroupBox("Sensors");
    QVBoxLayout *sensorLayout = new QVBoxLayout(sensorGroup);
    sensorLayout->setContentsMargins(10, 15, 10, 10);
    sensorLayout->setSpacing(8);

    m_loadDigBtn = new QPushButton("Load Digitizer...");
    m_loadTransBtn = new QPushButton("Load Transformation...");
    m_loadSensorDataBtn = new QPushButton("Load Measurement...");
    QLabel *evokedIndexLabel = new QLabel("Evoked Index:");
    m_evokedIndexSpin = new QSpinBox;
    m_evokedIndexSpin->setRange(0, 999);
    m_evokedIndexSpin->setValue(0);

    m_showMegCheck = new QCheckBox("Show MEG");
    m_showMegCheck->setChecked(false);
    m_showMegCheck->setEnabled(false);

    m_showMegGradCheck = new QCheckBox("  Gradiometers");
    m_showMegGradCheck->setChecked(false);
    m_showMegGradCheck->setEnabled(false);

    m_showMegMagCheck = new QCheckBox("  Magnetometers");
    m_showMegMagCheck->setChecked(false);
    m_showMegMagCheck->setEnabled(false);

    m_showMegHelmetCheck = new QCheckBox("  Helmet Surface");
    m_showMegHelmetCheck->setChecked(false);
    m_showMegHelmetCheck->setEnabled(false);

    QLabel *megHelmetLabel = new QLabel("MEG Helmet Surface:");
    m_megHelmetCombo = new QComboBox;
    m_megHelmetCombo->addItem("Auto", "");
    m_megHelmetCombo->setEnabled(false);
    m_megHelmetCombo->setToolTip("Override the auto-selected helmet surface.");

    QDir megHelmetDir(QCoreApplication::applicationDirPath()
        + "/../resources/general/sensorSurfaces");
    if (megHelmetDir.exists()) {
        const QStringList files = megHelmetDir.entryList(QStringList() << "*.fif", QDir::Files, QDir::Name);
        for (const auto &file : files) {
            m_megHelmetCombo->addItem(file, megHelmetDir.absoluteFilePath(file));
        }
    }

    m_showEegCheck = new QCheckBox("Show EEG");
    m_showEegCheck->setChecked(false);
    m_showEegCheck->setEnabled(false);

    m_showDigCheck = new QCheckBox("Show Digitizer");
    m_showDigCheck->setChecked(false);
    m_showDigCheck->setEnabled(false);

    m_showMegFieldCheck = new QCheckBox("  MEG Field Map");
    m_showMegFieldCheck->setChecked(false);
    m_showMegFieldCheck->setEnabled(false);

    m_showEegFieldCheck = new QCheckBox("  EEG Potential Map");
    m_showEegFieldCheck->setChecked(false);
    m_showEegFieldCheck->setEnabled(false);

    m_showMegFieldContoursCheck = new QCheckBox("    MEG Contours");
    m_showMegFieldContoursCheck->setChecked(false);
    m_showMegFieldContoursCheck->setEnabled(false);

    m_showEegFieldContoursCheck = new QCheckBox("    EEG Contours");
    m_showEegFieldContoursCheck->setChecked(false);
    m_showEegFieldContoursCheck->setEnabled(false);

    m_megFieldMapOnHeadCheck = new QCheckBox("    MEG Map on Head Surface");
    m_megFieldMapOnHeadCheck->setChecked(false);
    m_megFieldMapOnHeadCheck->setEnabled(false);

    m_sensorTimeLabel = new QLabel("Sensor Time: 0.000 s");
    m_sensorTimeSlider = new QSlider(Qt::Horizontal);
    m_sensorTimeSlider->setEnabled(false);

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

    m_applyTransCheck = new QCheckBox("Apply Transformation");
    m_applyTransCheck->setChecked(true);
    m_applyTransCheck->setToolTip("Apply Head-to-MRI transformation to sensors if available.");

    sensorLayout->addWidget(m_loadDigBtn);
    sensorLayout->addWidget(m_loadTransBtn);
    sensorLayout->addWidget(m_loadSensorDataBtn);
    sensorLayout->addWidget(evokedIndexLabel);
    sensorLayout->addWidget(m_evokedIndexSpin);
    sensorLayout->addWidget(m_showMegCheck);
    sensorLayout->addWidget(m_showMegGradCheck);
    sensorLayout->addWidget(m_showMegMagCheck);
    sensorLayout->addWidget(m_showMegHelmetCheck);
    sensorLayout->addWidget(megHelmetLabel);
    sensorLayout->addWidget(m_megHelmetCombo);
    sensorLayout->addWidget(m_showEegCheck);
    sensorLayout->addWidget(m_showDigCheck);
    sensorLayout->addWidget(m_showDigCardinalCheck);
    sensorLayout->addWidget(m_showDigHpiCheck);
    sensorLayout->addWidget(m_showDigEegCheck);
    sensorLayout->addWidget(m_showDigExtraCheck);
    sensorLayout->addWidget(m_showMegFieldCheck);
    sensorLayout->addWidget(m_showEegFieldCheck);
    sensorLayout->addWidget(m_showMegFieldContoursCheck);
    sensorLayout->addWidget(m_showEegFieldContoursCheck);
    sensorLayout->addWidget(m_megFieldMapOnHeadCheck);
    sensorLayout->addWidget(m_sensorTimeLabel);
    sensorLayout->addWidget(m_sensorTimeSlider);
    sensorLayout->addWidget(m_applyTransCheck);

    // ===== View Group =====
    QGroupBox *viewGroup = new QGroupBox("View");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewGroup);
    viewLayout->setContentsMargins(10, 15, 10, 10);
    viewLayout->setSpacing(8);

    // Multi-View Toggle
    m_multiViewCheck = new QCheckBox("Multi-View (4 cameras)");
    m_multiViewCheck->setChecked(false);
    m_multiViewCheck->setToolTip("Toggle between single interactive camera and four fixed camera views.");
    viewLayout->addWidget(m_multiViewCheck);

    QLabel *editTargetLabel = new QLabel("Apply Visualization To:");
    viewLayout->addWidget(editTargetLabel);

    m_viewEditTargetGroup = new QButtonGroup(this);
    m_editTopRadio = new QRadioButton("Top");
    m_editPerspectiveRadio = new QRadioButton("Perspective");
    m_editFrontRadio = new QRadioButton("Front");
    m_editLeftRadio = new QRadioButton("Left");

    m_viewEditTargetGroup->addButton(m_editTopRadio, 0);
    m_viewEditTargetGroup->addButton(m_editPerspectiveRadio, 1);
    m_viewEditTargetGroup->addButton(m_editFrontRadio, 2);
    m_viewEditTargetGroup->addButton(m_editLeftRadio, 3);

    m_editPerspectiveRadio->setChecked(true);
    m_editTopRadio->setEnabled(false);
    m_editPerspectiveRadio->setEnabled(false);
    m_editFrontRadio->setEnabled(false);
    m_editLeftRadio->setEnabled(false);

    QHBoxLayout *editRow1 = new QHBoxLayout();
    editRow1->addWidget(m_editTopRadio);
    editRow1->addWidget(m_editPerspectiveRadio);

    QHBoxLayout *editRow2 = new QHBoxLayout();
    editRow2->addWidget(m_editFrontRadio);
    editRow2->addWidget(m_editLeftRadio);

    viewLayout->addLayout(editRow1);
    viewLayout->addLayout(editRow2);

    // Viewport Toggles
    QHBoxLayout *vpRow1 = new QHBoxLayout();
    QHBoxLayout *vpRow2 = new QHBoxLayout();

    m_vpTopCheck = new QCheckBox("Top");
    m_vpBottomCheck = new QCheckBox("Perspective");
    m_vpFrontCheck = new QCheckBox("Front");
    m_vpLeftCheck = new QCheckBox("Left");

    m_vpTopCheck->setChecked(true);
    m_vpBottomCheck->setChecked(true);
    m_vpFrontCheck->setChecked(true);
    m_vpLeftCheck->setChecked(true);

    m_vpTopCheck->setEnabled(false);
    m_vpBottomCheck->setEnabled(false);
    m_vpFrontCheck->setEnabled(false);
    m_vpLeftCheck->setEnabled(false);

    vpRow1->addWidget(m_vpTopCheck);
    vpRow1->addWidget(m_vpBottomCheck);
    vpRow2->addWidget(m_vpFrontCheck);
    vpRow2->addWidget(m_vpLeftCheck);

    viewLayout->addLayout(vpRow1);
    viewLayout->addLayout(vpRow2);

    m_resetMultiViewLayoutBtn = new QPushButton("Reset Multi-View Layout");
    m_resetMultiViewLayoutBtn->setEnabled(false);
    viewLayout->addWidget(m_resetMultiViewLayoutBtn);

    // Info Panel Toggle
    m_showInfoCheck = new QCheckBox("Show Info Panel");
    m_showInfoCheck->setChecked(true);
    m_showInfoCheck->setToolTip("Toggle the FPS / vertex / shader info overlay.");
    viewLayout->addWidget(m_showInfoCheck);

    // Assemble side panel
    sideLayout->addWidget(surfGroup);
    sideLayout->addWidget(bemGroup);
    sideLayout->addWidget(stcGroup);
    sideLayout->addWidget(dipoleGroup);
    sideLayout->addWidget(srcSpaceGroup);
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
    auto refreshVisualizationControls = [this]() {
        const int target = m_brainView->visualizationEditTarget();

        {
            const QSignalBlocker blockSurf(m_surfCombo);
            m_surfCombo->setCurrentText(m_brainView->activeSurfaceForTarget(target));
        }

        {
            const QSignalBlocker blockShader(m_shaderCombo);
            m_shaderCombo->setCurrentText(m_brainView->shaderModeForTarget(target));
        }

        {
            const QSignalBlocker blockOverlay(m_overlayCombo);
            m_overlayCombo->setCurrentText(m_brainView->overlayModeForTarget(target));
        }

        if (m_linkShadersCheck->isChecked()) {
            const QSignalBlocker blockBem(m_bemShaderCombo);
            m_bemShaderCombo->setCurrentText(m_shaderCombo->currentText());
        }

        const bool isInflated = (m_surfCombo->currentText() == "inflated");
        if (isInflated) {
            m_headCheck->setChecked(false);
            m_outerCheck->setChecked(false);
            m_innerCheck->setChecked(false);
        }
        m_headCheck->setEnabled(!isInflated);
        m_outerCheck->setEnabled(!isInflated);
        m_innerCheck->setEnabled(!isInflated);

        m_brainView->setActiveSurface(m_surfCombo->currentText());
        m_brainView->setShaderMode(m_shaderCombo->currentText());
        m_brainView->setVisualizationMode(m_overlayCombo->currentText());
        if (m_linkShadersCheck->isChecked()) {
            m_brainView->setBemShaderMode(m_shaderCombo->currentText());
        } else {
            m_brainView->setBemShaderMode(m_bemShaderCombo->currentText());
        }
    };

    // Surface type
    connect(m_surfCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setActiveSurface(text);
    });

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
    connect(m_shaderCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setShaderMode(text);
    });
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

    connect(m_viewEditTargetGroup, &QButtonGroup::idClicked, [this, refreshVisualizationControls](int id) {
        m_brainView->setVisualizationEditTarget(id);
        refreshVisualizationControls();
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
        QString path = QFileDialog::getOpenFileName(this, "Select BEM Surface", dataDialogStartPath("bem"), "BEM Files (*-bem.fif *-bem-sol.fif);;FIF Files (*.fif);;All Files (*)");
        if (path.isEmpty()) return;
        rememberDataPath("bem", path);
        loadBem("User", path);
    });

    // BEM visibility
    connect(m_headCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("head", checked); });
    connect(m_outerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("outer_skull", checked); });
    connect(m_innerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("inner_skull", checked); });
    connect(m_bemColorCheck, &QCheckBox::toggled, m_brainView, &BrainView::setBemHighContrast);

    // Multi-view
    connect(m_multiViewCheck, &QCheckBox::toggled, [this, refreshVisualizationControls](bool checked) {
        m_vpTopCheck->setEnabled(checked);
        m_vpBottomCheck->setEnabled(checked);
        m_vpFrontCheck->setEnabled(checked);
        m_vpLeftCheck->setEnabled(checked);
        m_resetMultiViewLayoutBtn->setEnabled(checked);

        m_editTopRadio->setEnabled(checked);
        m_editPerspectiveRadio->setEnabled(checked);
        m_editFrontRadio->setEnabled(checked);
        m_editLeftRadio->setEnabled(checked);

        if (checked) {
            int target = m_viewEditTargetGroup->checkedId();
            if (target < 0 || target > 3) {
                target = 1;
                m_editPerspectiveRadio->setChecked(true);
            }
            m_brainView->setVisualizationEditTarget(target);
            m_brainView->showMultiView();
        } else {
            m_brainView->setVisualizationEditTarget(-1);
            m_brainView->showSingleView();
        }

        refreshVisualizationControls();
    });
    connect(m_vpTopCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setViewportEnabled(0, checked); });
    connect(m_vpBottomCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setViewportEnabled(1, checked); });
    connect(m_vpFrontCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setViewportEnabled(2, checked); });
    connect(m_vpLeftCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setViewportEnabled(3, checked); });
    connect(m_resetMultiViewLayoutBtn, &QPushButton::clicked, m_brainView, &BrainView::resetMultiViewLayout);
    connect(m_showInfoCheck, &QCheckBox::toggled, m_brainView, &BrainView::setInfoPanelVisible);

    // Brain Surface
    connect(m_loadSurfaceBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Surface", dataDialogStartPath("surface"), "FreeSurfer Surface (*.pial *.inflated *.white *.orig);;All Files (*)");
        if (path.isEmpty()) return;
        rememberDataPath("surface", path);

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
        QString path = QFileDialog::getOpenFileName(this, "Select Atlas", dataDialogStartPath("atlas"), "FreeSurfer Annotation (*.annot);;All Files (*)");
        if (path.isEmpty()) return;
        rememberDataPath("atlas", path);

        // Guess hemi
        QString fileName = QFileInfo(path).fileName();
        QString hemi = fileName.contains("lh.") ? "lh" : (fileName.contains("rh.") ? "rh" : "lh");

        Annotation annot(path);
        if (!annot.isEmpty()) {
            m_model->addAnnotation("User", hemi, annot);
        }
    });

    // STC loading
    connect(m_loadStcBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Source Estimate", dataDialogStartPath("stc"), "STC Files (*-lh.stc *-rh.stc *.stc)");
        if (path.isEmpty()) return;
        rememberDataPath("stc", path);

        QString lhPath, rhPath;
        if (path.contains("-lh.stc")) {
            lhPath = path;
            QString sibling = path; sibling.replace("-lh.stc", "-rh.stc");
            if (QFile::exists(sibling)) rhPath = sibling;
        } else if (path.contains("-rh.stc")) {
            rhPath = path;
            QString sibling = path; sibling.replace("-rh.stc", "-lh.stc");
            if (QFile::exists(sibling)) lhPath = sibling;
        } else {
            lhPath = path;
        }

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

    connect(m_brainView, &BrainView::sourceThresholdsUpdated, [this](float min, float mid, float max) {
        const QSignalBlocker blockMin(m_minThresh);
        const QSignalBlocker blockMid(m_midThresh);
        const QSignalBlocker blockMax(m_maxThresh);
        m_minThresh->setValue(static_cast<double>(min));
        m_midThresh->setValue(static_cast<double>(mid));
        m_maxThresh->setValue(static_cast<double>(max));
    });

    connect(m_timeSlider, &QSlider::valueChanged, m_brainView, &BrainView::setTimePoint);
    connect(m_brainView, &BrainView::timePointChanged, [this](int, float time) {
        m_timeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));
    });

    connect(m_colormapCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setSourceColormap);
    connect(m_colormapCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setSensorFieldColormap);

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

    // Playback
    connect(m_playButton, &QPushButton::clicked, [this]() {
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
    });

    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        float tstep = m_brainView->stcStep();
        if (tstep > 0) {
            double factor = m_speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            m_stcTimer->setInterval(interval);
        }
    });

    connect(m_stcTimer, &QTimer::timeout, [this]() {
        int next = m_timeSlider->value() + 1;
        if (next >= m_timeSlider->maximum()) next = 0;
        m_timeSlider->setValue(next);
    });

    // Sensors
    connect(m_loadDigBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Sensor/Digitizer File", dataDialogStartPath("digitizer"), "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        rememberDataPath("digitizer", path);

        if (m_brainView->loadSensors(path)) {
            m_lastSensorPath = path;
            m_showMegCheck->setEnabled(true);
            m_showMegGradCheck->setEnabled(false);
            m_showMegMagCheck->setEnabled(false);
            m_showMegHelmetCheck->setEnabled(false);
            m_megHelmetCombo->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            // Sub-category checkboxes stay disabled until master is toggled on
        }
    });

    connect(m_loadTransBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Transformation", dataDialogStartPath("transform"), "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        rememberDataPath("transform", path);
        m_brainView->loadTransformation(path);
    });

    connect(m_loadSensorDataBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Evoked/Average", dataDialogStartPath("measurement"), "Evoked/Average Files (*.fif)");
        if (path.isEmpty()) return;
        rememberDataPath("measurement", path);

        if (m_brainView->loadSensorField(path, m_evokedIndexSpin->value())) {
            m_showMegFieldCheck->setEnabled(true);
            m_showEegFieldCheck->setEnabled(true);
            m_showMegFieldContoursCheck->setEnabled(true);
            m_showEegFieldContoursCheck->setEnabled(true);
            m_megFieldMapOnHeadCheck->setEnabled(true);
            m_sensorTimeSlider->setEnabled(true);
            m_showMegFieldCheck->setChecked(true);
            m_showEegFieldCheck->setChecked(true);
        }
    });

    connect(m_showMegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG", checked); });
    connect(m_showMegCheck, &QCheckBox::toggled, [this](bool checked) {
        m_showMegGradCheck->setEnabled(checked);
        m_showMegMagCheck->setEnabled(checked);
        m_showMegHelmetCheck->setEnabled(checked);
        m_showMegGradCheck->setChecked(checked);
        m_showMegMagCheck->setChecked(checked);
        m_showMegHelmetCheck->setChecked(checked);
    });
    connect(m_showMegGradCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG/Grad", checked); });
    connect(m_showMegMagCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG/Mag", checked); });
    connect(m_showMegHelmetCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG Helmet", checked); });
    connect(m_megHelmetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        const QString overridePath = m_megHelmetCombo->currentData().toString();
        m_brainView->setMegHelmetOverride(overridePath);
        if (!m_lastSensorPath.isEmpty()) {
            if (m_brainView->loadSensors(m_lastSensorPath)) {
                m_brainView->setSensorVisible("MEG", m_showMegCheck->isChecked());
                m_brainView->setSensorVisible("MEG/Grad", m_showMegGradCheck->isChecked());
                m_brainView->setSensorVisible("MEG/Mag", m_showMegMagCheck->isChecked());
                m_brainView->setSensorVisible("MEG Helmet", m_showMegHelmetCheck->isChecked());
                m_brainView->setSensorVisible("EEG", m_showEegCheck->isChecked());
                m_brainView->setSensorVisible("Digitizer", m_showDigCheck->isChecked());
                m_brainView->setSensorVisible("Digitizer/Cardinal", m_showDigCardinalCheck->isChecked());
                m_brainView->setSensorVisible("Digitizer/HPI", m_showDigHpiCheck->isChecked());
                m_brainView->setSensorVisible("Digitizer/EEG", m_showDigEegCheck->isChecked());
                m_brainView->setSensorVisible("Digitizer/Extra", m_showDigExtraCheck->isChecked());
            }
        }
    });
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
    connect(m_applyTransCheck, &QCheckBox::toggled, m_brainView, &BrainView::setSensorTransEnabled);

    connect(m_showMegFieldCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorFieldVisible("MEG", checked); });
    connect(m_showEegFieldCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorFieldVisible("EEG", checked); });
    connect(m_showMegFieldContoursCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorFieldContourVisible("MEG", checked); });
    connect(m_showEegFieldContoursCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorFieldContourVisible("EEG", checked); });
    connect(m_megFieldMapOnHeadCheck, &QCheckBox::toggled, m_brainView, &BrainView::setMegFieldMapOnHead);
    connect(m_sensorTimeSlider, &QSlider::valueChanged, m_brainView, &BrainView::setSensorFieldTimePoint);

    connect(m_brainView, &BrainView::sensorFieldLoaded, [this](int numPoints) {
        m_sensorTimeSlider->setRange(0, numPoints - 1);
        m_sensorTimeSlider->setValue(0);
        m_sensorTimeLabel->setText("Sensor Time: 0.000 s");
    });
    connect(m_brainView, &BrainView::sensorFieldTimePointChanged, [this](int, float time) {
        m_sensorTimeLabel->setText(QString("Sensor Time: %1 s").arg(time, 0, 'f', 3));
    });

    // Dipoles
    connect(m_loadDipoleBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Dipoles", dataDialogStartPath("dipole"), "Dipole Files (*.dip *.bdip)");
        if (path.isEmpty()) return;
        rememberDataPath("dipole", path);
        if (m_brainView->loadDipoles(path)) {
            m_showDipoleCheck->setEnabled(true);
        }
    });

    connect(m_showDipoleCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setDipoleVisible(checked); });

    // Source Space
    connect(m_loadSrcSpaceBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Source Space / Forward Solution", dataDialogStartPath("srcspace"),
            "Source Space Files (*-src.fif *-fwd.fif);;All FIF Files (*.fif)");
        if (path.isEmpty()) return;
        rememberDataPath("srcspace", path);
        if (m_brainView->loadSourceSpace(path)) {
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
    });

    connect(m_showSrcSpaceCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSourceSpaceVisible(checked); });

    // Sync initial state
    m_brainView->setHemiVisible(0, m_lhCheck->isChecked());
    m_brainView->setHemiVisible(1, m_rhCheck->isChecked());
    m_brainView->setBemVisible("head", m_headCheck->isChecked());
    m_brainView->setBemVisible("outer_skull", m_outerCheck->isChecked());
    m_brainView->setBemVisible("inner_skull", m_innerCheck->isChecked());

    const bool isMultiView = (m_brainView->viewMode() == BrainView::MultiView);
    {
        const QSignalBlocker blockMulti(m_multiViewCheck);
        m_multiViewCheck->setChecked(isMultiView);
    }

    m_vpTopCheck->setEnabled(isMultiView);
    m_vpBottomCheck->setEnabled(isMultiView);
    m_vpFrontCheck->setEnabled(isMultiView);
    m_vpLeftCheck->setEnabled(isMultiView);
    m_resetMultiViewLayoutBtn->setEnabled(isMultiView);
    m_editTopRadio->setEnabled(isMultiView);
    m_editPerspectiveRadio->setEnabled(isMultiView);
    m_editFrontRadio->setEnabled(isMultiView);
    m_editLeftRadio->setEnabled(isMultiView);

    {
        const QSignalBlocker blockTop(m_vpTopCheck);
        const QSignalBlocker blockPerspective(m_vpBottomCheck);
        const QSignalBlocker blockFront(m_vpFrontCheck);
        const QSignalBlocker blockLeft(m_vpLeftCheck);
        m_vpTopCheck->setChecked(m_brainView->isViewportEnabled(0));
        m_vpBottomCheck->setChecked(m_brainView->isViewportEnabled(1));
        m_vpFrontCheck->setChecked(m_brainView->isViewportEnabled(2));
        m_vpLeftCheck->setChecked(m_brainView->isViewportEnabled(3));
    }

    int editTarget = m_brainView->visualizationEditTarget();
    if (isMultiView) {
        if (editTarget < 0 || editTarget > 3) {
            editTarget = 1;
        }
        if (QAbstractButton* button = m_viewEditTargetGroup->button(editTarget)) {
            const QSignalBlocker blockTarget(m_viewEditTargetGroup);
            button->setChecked(true);
        }
        m_brainView->setVisualizationEditTarget(editTarget);
    } else {
        m_brainView->setVisualizationEditTarget(-1);
    }
    refreshVisualizationControls();
}

//=============================================================================================================

void MainWindow::loadInitialData(const QString &subjectPath,
                                  const QString &subjectName,
                                  const QString &bemPath,
                                  const QString &transPath,
                                  const QString &stcPath,
                                  const QString &digitizerPath,
                                  const QString &srcSpacePath,
                                  const QString &atlasPath)
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
        rememberDataPath("atlas", atlasPath);
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
        rememberDataPath("bem", bemPath);
        loadBem(subjectName, bemPath);
    }

    // Auto-load digitizer
    if (!digitizerPath.isEmpty() && QFile::exists(digitizerPath)) {
        rememberDataPath("digitizer", digitizerPath);
        qDebug() << "Auto-loading digitizer from:" << digitizerPath;
        if (m_brainView->loadSensors(digitizerPath)) {
            m_lastSensorPath = digitizerPath;
            m_showMegCheck->setEnabled(true);
            m_showMegGradCheck->setEnabled(false);
            m_showMegMagCheck->setEnabled(false);
            m_showMegHelmetCheck->setEnabled(false);
            m_megHelmetCombo->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            // Sub-category checkboxes stay disabled until master is toggled on
            m_brainView->setSensorVisible("MEG", m_showMegCheck->isChecked());
            m_brainView->setSensorVisible("EEG", m_showEegCheck->isChecked());
            m_brainView->setSensorVisible("Digitizer", m_showDigCheck->isChecked());
        }
    }

    // Auto-load transformation
    if (!transPath.isEmpty() && QFile::exists(transPath)) {
        rememberDataPath("transform", transPath);
        qDebug() << "Auto-loading transformation from:" << transPath;
        m_brainView->loadTransformation(transPath);
    }

    // Auto-load source space
    if (!srcSpacePath.isEmpty() && QFile::exists(srcSpacePath)) {
        rememberDataPath("srcspace", srcSpacePath);
        qDebug() << "Auto-loading source space from:" << srcSpacePath;
        if (m_brainView->loadSourceSpace(srcSpacePath)) {
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
    }

    // Auto-load STC
    if (!stcPath.isEmpty() && QFile::exists(stcPath)) {
        rememberDataPath("stc", stcPath);
        qDebug() << "Auto-loading source estimate from:" << stcPath;

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

        m_loadStcBtn->setEnabled(false);
        m_stcStatusLabel->setText("Starting...");
        m_stcStatusLabel->show();
        m_stcProgressBar->setValue(0);
        m_stcProgressBar->show();

        m_brainView->loadSourceEstimate(lhPath, rhPath);
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
