//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     triggerdetectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the TriggerDetectionView trigger-detection panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggerdetectionview.h"

#include "ui_triggerdetectionview.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColorDialog>
#include <QPalette>
#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerDetectionView::TriggerDetectionView(const QString& sSettingsPath,
                                           QWidget* parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(std::make_unique<Ui::TriggerDetectionViewWidget>())
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Trigger Detection Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    updateProcessingMode(RealTime);

    loadSettings();
}

//=============================================================================================================

TriggerDetectionView::~TriggerDetectionView()
{
    saveSettings();
}

//=============================================================================================================

void TriggerDetectionView::init(const std::shared_ptr<FiffInfo>& pFiffInfo)
{
    if (pFiffInfo) {
        m_pFiffInfo = pFiffInfo;
        //Trigger detection
        connect(m_pUi->m_checkBox_activateTriggerDetection, &QCheckBox::checkStateChanged,
                this, &TriggerDetectionView::onTriggerInfoChanged, Qt::UniqueConnection);

        m_pUi->m_comboBox_triggerChannels->clear();

        for (int i = 0; i < std::ssize(m_pFiffInfo->chs); i++) {
            if (m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                m_pUi->m_comboBox_triggerChannels->addItem(QString::fromStdString(m_pFiffInfo->chs[i].ch_name));
            }
        }

        connect(m_pUi->m_comboBox_triggerChannels, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged, Qt::UniqueConnection);

        connect(m_pUi->m_comboBox_triggerColorType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onRealTimeTriggerColorTypeChanged, Qt::UniqueConnection);

        connect(m_pUi->m_pushButton_triggerColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onRealTimeTriggerColorChanged, Qt::UniqueConnection);

        m_pUi->m_pushButton_triggerColor->setAutoFillBackground(true);
        m_pUi->m_pushButton_triggerColor->setFlat(true);
        QPalette palette = m_pUi->m_pushButton_triggerColor->palette();
        palette.setColor(QPalette::Button, QColor(177, 0, 0));
        m_pUi->m_pushButton_triggerColor->setPalette(palette);
        m_pUi->m_pushButton_triggerColor->update();

        connect(m_pUi->m_doubleSpinBox_detectionThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged, Qt::UniqueConnection);

        connect(m_pUi->m_spinBox_detectionThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged, Qt::UniqueConnection);

        connect(m_pUi->m_pushButton_resetNumberTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onResetTriggerNumbers, Qt::UniqueConnection);

        connect(m_pUi->m_pushButton_DetectTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onDetectTriggers, Qt::UniqueConnection);

        loadSettings();
    }
}

//=============================================================================================================

void TriggerDetectionView::setNumberDetectedTriggersAndTypes(int numberDetections, const std::map<int, std::vector<std::pair<int, double>>>& mapDetectedTriggers)
{
    //if(m_bTriggerDetection) {
    m_pUi->m_label_numberDetectedTriggers->setText(QString("%1").arg(numberDetections));
    //}

    //Set trigger types
    for (const auto& [channel, triggers] : mapDetectedTriggers) {
        for (const auto& [sample, value] : triggers) {
            if (m_pUi->m_comboBox_triggerColorType->findText(QString::number(value)) == -1) {
                m_pUi->m_comboBox_triggerColorType->addItem(QString::number(value));
            }
        }
    }
}

//=============================================================================================================

void TriggerDetectionView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), m_pUi->m_checkBox_activateTriggerDetection->isChecked());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), m_pUi->m_comboBox_triggerChannels->currentIndex());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), m_pUi->m_doubleSpinBox_detectionThresholdFirst->value());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), m_pUi->m_spinBox_detectionThresholdSecond->value());

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    std::map<double, QColor>::const_iterator i = m_qMapTriggerColor.cbegin();
    while (i != m_qMapTriggerColor.cend()) {
        settings.setValue(QString::number(i->first), i->second);
        ++i;
    }
    settings.endGroup();
}

//=============================================================================================================

void TriggerDetectionView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    m_pUi->m_checkBox_activateTriggerDetection->setChecked(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), false).toBool());
    m_pUi->m_comboBox_triggerChannels->setCurrentIndex(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), 0).toInt());
    m_pUi->m_doubleSpinBox_detectionThresholdFirst->setValue(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), 0.1).toDouble());
    m_pUi->m_spinBox_detectionThresholdSecond->setValue(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), -1).toInt());

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    for (const QString& key : settings.childKeys()) {
        double dKey = key.toDouble();
        m_qMapTriggerColor[dKey] = settings.value(key).value<QColor>();
    }
    settings.endGroup();

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TriggerDetectionView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            m_pUi->label->hide();
            m_pUi->label_9->hide();
            m_pUi->m_pushButton_resetNumberTriggers->hide();
            m_pUi->m_pushButton_triggerColor->hide();
            m_pUi->m_checkBox_activateTriggerDetection->hide();
            m_pUi->m_comboBox_triggerColorType->hide();
            m_pUi->m_label_numberDetectedTriggers->hide();
            m_pUi->m_pushButton_DetectTriggers->show();
            break;
        default: // default is realtime mode
            m_pUi->label->show();
            m_pUi->label_9->show();
            m_pUi->m_pushButton_resetNumberTriggers->show();
            m_pUi->m_pushButton_triggerColor->show();
            m_pUi->m_checkBox_activateTriggerDetection->show();
            m_pUi->m_comboBox_triggerColorType->show();
            m_pUi->m_label_numberDetectedTriggers->show();
            m_pUi->m_pushButton_DetectTriggers->hide();
            break;
    }
}

//=============================================================================================================

void TriggerDetectionView::onTriggerInfoChanged()
{
    emit triggerInfoChanged(m_qMapTriggerColor,
                            m_pUi->m_checkBox_activateTriggerDetection->isChecked(),
                            m_pUi->m_comboBox_triggerChannels->currentText().toStdString(),
                            m_pUi->m_doubleSpinBox_detectionThresholdFirst->value() * pow(10, m_pUi->m_spinBox_detectionThresholdSecond->value()));

    saveSettings();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    QColor color = QColorDialog::getColor(m_qMapTriggerColor[m_pUi->m_comboBox_triggerColorType->currentText().toDouble()], this, "Set trigger color");
    // Cancelled: getColor() returns an invalid colour
    if (!color.isValid()) {
        return;
    }

    //Change color of pushbutton
    QPalette palette = m_pUi->m_pushButton_triggerColor->palette();
    palette.setColor(QPalette::Button, color);
    m_pUi->m_pushButton_triggerColor->setPalette(palette);
    m_pUi->m_pushButton_triggerColor->update();

    m_qMapTriggerColor[m_pUi->m_comboBox_triggerColorType->currentText().toDouble()] = color;

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorTypeChanged(const QString& value)
{
    //Change color of pushbutton
    QPalette palette = m_pUi->m_pushButton_triggerColor->palette();
    palette.setColor(QPalette::Button, m_qMapTriggerColor[value.toDouble()]);
    m_pUi->m_pushButton_triggerColor->setPalette(palette);
    m_pUi->m_pushButton_triggerColor->update();
}

//=============================================================================================================

void TriggerDetectionView::onResetTriggerNumbers()
{
    m_pUi->m_label_numberDetectedTriggers->setText(QString("0"));
    m_pUi->m_comboBox_triggerColorType->clear();

    emit resetTriggerCounter();

    //emit updateConnectedView();
}

//=============================================================================================================

void TriggerDetectionView::onDetectTriggers()
{
    if (m_pUi->m_comboBox_triggerChannels->currentText() == "") {
        return;
    }
    emit detectTriggers(m_pUi->m_comboBox_triggerChannels->currentText().toStdString(),
                        m_pUi->m_doubleSpinBox_detectionThresholdFirst->value() * pow(10, m_pUi->m_spinBox_detectionThresholdSecond->value()));
}

//=============================================================================================================

std::string TriggerDetectionView::getSelectedStimChannel()
{
    return m_pUi->m_comboBox_triggerChannels->currentText().toStdString();
}

//=============================================================================================================

void TriggerDetectionView::clearView()
{
}
