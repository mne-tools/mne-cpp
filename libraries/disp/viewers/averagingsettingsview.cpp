//=============================================================================================================
/**
 * @file     averagingsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the AveragingSettingsView class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsettingsview.h"
#include "ui_averagingsettingsview.h"

#include <fiff/fiff_evoked_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingSettingsView::AveragingSettingsView(const QString& sSettingsPath,
                                             const QMap<QString, int> &mapStimChsIndexNames,
                                             QWidget *parent)
: QWidget(parent)
, ui(new Ui::AverageSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_mapStimChsIndexNames(mapStimChsIndexNames)
{
    ui->setupUi(this);

    this->setWindowTitle("Averaging Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings(m_sSettingsPath);
    redrawGUI();
}


//*************************************************************************************************************

AveragingSettingsView::~AveragingSettingsView()
{
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::setStimChannels(const QMap<QString,int>& mapStimChsIndexNames)
{
    if(!mapStimChsIndexNames.isEmpty()) {
        m_mapStimChsIndexNames = mapStimChsIndexNames;

        ui->m_pComboBoxChSelection->clear();

        QMapIterator<QString, int> i(mapStimChsIndexNames);
        while (i.hasNext()) {
            i.next();
            ui->m_pComboBoxChSelection->insertItem(ui->m_pComboBoxChSelection->count(),i.key());
        }

        ui->m_pComboBoxChSelection->setCurrentText(m_sCurrentStimChan);

        connect(ui->m_pComboBoxChSelection, &QComboBox::currentTextChanged,
                this, &AveragingSettingsView::onChangeStimChannel);
    }
}


//*************************************************************************************************************

QString AveragingSettingsView::getCurrentStimCh()
{
    return m_sCurrentStimChan;
}


//*************************************************************************************************************

bool AveragingSettingsView::getDoBaselineCorrection()
{
    return m_bDoBaselineCorrection;
}


//*************************************************************************************************************

int AveragingSettingsView::getNumAverages()
{
    return m_iNumAverages;
}


//*************************************************************************************************************

int AveragingSettingsView::getBaselineFromSeconds()
{
    return m_iBaselineFromSeconds;
}


//*************************************************************************************************************

int AveragingSettingsView::getBaselineToSeconds()
{
    return m_iBaselineToSeconds;
}


//*************************************************************************************************************

int AveragingSettingsView::getPreStimSeconds()
{
    return m_iPreStimSeconds;
}


//*************************************************************************************************************

int AveragingSettingsView::getPostStimSeconds()
{
    return m_iPostStimSeconds;
}


//*************************************************************************************************************

int AveragingSettingsView::getStimChannelIdx()
{
    return ui->m_pComboBoxChSelection->currentData().toInt();
}


//*************************************************************************************************************

void AveragingSettingsView::redrawGUI()
{
    if(!m_mapStimChsIndexNames.isEmpty()) {
        ui->m_pComboBoxChSelection->clear();

        QMapIterator<QString, int> i(m_mapStimChsIndexNames);
        while (i.hasNext()) {
            i.next();
            ui->m_pComboBoxChSelection->insertItem(ui->m_pComboBoxChSelection->count(),i.key());
        }

        ui->m_pComboBoxChSelection->setCurrentText(m_sCurrentStimChan);

        connect(ui->m_pComboBoxChSelection, &QComboBox::currentTextChanged,
                this, &AveragingSettingsView::changeStimChannel);
    }

    ui->m_pSpinBoxNumAverages->setValue(m_iNumAverages);
    connect(ui->m_pSpinBoxNumAverages, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeNumAverages);

    //Pre Post stimulus
    ui->m_pSpinBoxPreStimMSeconds->setValue(m_iPreStimSeconds);
    connect(ui->m_pSpinBoxPreStimMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePreStim);

    ui->m_pSpinBoxPostStimMSeconds->setValue(m_iPostStimSeconds);
    connect(ui->m_pSpinBoxPostStimMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePostStim);

    //Baseline Correction
    ui->m_pcheckBoxBaselineCorrection->setChecked(m_bDoBaselineCorrection);
    connect(ui->m_pcheckBoxBaselineCorrection, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeBaselineActive);

    ui->m_pSpinBoxBaselineFromMSeconds->setMinimum(ui->m_pSpinBoxPreStimMSeconds->value()*-1);
    ui->m_pSpinBoxBaselineFromMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());
    ui->m_pSpinBoxBaselineFromMSeconds->setValue(m_iBaselineFromSeconds);
    connect(ui->m_pSpinBoxBaselineFromMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineFrom);

    ui->m_pSpinBoxBaselineToMSeconds->setMinimum(ui->m_pSpinBoxPreStimMSeconds->value()*-1);
    ui->m_pSpinBoxBaselineToMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());
    ui->m_pSpinBoxBaselineToMSeconds->setValue(m_iBaselineToSeconds);
    connect(ui->m_pSpinBoxBaselineToMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineTo);

    connect(ui->m_pushButton_reset, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &AveragingSettingsView::resetAverage);

    setWindowFlags(Qt::WindowStaysOnTopHint);

    ui->m_groupBox_detectedTrials->hide();
}


//*************************************************************************************************************

void AveragingSettingsView::setDetectedEpochs(const FiffEvokedSet& evokedSet)
{
    if(evokedSet.evoked.isEmpty()) {
        ui->m_groupBox_detectedTrials->hide();
        return;
    } else {
        ui->m_groupBox_detectedTrials->show();
    }

    QGridLayout* topLayout = static_cast<QGridLayout*>(ui->m_groupBox_detectedTrials->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    topLayout->addWidget(new QLabel("Type"),0,0);
    topLayout->addWidget(new QLabel("#"),0,1);

    for(int i = 0; i < evokedSet.evoked.size(); i++) {
        if(i < 10) {
            // Show only a maximum of 10 average types
            topLayout->addWidget(new QLabel(evokedSet.evoked.at(i).comment),i+1,0);
            topLayout->addWidget(new QLabel(QString::number(evokedSet.evoked.at(i).nave)),i+1,1);
        }
    }

    //Find Filter tab and add current layout
    ui->m_groupBox_detectedTrials->setLayout(topLayout);
}


//*************************************************************************************************************

void AveragingSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Store Settings
    QSettings settings;

    settings.setValue(settingsPath + QString("/preStimSeconds"), m_iPreStimSeconds);
    settings.setValue(settingsPath + QString("/postStimSeconds"), m_iPostStimSeconds);
    settings.setValue(settingsPath + QString("/numAverages"), m_iNumAverages);
    settings.setValue(settingsPath + QString("/currentStimChannel"), m_sCurrentStimChan);
    settings.setValue(settingsPath + QString("/baselineFromSeconds"), m_iBaselineFromSeconds);
    settings.setValue(settingsPath + QString("/baselineToSeconds"), m_iBaselineToSeconds);
    settings.setValue(settingsPath + QString("/doBaselineCorrection"), m_bDoBaselineCorrection);
}


//*************************************************************************************************************

void AveragingSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings;

    m_iPreStimSeconds = settings.value(settingsPath + QString("/preStimSeconds"), 100).toInt();
    m_iPostStimSeconds = settings.value(settingsPath + QString("/postStimSeconds"), 400).toInt();
    m_iBaselineFromSeconds = settings.value(settingsPath + QString("/baselineFromSeconds"), 0).toInt();
    m_iBaselineToSeconds = settings.value(settingsPath + QString("/baselineToSeconds"), 0).toInt();

    if(m_iBaselineFromSeconds < -1 * m_iPreStimSeconds || m_iBaselineFromSeconds > m_iPostStimSeconds) {
        m_iBaselineFromSeconds = -1 * m_iPreStimSeconds;
    }

    if(m_iBaselineToSeconds > m_iPostStimSeconds  || m_iBaselineToSeconds < m_iPreStimSeconds) {
        m_iBaselineToSeconds = 0;
    }

    m_iNumAverages = settings.value(settingsPath + QString("/numAverages"), 10).toInt();
    m_sCurrentStimChan = settings.value(settingsPath + QString("/currentStimChannel"), "STI014").toString();
    m_bDoBaselineCorrection = settings.value(settingsPath + QString("/doBaselineCorrection"), false).toBool();
}


//*************************************************************************************************************

void AveragingSettingsView::onChangePreStim()
{
    qint32 mSeconds = ui->m_pSpinBoxPreStimMSeconds->value();
    ui->m_pSpinBoxBaselineToMSeconds->setMinimum(ui->m_pSpinBoxPreStimMSeconds->value()*-1);
    ui->m_pSpinBoxBaselineFromMSeconds->setMinimum(ui->m_pSpinBoxPreStimMSeconds->value()*-1);

    m_iPreStimSeconds = mSeconds;

    emit changePreStim(mSeconds);

    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangePostStim()
{
    qint32 mSeconds = ui->m_pSpinBoxPostStimMSeconds->value();
    ui->m_pSpinBoxBaselineToMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());
    ui->m_pSpinBoxBaselineFromMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());

    m_iPostStimSeconds = mSeconds;

    emit changePostStim(mSeconds);

    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineFrom()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineFromMSeconds->value();
    ui->m_pSpinBoxBaselineToMSeconds->setMinimum(mSeconds);

    m_iBaselineFromSeconds = mSeconds;

    emit changeBaselineFrom(mSeconds);

    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineTo()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineToMSeconds->value();
    ui->m_pSpinBoxBaselineFromMSeconds->setMaximum(mSeconds);

    m_iBaselineToSeconds = mSeconds;

    emit changeBaselineTo(mSeconds);

    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeNumAverages()
{
    m_iNumAverages = ui->m_pSpinBoxNumAverages->value();

    emit changeNumAverages(ui->m_pSpinBoxNumAverages->value());

    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeStimChannel()
{
    m_sCurrentStimChan = ui->m_pComboBoxChSelection->currentText();

    emit changeStimChannel(m_sCurrentStimChan);

    saveSettings(m_sSettingsPath);
}
