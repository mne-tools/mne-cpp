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

#include "brainview.h"

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
    parser.addOptions({subjectPathOption, subjectOption, hemiOption});
    parser.process(app);
    
    QString subPath = parser.value(subjectPathOption);
    QString subName = parser.value(subjectOption);
    int hemi = parser.value(hemiOption).toInt();

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("MNE-CPP Brain View (Modular)");
    
    QWidget *centralWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainWindow.setCentralWidget(centralWidget);

    // Side Panel (Controls)
    QWidget *sidePanel = new QWidget;
    sidePanel->setFixedWidth(250);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    
    QGroupBox *controlGroup = new QGroupBox("Controls");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // Controls: Surface Selector
    QLabel *surfLabel = new QLabel("Surface Type:");
    QComboBox *surfCombo = new QComboBox;
    surfCombo->addItems({"pial", "inflated", "white"});
    
    // Controls: Shader Mode (Material)
    QLabel *shaderLabel = new QLabel("Shader:");
    QComboBox *shaderCombo = new QComboBox();
    shaderCombo->addItem("Standard");
    shaderCombo->addItem("Holographic");
    shaderCombo->addItem("Glossy Realistic");
    
    // Controls: Overlay (Data)
    QLabel *overlayLabel = new QLabel("Overlay:");
    QComboBox *overlayCombo = new QComboBox();
    overlayCombo->addItem("Surface");
    overlayCombo->addItem("Annotation");
    overlayCombo->addItem("Scientific");
    
    QCheckBox *lhCheck = new QCheckBox("Left Hemisphere");
    lhCheck->setChecked(true);
    // lhCheck->setStyleSheet("color: white;"); // Use default style for consistency
    
    QCheckBox *rhCheck = new QCheckBox("Right Hemisphere");
    rhCheck->setChecked(true);
    
    controlLayout->addWidget(surfLabel);
    controlLayout->addWidget(surfCombo);
    controlLayout->addWidget(overlayLabel);
    controlLayout->addWidget(overlayCombo);
    controlLayout->addWidget(shaderLabel);
    controlLayout->addWidget(shaderCombo);
    controlLayout->addWidget(lhCheck);
    controlLayout->addWidget(rhCheck);
    controlLayout->addStretch();
    
    sideLayout->addWidget(controlGroup);
    sideLayout->addStretch();
    
    // Brain View Widget
    BrainView *brainView = new BrainView();
    brainView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Initial Load
    qDebug() << "Loading surfaces...";
    // LH
    brainView->loadSurface(subPath, subName, "lh", "pial");
    brainView->loadSurface(subPath, subName, "lh", "inflated");
    brainView->loadSurface(subPath, subName, "lh", "white");
    // brainView->loadAtlas(subPath, subName, "lh"); // Moved to end
    
    // RH
    brainView->loadSurface(subPath, subName, "rh", "pial");
    brainView->loadSurface(subPath, subName, "rh", "inflated");
    brainView->loadSurface(subPath, subName, "rh", "white");

    // Load Atlases (Annotations) after all surfaces are loaded
    brainView->loadAtlas(subPath, subName, "lh");
    brainView->loadAtlas(subPath, subName, "rh");

    // Set initial mode to Annotation (Scientific implies this too if chosen)
    // The UI default for overlayCombo is "Surface" (index 0). 
    // So logic will follow UI.
    qDebug() << "Surfaces loaded.";

    // Connections
    QObject::connect(surfCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setActiveSurface);
    QObject::connect(shaderCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setShaderMode);
    QObject::connect(overlayCombo, &QComboBox::currentTextChanged, brainView, &BrainView::setVisualizationMode);
    
    QObject::connect(lhCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setHemiVisible(0, checked);
    });
    QObject::connect(rhCheck, &QCheckBox::toggled, [brainView](bool checked){
        brainView->setHemiVisible(1, checked);
    });
    
    // Sync initial state
    brainView->setHemiVisible(0, lhCheck->isChecked());
    brainView->setHemiVisible(1, rhCheck->isChecked());
    
    mainLayout->addWidget(sidePanel);
    mainLayout->addWidget(brainView);
    
    mainWindow.resize(1200, 800);
    mainWindow.show();

    return app.exec();
}
