//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2026
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
 * @brief    Brain View example application.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QCommandLineParser>
#include <QDebug>
#include <QTimer>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QScrollArea>

#include "brainview.h"
#include "model/braintreemodel.h"
#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <mne/mne_bem.h>

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QCommandLineParser parser;
    parser.setApplicationDescription("QRhi Brain View");
    parser.addHelpOption();
    
    QCommandLineOption subjectPathOption("subjectPath", "Path", "path", 
        QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption subjectOption("subject", "Subject", "name", "sample");
    QCommandLineOption hemiOption("hemi", "Hemi", "hemi", "0");
    QCommandLineOption bemOption("bem", "BEM File", "path", "");
    QCommandLineOption transOption("trans", "Transformation File", "path", "");
    parser.addOptions({subjectPathOption, subjectOption, hemiOption, bemOption, transOption});
    parser.process(app);
    
    QString subPath = parser.value(subjectPathOption);
    QString subName = parser.value(subjectOption);
    int hemi = parser.value(hemiOption).toInt();
    QString bemPath = parser.value(bemOption);
    QString transPath = parser.value(transOption);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("MNE-CPP Brain View (Modular)");
    
    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainWindow.setCentralWidget(centralWidget);

    // Side Panel (Controls) with Scroll Area
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFixedWidth(270); // Slightly wider to account for scrollbar
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *sidePanel = new QWidget;
    sidePanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(5, 5, 5, 5);
    sideLayout->setSpacing(5);
    scrollArea->setWidget(sidePanel);
    
    QGroupBox *controlGroup = new QGroupBox("Controls");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    controlLayout->setContentsMargins(10, 15, 10, 10);
    controlLayout->setSpacing(8);
    
    // Controls: Surface Selector
    QLabel *surfLabel = new QLabel("Surface Type:");
    QComboBox *surfCombo = new QComboBox;
    surfCombo->addItems({"pial", "inflated", "white"});
    
    // Controls: Shader Mode (Material)
    QLabel *shaderLabel = new QLabel("Brain Shader:");
    QComboBox *shaderCombo = new QComboBox();
    shaderCombo->addItem("Standard");
    shaderCombo->addItem("Holographic");
    shaderCombo->addItem("Glossy Realistic");

    QLabel *bemShaderLabel = new QLabel("Head Shader:");
    QComboBox *bemShaderCombo = new QComboBox();
    bemShaderCombo->addItem("Standard");
    bemShaderCombo->addItem("Holographic");
    bemShaderCombo->addItem("Glossy Realistic");
    bemShaderCombo->setEnabled(false); // Disabled by default as linked

    QCheckBox *linkShadersCheck = new QCheckBox("Link Shaders");
    linkShadersCheck->setChecked(true);
    linkShadersCheck->setToolTip("When checked, Head Shader follows Brain Shader selection.");

    QCheckBox *bemColorCheck = new QCheckBox("Show BEM Colors");
    bemColorCheck->setChecked(true);
    bemColorCheck->setToolTip("Toggle between standard Red/Green/Blue colors and White/Overlay colors.");
    
    // Ensure group box itself doesn't have too much margin
    controlGroup->setFlat(true);
    
    // Controls: Overlay (Data)
    QLabel *overlayLabel = new QLabel("Overlay:");
    QComboBox *overlayCombo = new QComboBox();
    overlayCombo->addItem("Surface");
    overlayCombo->addItem("Annotation");
    overlayCombo->addItem("Scientific");
    
    QCheckBox *lhCheck = new QCheckBox("Left Hemisphere");
    lhCheck->setChecked(true);
    
    QCheckBox *rhCheck = new QCheckBox("Right Hemisphere");
    rhCheck->setChecked(true);
    
    // BEM Checks
    QCheckBox *headCheck = new QCheckBox("Head Surface");
    headCheck->setChecked(true);
    QCheckBox *outerCheck = new QCheckBox("Outer Skull");
    outerCheck->setChecked(true);
    QCheckBox *innerCheck = new QCheckBox("Inner Skull");
    innerCheck->setChecked(true);
    
    controlLayout->addWidget(surfLabel);
    controlLayout->addWidget(surfCombo);
    controlLayout->addWidget(overlayLabel);
    controlLayout->addWidget(overlayCombo);
    controlLayout->addWidget(shaderLabel);
    controlLayout->addWidget(shaderCombo);
    controlLayout->addWidget(linkShadersCheck);
    controlLayout->addWidget(bemColorCheck);
    controlLayout->addWidget(bemShaderLabel);
    controlLayout->addWidget(bemShaderCombo);
    controlLayout->addWidget(lhCheck);
    controlLayout->addWidget(rhCheck);
    
    controlLayout->addWidget(new QLabel("------- BEM -------"));
    // bemColorCheck moved above
    controlLayout->addWidget(headCheck);
    controlLayout->addWidget(outerCheck);
    controlLayout->addWidget(innerCheck);
    
    controlLayout->addStretch();
    
    // Source Estimate Group
    QGroupBox *stcGroup = new QGroupBox("Source Estimate");
    QVBoxLayout *stcLayout = new QVBoxLayout(stcGroup);
    stcLayout->setContentsMargins(10, 15, 10, 10);
    stcLayout->setSpacing(8);
    
    QPushButton *loadStcBtn = new QPushButton("Load STC...");
    
    QLabel *colormapLabel = new QLabel("Colormap:");
    QComboBox *colormapCombo = new QComboBox();
    colormapCombo->addItems({"Hot", "Jet", "Viridis", "Cool", "RedBlue", "Bone"});
    
    QLabel *timeLabel = new QLabel("Time: 0.000 s");
    QSlider *timeSlider = new QSlider(Qt::Horizontal);
    timeSlider->setEnabled(false);
    
    QLabel *threshLabel = new QLabel("Thresholds:");
    QHBoxLayout *threshLayout = new QHBoxLayout();
    QDoubleSpinBox *minThresh = new QDoubleSpinBox();
    QDoubleSpinBox *midThresh = new QDoubleSpinBox();
    QDoubleSpinBox *maxThresh = new QDoubleSpinBox();
    minThresh->setDecimals(6);
    midThresh->setDecimals(6);
    maxThresh->setDecimals(6);
    minThresh->setRange(0, 1e12);
    midThresh->setRange(0, 1e12);
    maxThresh->setRange(0, 1e12);
    
    minThresh->setValue(0.0);
    midThresh->setValue(0.5);
    maxThresh->setValue(10.0); // Using 10.0 as a reasonable initial max for visualization
    
    QGridLayout *threshGrid = new QGridLayout();
    threshGrid->addWidget(new QLabel("Min"), 0, 0);
    threshGrid->addWidget(minThresh, 0, 1);
    threshGrid->addWidget(new QLabel("Mid"), 1, 0);
    threshGrid->addWidget(midThresh, 1, 1);
    threshGrid->addWidget(new QLabel("Max"), 2, 0);
    threshGrid->addWidget(maxThresh, 2, 1);

    // Playback Controls
    QLabel *playbackLabel = new QLabel("Playback:");
    QHBoxLayout *playbackLayout = new QHBoxLayout();
    QPushButton *playButton = new QPushButton("Play");
    QComboBox *speedCombo = new QComboBox();
    speedCombo->addItem("0.1x", 0.1);
    speedCombo->addItem("0.25x", 0.25);
    speedCombo->addItem("0.5x", 0.5);
    speedCombo->addItem("1.0x", 1.0);
    speedCombo->addItem("2.0x", 2.0);
    speedCombo->setCurrentIndex(3); // Default 1.0x

    playbackLayout->addWidget(playButton);
    playbackLayout->addWidget(speedCombo);
    
    // Sensor Controls
    QGroupBox *sensorGroup = new QGroupBox("Sensors");
    QVBoxLayout *sensorLayout = new QVBoxLayout(sensorGroup);
    sensorLayout->setContentsMargins(10, 15, 10, 10);
    sensorLayout->setSpacing(8);
    
    QPushButton *loadDigBtn = new QPushButton("Load Digitizer...");
    QPushButton *loadTransBtn = new QPushButton("Load Transformation...");
    
    QCheckBox *showMegCheck = new QCheckBox("Show MEG");
    showMegCheck->setChecked(true);
    showMegCheck->setEnabled(false); // Disabled until loaded
    
    QCheckBox *showEegCheck = new QCheckBox("Show EEG");
    showEegCheck->setChecked(true);
    showEegCheck->setEnabled(false);
    
    QCheckBox *showDigCheck = new QCheckBox("Show Digitizer");
    showDigCheck->setChecked(true);
    showDigCheck->setEnabled(false);
    
    sensorLayout->addWidget(loadDigBtn);
    sensorLayout->addWidget(loadTransBtn);
    sensorLayout->addWidget(showMegCheck);
    sensorLayout->addWidget(showEegCheck);
    sensorLayout->addWidget(showDigCheck);
    
    QTimer *stcTimer = new QTimer();
    
    stcLayout->addWidget(loadStcBtn);
    stcLayout->addWidget(colormapLabel);
    stcLayout->addWidget(colormapCombo);
    stcLayout->addWidget(threshLabel);
    stcLayout->addLayout(threshGrid);
    stcLayout->addWidget(playbackLabel);
    stcLayout->addLayout(playbackLayout);
    stcLayout->addWidget(timeSlider);
    stcLayout->addWidget(timeLabel);
    stcLayout->addStretch();
    
    sideLayout->addWidget(controlGroup);
    sideLayout->addWidget(stcGroup);
    
    // Dipole Group
    QGroupBox *dipoleGroup = new QGroupBox("Dipoles");
    QVBoxLayout *dipoleLayout = new QVBoxLayout(dipoleGroup);
    dipoleLayout->setContentsMargins(10, 15, 10, 10);
    dipoleLayout->setSpacing(8);
    QPushButton *loadDipoleBtn = new QPushButton("Load Dipoles...");
    QCheckBox *showDipoleCheck = new QCheckBox("Show Dipoles");
    showDipoleCheck->setChecked(true);
    showDipoleCheck->setEnabled(false);
    dipoleLayout->addWidget(loadDipoleBtn);
    dipoleLayout->addWidget(showDipoleCheck);
    
    sideLayout->addWidget(dipoleGroup);
    sideLayout->addWidget(sensorGroup);
    sideLayout->addStretch();
    
    // Brain View Widget
    BrainView *brainView = new BrainView();
    // Explicit cast to QObject* to avoid ambiguity if any
    BrainTreeModel *model = new BrainTreeModel(static_cast<QObject*>(brainView));
    brainView->setModel(model);
    
    // Debug: Blue background is set in BrainRenderer::beginFrame directly
    
    brainView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Initial Load
    qDebug() << "Loading surfaces...";
    auto loadHemi = [&](const QString &hemi) {
        QStringList types = {"pial", "inflated", "white"};
        for(const auto &type : types) {
             FSLIB::Surface surf(subPath + "/" + subName + "/surf/" + hemi + "." + type);
             if (!surf.isEmpty()) {
                 model->addSurface(subName, hemi, type, surf);
                 qDebug() << "Added " << hemi << type;
             }
        }
        
        // Load Atlas (Annotation)
        QString annotPath = subPath + "/" + subName + "/label/" + hemi + ".aparc.annot";
        FSLIB::Annotation annot(annotPath);
        if (!annot.isEmpty()) {
            model->addAnnotation(subName, hemi, annot);
            qDebug() << "Added annotation for " << hemi;
        }
    };

    loadHemi("lh");
    loadHemi("rh");
    
    // Load BEM
    if (!bemPath.isEmpty()) {
        QFile bemFile(bemPath);
        if (bemFile.exists()) {
            MNELIB::MNEBem bem(bemFile);
            for(int i=0; i<bem.size(); ++i) {
                QString name;
                // IDs: 4=Head, 3=Outer Skull, 1=Inner Skull (Standard MNE/FS)
                switch(bem[i].id) {
                    case 4: name = "bem_head"; break;        // FIFFV_BEM_SURF_ID_HEAD
                    case 3: name = "bem_outer_skull"; break; // FIFFV_BEM_SURF_ID_OUTER_SKULL
                    case 1: name = "bem_inner_skull"; break; // FIFFV_BEM_SURF_ID_INNER_SKULL
                    default: name = QString("bem_%1").arg(i); break;
                }
                model->addBemSurface(subName, name, bem[i]);
                qDebug() << "Added BEM:" << name;
            }
        } else {
             qDebug() << "BEM path provided but file not found:" << bemPath;
        }
    }
    
    // Load Transformation (if provided)
    /*
    if (!transPath.isEmpty()) {
        brainView->loadTransformation(transPath);
    }
    */

    // Set initial mode to Annotation (Scientific implies this too if chosen)
    // The UI default for overlayCombo is "Surface" (index 0). 
    // So logic will follow UI.
    qDebug() << "Surfaces loaded.";

    // Connections
    QObject::connect(surfCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setActiveSurface);
    
    // Handle Inflated Surface Logic (Disable BEM)
    QObject::connect(surfCombo, &QComboBox::currentTextChanged, [=](const QString &text){
        bool isInflated = (text == "inflated");
        
        if (isInflated) {
            // Uncheck to hide
            headCheck->setChecked(false);
            outerCheck->setChecked(false);
            innerCheck->setChecked(false);
            
            // Disable
            headCheck->setEnabled(false);
            outerCheck->setEnabled(false);
            innerCheck->setEnabled(false);
        } else {
            // Enable
            headCheck->setEnabled(true);
            outerCheck->setEnabled(true);
            innerCheck->setEnabled(true);
        }
    });
    
    // Shader Connections
    // Brain Shader updates model
    QObject::connect(shaderCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setShaderMode);
    // Also update BEM shader if linked
    QObject::connect(shaderCombo, &QComboBox::currentTextChanged, [=](const QString &text){
        if (linkShadersCheck->isChecked()) {
            bemShaderCombo->setCurrentText(text);
        }
    });

    // BEM Shader updates BEM model
    QObject::connect(bemShaderCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setBemShaderMode);
    
    // Link Checkbox Logic
    QObject::connect(linkShadersCheck, &QCheckBox::toggled, [=](bool checked){
        bemShaderCombo->setEnabled(!checked);
        if (checked) {
            bemShaderCombo->setCurrentText(shaderCombo->currentText());
        }
    });
    QObject::connect(overlayCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setVisualizationMode);
    
    QObject::connect(lhCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setHemiVisible(0, checked);
    });
    QObject::connect(rhCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setHemiVisible(1, checked);
    });
    
    QObject::connect(headCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setBemVisible("head", checked);
    });
    QObject::connect(outerCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setBemVisible("outer_skull", checked);
    });
    QObject::connect(innerCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setBemVisible("inner_skull", checked);
    });
    QObject::connect(bemColorCheck, &QCheckBox::toggled, brainView, &BrainView::setBemHighContrast);
    
    // Source Estimate Connections
    QObject::connect(loadStcBtn, &QPushButton::clicked, [&](){
        QString path = QFileDialog::getOpenFileName(nullptr, "Select Source Estimate", "", "STC Files (*-lh.stc *-rh.stc *.stc)");
        if (path.isEmpty()) return;
        
        QString lhPath;
        QString rhPath;
        
        // Detect hemisphere and potential sibling
        if (path.contains("-lh.stc")) {
            lhPath = path;
            QString sibling = path;
            sibling.replace("-lh.stc", "-rh.stc");
            if (QFile::exists(sibling)) rhPath = sibling;
        } else if (path.contains("-rh.stc")) {
            rhPath = path;
            QString sibling = path;
            sibling.replace("-rh.stc", "-lh.stc");
            if (QFile::exists(sibling)) lhPath = sibling;
        } else {
            // Default behavior if not strictly named -lh/-rh
            // Try to assume it's LH and check for RH?
            // Or just try to load this one file as LH? 
            // In strict STC world, filename usually ends in -lh by convention for loading.
            // But if user picks 'test.stc', assume it's a single file.
            // Let's assume it is LH for now or try to match pattern.
            // Actually, BrainView needs explicit LH/RH paths to know where to put it.
            // If the user loads a file that doesn't follow convention, we might not know the hemi.
            // But standard MNE-C/Python uses -lh.stc.
            // Let's try to infer. If fail, maybe force LH?
            qWarning() << "Selected file does not follow -lh/-rh convention. Assuming it is LH and checking for RH pattern if applicable.";
            lhPath = path; 
        }
        
        brainView->loadSourceEstimate(lhPath, rhPath);
    });
    
    QObject::connect(brainView, &BrainView::sourceEstimateLoaded, [&](int numPoints){
        timeSlider->setEnabled(true);
        timeSlider->setRange(0, numPoints - 1);
        timeSlider->setValue(0);
        timeSlider->setEnabled(true);
        timeSlider->setRange(0, numPoints - 1);
        timeSlider->setValue(0);
        overlayCombo->addItem("Source Estimate");
        
        // Reset playback
        playButton->setText("Play");
        stcTimer->stop();
        
        // Calculate interval
        float tstep = brainView->stcStep();
        if (tstep > 0) {
            double factor = speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            stcTimer->setInterval(interval);
        }
    });
    
    QObject::connect(timeSlider, &QSlider::valueChanged, brainView, &BrainView::setTimePoint);
    
    QObject::connect(brainView, &BrainView::timePointChanged, [&](int /*idx*/, float time){
        timeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));
    });
    
    QObject::connect(colormapCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setSourceColormap);
    
    auto updateThresholds = [&](){
        brainView->setSourceThresholds(
            static_cast<float>(minThresh->value()),
            static_cast<float>(midThresh->value()),
            static_cast<float>(maxThresh->value())
        );
    };
    QObject::connect(minThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    QObject::connect(midThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    QObject::connect(maxThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);

    // Playback Logic
    QObject::connect(playButton, &QPushButton::clicked, [=](){
        if (stcTimer->isActive()) {
            stcTimer->stop();
            playButton->setText("Play");
        } else {
            stcTimer->start();
            playButton->setText("Pause");
        }
    });
    
    QObject::connect(speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){
        float tstep = brainView->stcStep();
        if (tstep > 0) {
            double factor = speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            stcTimer->setInterval(interval);
        }
    });

    QObject::connect(stcTimer, &QTimer::timeout, [=](){
        int next = timeSlider->value() + 1;
        if (next >= timeSlider->maximum()) {
            next = 0; // Loop
        }
        timeSlider->setValue(next);
    });
    
    // Sensor Connections
    QObject::connect(loadDigBtn, &QPushButton::clicked, [&](){
        QString path = QFileDialog::getOpenFileName(nullptr, "Select Sensor/Digitizer File", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        
        if (brainView->loadSensors(path)) {
            showMegCheck->setEnabled(true);
            showEegCheck->setEnabled(true);
            showDigCheck->setEnabled(true);
        }
    });
    
    QObject::connect(loadTransBtn, &QPushButton::clicked, [&](){
        QString path = QFileDialog::getOpenFileName(nullptr, "Select Transformation", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        
        brainView->loadTransformation(path);
    });

    QObject::connect(showMegCheck, &QCheckBox::toggled, [=](bool checked){
        brainView->setSensorVisible("MEG", checked);
    });
    QObject::connect(showEegCheck, &QCheckBox::toggled, [=](bool checked){
        brainView->setSensorVisible("EEG", checked);
    });
    QObject::connect(showDigCheck, &QCheckBox::toggled, [=](bool checked){
        brainView->setSensorVisible("Digitizer", checked);
    });

    // Dipole Connections
    QObject::connect(loadDipoleBtn, &QPushButton::clicked, [&](){
        QString path = QFileDialog::getOpenFileName(nullptr, "Select Dipoles", "", "Dipole Files (*.dip *.bdip)");
        if (path.isEmpty()) return;
        
        if (brainView->loadDipoles(path)) {
            showDipoleCheck->setEnabled(true);
        }
    });

    QObject::connect(showDipoleCheck, &QCheckBox::toggled, [=](bool checked){
        brainView->setDipoleVisible(checked);
    });

    // Sync initial state
    brainView->setHemiVisible(0, lhCheck->isChecked());
    brainView->setHemiVisible(1, rhCheck->isChecked());
    brainView->setBemVisible("head", headCheck->isChecked());
    brainView->setBemVisible("outer_skull", outerCheck->isChecked());
    brainView->setBemVisible("inner_skull", innerCheck->isChecked());
    
    mainLayout->addWidget(scrollArea);
    // BrainView inherits QRhiWidget (Qt 6.7+) -> QWidget
    mainLayout->addWidget(brainView);
    
    mainWindow.resize(1200, 800);
    mainWindow.show();

    return app.exec();
}
