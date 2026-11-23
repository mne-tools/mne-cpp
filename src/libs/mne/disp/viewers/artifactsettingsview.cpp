//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     artifactsettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of the ArtifactSettingsView per-modality threshold panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifactsettingsview.h"

#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QLabel>
#include <QGridLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ArtifactSettingsView::ArtifactSettingsView(const QString& sSettingsPath,
                                           const std::vector<FiffChInfo>& fiffChInfoList,
                                           QWidget* parent)
: AbstractView(parent)
, m_fiffChInfoList(fiffChInfoList)
{
    m_sSettingsPath = sSettingsPath;
    qRegisterMetaType<std::map<std::string, double>>("std::map<std::string, double>");

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

void ArtifactSettingsView::setChInfo(const std::vector<mne::FiffChInfo>& fiffChInfoList)
{
    m_fiffChInfoList = fiffChInfoList;

    redrawGUI();
    onChangeArtifactThreshold();
}

//=============================================================================================================

std::map<std::string, double> ArtifactSettingsView::getThresholdMap()
{
    return m_mapThresholds;
}

//=============================================================================================================

void ArtifactSettingsView::setThresholdMap(const std::map<std::string, double>& mapThresholds)
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
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/ArtifactSettingsView/doArtifactThresholdReduction"), m_bDoArtifactThresholdReduction);

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsFirst"));
    std::map<std::string, double>::const_iterator itrFirst = m_mapThresholdsFirst.cbegin();
    while (itrFirst != m_mapThresholdsFirst.cend()) {
        settings.setValue(QString::fromStdString(itrFirst->first), itrFirst->second);
        ++itrFirst;
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsSecond"));
    std::map<std::string, int>::const_iterator itrSecond = m_mapThresholdsSecond.cbegin();
    while (itrSecond != m_mapThresholdsSecond.cend()) {
        settings.setValue(QString::fromStdString(itrSecond->first), itrSecond->second);
        ++itrSecond;
    }
    settings.endGroup();
}

//=============================================================================================================

void ArtifactSettingsView::loadSettings()
{
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

    if (m_sSettingsPath.isEmpty()) {
        m_mapThresholds["Active"] = m_bDoArtifactThresholdReduction ? 1.0 : 0.0;
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

    m_bDoArtifactThresholdReduction = settings.value(m_sSettingsPath + QString("/ArtifactSettingsView/doArtifactThresholdReduction"), false).toBool();
    m_mapThresholds["Active"] = m_bDoArtifactThresholdReduction ? 1.0 : 0.0;

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsFirst"));
    for (const QString& key : settings.childKeys()) {
        m_mapThresholdsFirst.insert_or_assign(key.toStdString(), settings.value(key, 1.0).toDouble());
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/ArtifactSettingsView/artifactThresholdsSecond"));
    for (const QString& key : settings.childKeys()) {
        m_mapThresholdsSecond.insert_or_assign(key.toStdString(), settings.value(key, -1).toInt());
    }
    settings.endGroup();
}

//=============================================================================================================

void ArtifactSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ArtifactSettingsView::redrawGUI()
{
    // The view accepts the new layout only once the old one is gone, and deleting a layout leaves its widgets on the view
    if (std::unique_ptr<QLayout> pLayout{this->layout()}) {
        while (std::unique_ptr<QLayoutItem> pItem{pLayout->takeAt(0)}) {
            std::unique_ptr<QWidget> pWidget{pItem->widget()};
        }
    }
    m_mapChThresholdsDoubleSpinBoxes.clear();
    m_mapChThresholdsSpinBoxes.clear();

    QGridLayout* pGroupBoxArtifactRejection = new QGridLayout();
    this->setLayout(pGroupBoxArtifactRejection);

    //Artifact rejection
    if (!m_fiffChInfoList.empty()) {
        std::vector<std::string> channelTypes;
        const auto addType = [&channelTypes](const char* type) {
            if (!std::ranges::contains(channelTypes, type)) {
                channelTypes.push_back(type);
            }
        };
        int kind, unit;

        for (int i = 0; i < std::ssize(m_fiffChInfoList); ++i) {
            kind = m_fiffChInfoList.at(i).kind;
            unit = m_fiffChInfoList.at(i).unit;

            if (kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T_M) {
                addType("grad");
            }
            if (kind == FIFFV_MEG_CH && unit == FIFF_UNIT_T) {
                addType("mag");
            }
            if (kind == FIFFV_EEG_CH) {
                addType("eeg");
            }
            if (kind == FIFFV_EOG_CH) {
                addType("eog");
            }
            if (kind == FIFFV_EMG_CH) {
                addType("emg");
            }
            if (kind == FIFFV_ECG_CH) {
                addType("ecg");
            }
        }

        if (!channelTypes.empty()) {
            m_pArtifactRejectionCheckBox = new QCheckBox("Activate artifact rejection");
            pGroupBoxArtifactRejection->addWidget(m_pArtifactRejectionCheckBox, 0, 0, 1, 2);
            m_pArtifactRejectionCheckBox->setChecked(m_bDoArtifactThresholdReduction);
            connect(m_pArtifactRejectionCheckBox.data(), &QCheckBox::clicked,
                    this, &ArtifactSettingsView::onChangeArtifactThreshold);

            for (int i = 0; i < std::ssize(channelTypes); ++i) {
                QLabel* pLabel = new QLabel(QString::fromStdString(channelTypes.at(i)));
                pGroupBoxArtifactRejection->addWidget(pLabel, i + 1, 0);

                QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox();
                pDoubleSpinBox->setPrefix("+/-");
                pDoubleSpinBox->setMinimum(0.0);
                pDoubleSpinBox->setMaximum(100000.0);
                pDoubleSpinBox->setValue(m_mapThresholdsFirst[channelTypes.at(i)]);
                pGroupBoxArtifactRejection->addWidget(pDoubleSpinBox, i + 1, 1);
                connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                        this, &ArtifactSettingsView::onChangeArtifactThreshold);
                m_mapChThresholdsDoubleSpinBoxes[channelTypes.at(i)] = pDoubleSpinBox;

                QSpinBox* pSpinBox = new QSpinBox();
                pSpinBox->setPrefix("e");
                pSpinBox->setMaximum(0);
                pSpinBox->setMinimum(-10000);
                pSpinBox->setValue(m_mapThresholdsSecond[channelTypes.at(i)]);
                pGroupBoxArtifactRejection->addWidget(pSpinBox, i + 1, 2);
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

    if (m_pArtifactRejectionCheckBox) {
        if (m_pArtifactRejectionCheckBox->isChecked()) {
            m_mapThresholds["Active"] = 1.0;
            m_bDoArtifactThresholdReduction = true;
        } else {
            m_mapThresholds["Active"] = 0.0;
            m_bDoArtifactThresholdReduction = false;
        }
    }

    for (const auto& [key, pDoubleSpinBox] : m_mapChThresholdsDoubleSpinBoxes) {
        if (pDoubleSpinBox) {
            const int exponent = m_mapChThresholdsSpinBoxes[key]->value();
            m_mapThresholdsFirst[key] = pDoubleSpinBox->value();
            m_mapThresholdsSecond[key] = exponent;

            if (mne::to_lower(key).contains("grad")) {
                m_mapThresholds["grad"] = pDoubleSpinBox->value() * pow(10, exponent);
            }
            if (mne::to_lower(key).contains("mag")) {
                m_mapThresholds["mag"] = pDoubleSpinBox->value() * pow(10, exponent);
            }
            if (mne::to_lower(key).contains("eeg")) {
                m_mapThresholds["eeg"] = pDoubleSpinBox->value() * pow(10, exponent);
            }
            if (mne::to_lower(key).contains("ecg")) {
                m_mapThresholds["ecg"] = pDoubleSpinBox->value() * pow(10, exponent);
            }
            if (mne::to_lower(key).contains("eog")) {
                m_mapThresholds["eog"] = pDoubleSpinBox->value() * pow(10, exponent);
            }
            if (mne::to_lower(key).contains("emg")) {
                m_mapThresholds["emg"] = pDoubleSpinBox->value() * pow(10, exponent);
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
