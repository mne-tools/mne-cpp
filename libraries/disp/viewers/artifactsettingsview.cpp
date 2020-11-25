//=============================================================================================================
/**
 * @file     artifactsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ArtifactSettingsView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifactsettingsview.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QLabel>
#include <QGridLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ArtifactSettingsView::ArtifactSettingsView(const QString& sSettingsPath,
                                           const QList<FiffChInfo>& fiffChInfoList,
                                           QWidget *parent)
: AbstractView(parent)
, m_fiffChInfoList(fiffChInfoList)
{
    m_sSettingsPath = sSettingsPath;
    qRegisterMetaType<QMap<QString,double> >("QMap<QString,double>");

    this->setWindowTitle("Artifact Rejection Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

ArtifactSettingsView::~ArtifactSettingsView()
{

    saveSettings();
}

//=============================================================================================================

void ArtifactSettingsView::setChInfo(const QList<FIFFLIB::FiffChInfo>& fiffChInfoList)
{
    m_fiffChInfoList = fiffChInfoList;

    redrawGUI();
    onChangeArtifactThreshold();
}

//=============================================================================================================

QMap<QString,double> ArtifactSettingsView::getThresholdMap()
{
    return m_mapThresholds;
}

//=============================================================================================================

void ArtifactSettingsView::setThresholdMap(const QMap<QString,double>& mapThresholds)
{
    m_mapThresholds = mapThresholds;

    redrawGUI();
    onChangeArtifactThreshold();
}

//=============================================================================================================

bool ArtifactSettingsView::getDoArtifactThresholdRejection()
{
    if (m_pArtifactRejectionCheckBox) {
        return m_pArtifactRejectionCheckBox->isChecked();
    }

    return false;
}

//=============================================================================================================

void ArtifactSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/ArtifactSettingsView/doArtifactThresholdReduction"), m_bDoArtifactThresholdReduction);

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsFirst"));
    QMap<QString, double>::const_iterator itrFirst = m_mapThresholdsFirst.constBegin();
    while (itrFirst != m_mapThresholdsFirst.constEnd()) {
         settings.setValue(itrFirst.key(), itrFirst.value());
         ++itrFirst;
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsSecond"));
    QMap<QString, int>::const_iterator itrSecond = m_mapThresholdsSecond.constBegin();
    while (itrSecond != m_mapThresholdsSecond.constEnd()) {
         settings.setValue(itrSecond.key(), itrSecond.value());
         ++itrSecond;
    }
    settings.endGroup();
}

//=============================================================================================================

void ArtifactSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

    m_bDoArtifactThresholdReduction = settings.value(m_sSettingsPath + QString("/ArtifactSettingsView/doArtifactThresholdReduction"), false).toBool();

    if(m_bDoArtifactThresholdReduction) {
        m_mapThresholds["Active"] = 1.0;
    } else {
        m_mapThresholds["Active"] = 0.0;
    }

    m_mapThresholdsFirst["grad"] = 1.0;
    m_mapThresholdsFirst["mag"] = 1.0;
    m_mapThresholdsFirst["eeg"] = 1.0;
    m_mapThresholdsFirst["ecg"] = 1.0;
    m_mapThresholdsFirst["emg"] = 1.0;
    m_mapThresholdsFirst["eog"] = 1.0;

    m_mapThresholdsSecond["grad"] = -1;
    m_mapThresholdsSecond["mag"] = -1;
    m_mapThresholdsSecond["eeg"] = -1;
    m_mapThresholdsSecond["ecg"] = -1;
    m_mapThresholdsSecond["emg"] = -1;
    m_mapThresholdsSecond["eog"] = -1;

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsFirst"));
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
         m_mapThresholdsFirst.insert(key, settings.value(key, 1.0).toDouble());
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsSecond"));
    keys = settings.childKeys();
    foreach (QString key, keys) {
         m_mapThresholdsSecond.insert(key, settings.value(key, -1).toInt());
    }
    settings.endGroup();
}

//=============================================================================================================

void ArtifactSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::redrawGUI()
{
    if(QLayout* layout = this->layout()) {
        delete layout;
    }

    QGridLayout* pGroupBoxArtifactRejection = new QGridLayout();
    this->setLayout(pGroupBoxArtifactRejection);

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
            if(kind == FIFFV_EEG_CH && !channelTypes.contains("eeg", Qt::CaseInsensitive)) {
                channelTypes << "eeg";
            }
            if(kind == FIFFV_EOG_CH && !channelTypes.contains("eog", Qt::CaseInsensitive)) {
                channelTypes << "eog";
            }
            if(kind == FIFFV_EMG_CH && !channelTypes.contains("emg", Qt::CaseInsensitive)) {
                channelTypes << "emg";
            }
            if(kind == FIFFV_ECG_CH && !channelTypes.contains("ecg", Qt::CaseInsensitive)) {
                channelTypes << "ecg";
            }
        }

        if(!channelTypes.isEmpty()) {
            m_pArtifactRejectionCheckBox = new QCheckBox("Activate artifact rejection");
            pGroupBoxArtifactRejection->addWidget(m_pArtifactRejectionCheckBox,0,0,1,2);
            m_pArtifactRejectionCheckBox->setChecked(m_bDoArtifactThresholdReduction);
            connect(m_pArtifactRejectionCheckBox.data(), &QCheckBox::clicked,
                    this, &ArtifactSettingsView::onChangeArtifactThreshold);

            for(int i = 0; i < channelTypes.size(); ++i) {
                QLabel* pLabel = new QLabel(channelTypes.at(i));
                pGroupBoxArtifactRejection->addWidget(pLabel,i+1,0);

                QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox();
                pDoubleSpinBox->setPrefix("+/-");
                pDoubleSpinBox->setMinimum(0.0);
                pDoubleSpinBox->setMaximum(100000.0);
                pDoubleSpinBox->setValue(m_mapThresholdsFirst[channelTypes.at(i)]);
                pGroupBoxArtifactRejection->addWidget(pDoubleSpinBox,i+1,1);
                connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                            this, &ArtifactSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsDoubleSpinBoxes[channelTypes.at(i)] = pDoubleSpinBox;

                QSpinBox* pSpinBox = new QSpinBox();
                pSpinBox->setPrefix("e");
                pSpinBox->setMaximum(0);
                pSpinBox->setMinimum(-10000);
                pSpinBox->setValue(m_mapThresholdsSecond[channelTypes.at(i)]);
                pGroupBoxArtifactRejection->addWidget(pSpinBox,i+1,2);
                connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                            this, &ArtifactSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsSpinBoxes[channelTypes.at(i)] = pSpinBox;
            }
        }
    }
}

//=============================================================================================================

void ArtifactSettingsView::onChangeArtifactThreshold()
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

            if(i.key().contains("grad", Qt::CaseInsensitive)) {
                m_mapThresholds["grad"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("mag", Qt::CaseInsensitive)) {
                m_mapThresholds["mag"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("eeg", Qt::CaseInsensitive)) {
                m_mapThresholds["eeg"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("ecg", Qt::CaseInsensitive)) {
                m_mapThresholds["ecg"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("eog", Qt::CaseInsensitive)) {
                m_mapThresholds["eog"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
            if(i.key().contains("emg", Qt::CaseInsensitive)) {
                m_mapThresholds["emg"] = i.value()->value() * pow(10, m_mapChThresholdsSpinBoxes[i.key()]->value());
            }
        }
    }

    emit changeArtifactThreshold(m_mapThresholds);

    saveSettings();
}

//=============================================================================================================

void ArtifactSettingsView::clearView()
{

}
