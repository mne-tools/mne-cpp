//=============================================================================================================
/**
 * @file     neurofeedbacksetupwidget.cpp
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Definition of the NeurofeedbackSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedbacksetupwidget.h"
#include "../neurofeedback.h"

#include "disp/viewers/neurofeedbackcsettingsview.h"
#include "disp/viewers/neurofeedbackfsettingsview.h"
#include "disp/viewers/neurofeedbackbsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QTabWidget>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROFEEDBACKPLUGIN;
using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeurofeedbackSetupWidget::NeurofeedbackSetupWidget(Neurofeedback* pNeurofeedback, const QString& sSettingsPath, QWidget *parent)
: QWidget(parent)
, m_pNeurofeedback(pNeurofeedback)
, m_sSettingsPath(sSettingsPath)
{

    ui.setupUi(this);

    loadSettings(m_sSettingsPath);

    NeurofeedbackCSettingsView* pNeurofeedbackCSettingsView = new NeurofeedbackCSettingsView(QString("MNESCAN/%1/").arg(m_pNeurofeedback->getName()));
    NeurofeedbackFSettingsView* pNeurofeedbackFSettingsView = new NeurofeedbackFSettingsView(QString("MNESCAN/%1/").arg(m_pNeurofeedback->getName()));
    NeurofeedbackBSettingsView* pNeurofeedbackBSettingsView = new NeurofeedbackBSettingsView(QString("MNESCAN/%1/").arg(m_pNeurofeedback->getName()));

    QGridLayout* settingsLayoutN = new QGridLayout;

    QLabel* NeuroOutputLabel = new QLabel("Output");
    QRadioButton* ClassifierButton = new QRadioButton("Classifier");
    QRadioButton* FrequencyButton = new QRadioButton("Frequency");
    QRadioButton* BalloonButton = new QRadioButton("Balloon");

    settingsLayoutN->addWidget(NeuroOutputLabel);
    settingsLayoutN->addWidget(ClassifierButton,0,1);
    settingsLayoutN->addWidget(FrequencyButton,0,2);
    settingsLayoutN->addWidget(BalloonButton,0,3);

    QTabWidget* settingsTab = new QTabWidget;

    settingsTab->addTab(pNeurofeedbackCSettingsView, "Classifier Settings");
    settingsTab->addTab(pNeurofeedbackFSettingsView, "Frequency Settings");
    settingsTab->addTab(pNeurofeedbackBSettingsView, "Balloon Setting");


    settingsLayoutN->addWidget(settingsTab,1,0,1,10);

    QButtonGroup* buttonGroupOutput = new QButtonGroup();
    buttonGroupOutput->addButton(ClassifierButton);
    buttonGroupOutput->addButton(FrequencyButton);
    buttonGroupOutput->addButton(BalloonButton);

    buttonGroupOutput->setId(ClassifierButton, 0);
    buttonGroupOutput->setId(FrequencyButton, 1);
    buttonGroupOutput->setId(BalloonButton, 2);

    if(m_iOutput == 0){
        ClassifierButton->setChecked(true);
    }
    else if(m_iOutput == 1){
        FrequencyButton->setChecked(true);
    }
    else if(m_iOutput == 2){
        BalloonButton->setChecked(true);
    }


    ui.m_qGroupBox_NeurofeedbackOptions->setLayout(settingsLayoutN);


    connect(buttonGroupOutput, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), m_pNeurofeedback, &Neurofeedback::changeOutput);
    connect(buttonGroupOutput, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackSetupWidget::changeiOutput);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeSliders, m_pNeurofeedback, &Neurofeedback::changeSliders);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeballCh, m_pNeurofeedback, &Neurofeedback::changeballCh);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeMax, m_pNeurofeedback, &Neurofeedback::changeFMax);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeMin, m_pNeurofeedback, &Neurofeedback::changeFMin);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeMaxAutoScale, m_pNeurofeedback, &Neurofeedback::changeMaxAutoScale);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeMinAutoScale, m_pNeurofeedback, &Neurofeedback::changeMinAutoScale);
    connect(pNeurofeedbackFSettingsView, &NeurofeedbackFSettingsView::changeResetAutoScale, m_pNeurofeedback, &Neurofeedback::changeResetAutoScale);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeNumbofClass, m_pNeurofeedback, &Neurofeedback::changeNumbofClass);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeClass0, m_pNeurofeedback, &Neurofeedback::changeClass0);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeClass1, m_pNeurofeedback, &Neurofeedback::changeClass1);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeClass2, m_pNeurofeedback, &Neurofeedback::changeClass2);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeClass3, m_pNeurofeedback, &Neurofeedback::changeClass3);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeClass4, m_pNeurofeedback, &Neurofeedback::changeClass4);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeButtonGroupC0, m_pNeurofeedback, &Neurofeedback::changeGBC0);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeButtonGroupC1, m_pNeurofeedback, &Neurofeedback::changeGBC1);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeButtonGroupC2, m_pNeurofeedback, &Neurofeedback::changeGBC2);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeButtonGroupC3, m_pNeurofeedback, &Neurofeedback::changeGBC3);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeButtonGroupC4, m_pNeurofeedback, &Neurofeedback::changeGBC4);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeiClass0, m_pNeurofeedback, &Neurofeedback::changeiClass0);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeiClass1, m_pNeurofeedback, &Neurofeedback::changeiClass1);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeiClass2, m_pNeurofeedback, &Neurofeedback::changeiClass2);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeiClass3, m_pNeurofeedback, &Neurofeedback::changeiClass3);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeiClass4, m_pNeurofeedback, &Neurofeedback::changeiClass4);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeImgClass0, m_pNeurofeedback, &Neurofeedback::changeImgClass0);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeImgClass1, m_pNeurofeedback, &Neurofeedback::changeImgClass1);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeImgClass2, m_pNeurofeedback, &Neurofeedback::changeImgClass2);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeImgClass3, m_pNeurofeedback, &Neurofeedback::changeImgClass3);
    connect(pNeurofeedbackCSettingsView, &NeurofeedbackCSettingsView::changeImgClass4, m_pNeurofeedback, &Neurofeedback::changeImgClass4);
    connect(pNeurofeedbackBSettingsView, &NeurofeedbackBSettingsView::changeMax, m_pNeurofeedback, &Neurofeedback::changeBMax);
    connect(pNeurofeedbackBSettingsView, &NeurofeedbackBSettingsView::changeMin, m_pNeurofeedback, &Neurofeedback::changeBMin);
    connect(pNeurofeedbackBSettingsView, &NeurofeedbackBSettingsView::changeImgBackground, m_pNeurofeedback, &Neurofeedback::changeImgBackground);
    connect(pNeurofeedbackBSettingsView, &NeurofeedbackBSettingsView::changeImgObject, m_pNeurofeedback, &Neurofeedback::changeImgObject);


    pNeurofeedbackCSettingsView->emitSignals();
    pNeurofeedbackBSettingsView->emitSignals();
    pNeurofeedbackFSettingsView->emitSignals();

    ClassifierButton->click();
}

//=============================================================================================================

NeurofeedbackSetupWidget::~NeurofeedbackSetupWidget()
{

}

//=============================================================================================================

void NeurofeedbackSetupWidget::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Store Settings
    QSettings settings;

    settings.setValue(settingsPath + QString("/iOutput"), m_iOutput);

}

//=============================================================================================================

void NeurofeedbackSetupWidget::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings;

    m_iOutput = settings.value(settingsPath + QString("/iOutput"), m_iOutput).toInt();
}

//=============================================================================================================

void NeurofeedbackSetupWidget::changeiOutput(int value)
{
    m_iOutput = value;
    saveSettings(m_sSettingsPath);
}
