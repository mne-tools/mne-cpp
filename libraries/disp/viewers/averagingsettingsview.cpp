//=============================================================================================================
/**
* @file     averagingsettingsview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
                                             const QList<FiffChInfo>& fiffChInfoList,
                                             const QMap<QString, int> &mapStimChsIndexNames,
                                             QWidget *parent)
: QWidget(parent)
, ui(new Ui::AverageSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_mapStimChsIndexNames(mapStimChsIndexNames)
, m_fiffChInfoList(fiffChInfoList)
{
    qRegisterMetaType<QMap<QString,double> >("QMap<QString,double>");

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
                this, &AveragingSettingsView::changeStimChannel);
    }
}


//*************************************************************************************************************

void AveragingSettingsView::setChInfo(const QList<FIFFLIB::FiffChInfo>& fiffChInfoList)
{
    m_fiffChInfoList = fiffChInfoList;

    //Artifact rejection
    if(!m_fiffChInfoList.isEmpty()) {
        QStringList channelTypes;
        int kind, unit;

        for(int i = 0; i < m_fiffChInfoList.size(); ++i) {
            kind = m_fiffChInfoList.at(i).kind;
            unit = m_fiffChInfoList.at(i).unit;

            if(kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T_M && !channelTypes.contains("GRAD Tm")) {
                channelTypes << "GRAD Tm";
            }
            if(kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T && !channelTypes.contains("MAG T")) {
                channelTypes << "MAG T";
            }
            if(kind == FIFFV_EEG_CH && !channelTypes.contains("EEG V")) {
                channelTypes << "EEG V";
            }
            if(kind == FIFFV_EOG_CH && !channelTypes.contains("EOG V")) {
                channelTypes << "EOG V";
            }
            if(kind == FIFFV_EMG_CH && !channelTypes.contains("EMG V")) {
                channelTypes << "EMG V";
            }
            if(kind == FIFFV_ECG_CH && !channelTypes.contains("ECG V")) {
                channelTypes << "ECG V";
            }
        }

        if(!channelTypes.isEmpty()) {
            ui->m_groupBox_artifactRejection->show();
            QGridLayout* pLayout = new QGridLayout();
            m_pArtifactRejectionCheckBox = new QCheckBox("Activate");
            pLayout->addWidget(m_pArtifactRejectionCheckBox,0,0,1,2);
            m_pArtifactRejectionCheckBox->setChecked(m_bDoArtifactThresholdReduction);
            connect(m_pArtifactRejectionCheckBox.data(), &QCheckBox::clicked,
                    this, &AveragingSettingsView::onChangeArtifactThreshold);

            for(int i = 0; i < channelTypes.size(); ++i) {
                QLabel* pLabel = new QLabel(channelTypes.at(i));
                pLayout->addWidget(pLabel,i+1,0);

                QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox();
                pDoubleSpinBox->setPrefix("+/-");
                pDoubleSpinBox->setMinimum(0.0);
                pDoubleSpinBox->setMaximum(100000.0);
                pDoubleSpinBox->setValue(m_mapThresholdsFirst[channelTypes.at(i)]);
                pLayout->addWidget(pDoubleSpinBox,i+1,1);
                connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                            this, &AveragingSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsDoubleSpinBoxes[channelTypes.at(i)] = pDoubleSpinBox;

                QSpinBox* pSpinBox = new QSpinBox();
                pSpinBox->setPrefix("e");
                pSpinBox->setMaximum(0);
                pSpinBox->setMinimum(-10000);
                pSpinBox->setValue(m_mapThresholdsSecond[channelTypes.at(i)]);
                pLayout->addWidget(pSpinBox,i+1,2);
                connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                            this, &AveragingSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsSpinBoxes[channelTypes.at(i)] = pSpinBox;
            }

            if(QLayout* layout = ui->m_groupBox_artifactRejection->layout()) {
                delete layout;
            }
            ui->m_groupBox_artifactRejection->setLayout(pLayout);
        } else {
            ui->m_groupBox_artifactRejection->hide();
        }
    }
}


//*************************************************************************************************************

QString AveragingSettingsView::getCurrentStimCh()
{
    return m_sCurrentStimChan;
}


//*************************************************************************************************************

QMap<QString,double> AveragingSettingsView::getThresholdMap()
{
    return m_mapThresholds;
}


//*************************************************************************************************************

void AveragingSettingsView::setThresholdMap(const QMap<QString,double>& mapThresholds)
{
    m_mapThresholds = mapThresholds;
}


//*************************************************************************************************************

bool AveragingSettingsView::getDoArtifactThresholdRejection()
{
    if (m_pArtifactRejectionCheckBox) {
        return m_pArtifactRejectionCheckBox->isChecked();
    }

    return false;
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

    //Artifact rejection
    if(!m_fiffChInfoList.isEmpty()) {
        QStringList channelTypes;
        int kind, unit;

        for(int i = 0; i < m_fiffChInfoList.size(); ++i) {
            kind = m_fiffChInfoList.at(i).kind;
            unit = m_fiffChInfoList.at(i).unit;

            if(kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T_M && !channelTypes.contains("grad")) {
                channelTypes << "grad";
            }
            if(kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T && !channelTypes.contains("mag")) {
                channelTypes << "mag";
            }
            if(kind == FIFFV_EEG_CH && !channelTypes.contains("eeg")) {
                channelTypes << "eeg";
            }
            if(kind == FIFFV_EOG_CH && !channelTypes.contains("eog")) {
                channelTypes << "eog";
            }
            if(kind == FIFFV_EMG_CH && !channelTypes.contains("emg")) {
                channelTypes << "emg";
            }
            if(kind == FIFFV_ECG_CH && !channelTypes.contains("ecg")) {
                channelTypes << "ecg";
            }
        }

        if(!channelTypes.isEmpty()) {
            ui->m_groupBox_artifactRejection->show();
            QGridLayout* pLayout = new QGridLayout();
            m_pArtifactRejectionCheckBox = new QCheckBox("Activate artifact rejection");
            pLayout->addWidget(m_pArtifactRejectionCheckBox,0,0,1,2);
            m_pArtifactRejectionCheckBox->setChecked(m_bDoArtifactThresholdReduction);
            connect(m_pArtifactRejectionCheckBox.data(), &QCheckBox::clicked,
                    this, &AveragingSettingsView::onChangeArtifactThreshold);

            for(int i = 0; i < channelTypes.size(); ++i) {
                QLabel* pLabel = new QLabel(channelTypes.at(i));
                pLayout->addWidget(pLabel,i+1,0);

                QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox();
                pDoubleSpinBox->setPrefix("+/-");
                pDoubleSpinBox->setMinimum(0.0);
                pDoubleSpinBox->setMaximum(100000.0);
                pDoubleSpinBox->setValue(m_mapThresholdsFirst[channelTypes.at(i)]);
                pLayout->addWidget(pDoubleSpinBox,i+1,1);
                connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                            this, &AveragingSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsDoubleSpinBoxes[channelTypes.at(i)] = pDoubleSpinBox;

                QSpinBox* pSpinBox = new QSpinBox();
                pSpinBox->setPrefix("e");
                pSpinBox->setMaximum(0);
                pSpinBox->setMinimum(-10000);
                pSpinBox->setValue(m_mapThresholdsSecond[channelTypes.at(i)]);
                pLayout->addWidget(pSpinBox,i+1,2);
                connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                            this, &AveragingSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsSpinBoxes[channelTypes.at(i)] = pSpinBox;
            }

            if(QLayout* layout = ui->m_groupBox_artifactRejection->layout()) {
                delete layout;
            }
            ui->m_groupBox_artifactRejection->setLayout(pLayout);
        } else {
            ui->m_groupBox_artifactRejection->hide();
        }
    }

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
    settings.setValue(settingsPath + QString("/doArtifactThresholdReduction"), m_bDoArtifactThresholdReduction);

    settings.beginGroup(settingsPath + QString("/artifactThresholdsFirst"));
    QMap<QString, double>::const_iterator itrFirst = m_mapThresholdsFirst.constBegin();
    while (itrFirst != m_mapThresholdsFirst.constEnd()) {
         settings.setValue(itrFirst.key(), itrFirst.value());
         ++itrFirst;
    }
    settings.endGroup();

    settings.beginGroup(settingsPath + QString("/artifactThresholdsSecond"));
    QMap<QString, int>::const_iterator itrSecond = m_mapThresholdsSecond.constBegin();
    while (itrSecond != m_mapThresholdsSecond.constEnd()) {
         settings.setValue(itrSecond.key(), itrSecond.value());
         ++itrSecond;
    }
    settings.endGroup();

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

    m_bDoArtifactThresholdReduction = settings.value(settingsPath + QString("/doArtifactThresholdReduction"), false).toBool();

    if(m_bDoArtifactThresholdReduction) {
        m_mapThresholds["Active"] = 1.0;
    } else {
        m_mapThresholds["Active"] = 0.0;
    }

    settings.beginGroup(settingsPath + QString("/artifactThresholdsFirst"));
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
         m_mapThresholdsFirst.insert(key, settings.value(key, 1.0).toDouble());
    }
    settings.endGroup();

    settings.beginGroup(settingsPath + QString("/artifactThresholdsSecond"));
    keys = settings.childKeys();
    foreach (QString key, keys) {
         m_mapThresholdsSecond.insert(key, settings.value(key, -1).toInt());
    }
    settings.endGroup();

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
}


//*************************************************************************************************************

void AveragingSettingsView::onChangePostStim()
{
    qint32 mSeconds = ui->m_pSpinBoxPostStimMSeconds->value();
    ui->m_pSpinBoxBaselineToMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());
    ui->m_pSpinBoxBaselineFromMSeconds->setMaximum(ui->m_pSpinBoxPostStimMSeconds->value());

    m_iPostStimSeconds = mSeconds;

    emit changePostStim(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineFrom()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineFromMSeconds->value();
    ui->m_pSpinBoxBaselineToMSeconds->setMinimum(mSeconds);

    m_iBaselineFromSeconds = mSeconds;

    emit changeBaselineFrom(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeBaselineTo()
{
    qint32 mSeconds = ui->m_pSpinBoxBaselineToMSeconds->value();
    ui->m_pSpinBoxBaselineFromMSeconds->setMaximum(mSeconds);

    m_iBaselineToSeconds = mSeconds;

    emit changeBaselineTo(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeArtifactThreshold()
{
    m_mapThresholds.clear();
    m_mapThresholdsFirst.clear();
    m_mapThresholdsSecond.clear();

    if(m_pArtifactRejectionCheckBox) {
        if(m_pArtifactRejectionCheckBox->isChecked()) {
            m_mapThresholds["Active"] = 1.0;
            m_bDoArtifactThresholdReduction = true;
        } else {
            m_mapThresholds["Active"] = 0.0;
            m_bDoArtifactThresholdReduction = false;
        }
    }

    QMapIterator<QString, QDoubleSpinBox*> i(m_mapChThresholdsDoubleSpinBoxes);

    while (i.hasNext()) {
        i.next();
        if(i.value()) {
            m_mapThresholdsFirst[i.key()] = i.value()->value();
            m_mapThresholdsSecond[i.key()] = m_mapChThresholdsSpinBoxes[i.key()]->value();

            if(i.key().contains("GRAD")) {
                m_mapThresholds["GRAD"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("MAG")) {
                m_mapThresholds["MAG"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("EEG")) {
                m_mapThresholds["EEG"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("ECG")) {
                m_mapThresholds["ECG"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("EOG")) {
                m_mapThresholds["EOG"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("EMG")) {
                m_mapThresholds["EMG"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
        }
    }

    emit changeArtifactThreshold(m_mapThresholds);
}


//*************************************************************************************************************

void AveragingSettingsView::onChangeNumAverages()
{
    m_iNumAverages = ui->m_pSpinBoxNumAverages->value();

    emit changeNumAverages(ui->m_pSpinBoxNumAverages->value());
}
