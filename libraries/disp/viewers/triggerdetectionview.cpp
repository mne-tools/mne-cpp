//=============================================================================================================
/**
 * @file     triggerdetectionview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
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
 * @brief    Definition of the TriggerDetectionView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggerdetectionview.h"

#include "ui_triggerdetectionview.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColorDialog>
#include <QPalette>
#include <QSettings>

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

TriggerDetectionView::TriggerDetectionView(const QString& sSettingsPath,
                                           QWidget *parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::TriggerDetectionViewWidget)
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

    delete m_pUi;
}

//=============================================================================================================

void TriggerDetectionView::init(const FiffInfo::SPtr pFiffInfo)
{
    if(pFiffInfo) {
        m_pFiffInfo = pFiffInfo;
        //Trigger detection
        connect(m_pUi->m_checkBox_activateTriggerDetection, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        m_pUi->m_comboBox_triggerChannels->clear();

        for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
            if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                m_pUi->m_comboBox_triggerChannels->addItem(m_pFiffInfo->chs[i].ch_name);
            }
        }

        connect(m_pUi->m_comboBox_triggerChannels, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(m_pUi->m_comboBox_triggerColorType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onRealTimeTriggerColorTypeChanged);

        connect(m_pUi->m_pushButton_triggerColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onRealTimeTriggerColorChanged);

        m_pUi->m_pushButton_triggerColor->setAutoFillBackground(true);
        m_pUi->m_pushButton_triggerColor->setFlat(true);
        QPalette* palette1 = new QPalette();
        palette1->setColor(QPalette::Button,QColor(177,0,0));
        m_pUi->m_pushButton_triggerColor->setPalette(*palette1);
        m_pUi->m_pushButton_triggerColor->update();

        connect(m_pUi->m_doubleSpinBox_detectionThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(m_pUi->m_spinBox_detectionThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(m_pUi->m_pushButton_resetNumberTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onResetTriggerNumbers);

        connect(m_pUi->m_pushButton_DetectTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onDetectTriggers, Qt::UniqueConnection);

        loadSettings();
    }
}

//=============================================================================================================

void TriggerDetectionView::setNumberDetectedTriggersAndTypes(int numberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers)
{
    //if(m_bTriggerDetection) {
        m_pUi->m_label_numberDetectedTriggers->setText(QString("%1").arg(numberDetections));
    //}

    //Set trigger types
    QMapIterator<int,QList<QPair<int,double> > > i(mapDetectedTriggers);
    while (i.hasNext()) {
        i.next();

        for(int j = 0; j < i.value().size(); ++j) {
            if(m_pUi->m_comboBox_triggerColorType->findText(QString::number(i.value().at(j).second)) == -1) {
                m_pUi->m_comboBox_triggerColorType->addItem(QString::number(i.value().at(j).second));
            }
        }
    }
}

//=============================================================================================================

void TriggerDetectionView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), m_pUi->m_checkBox_activateTriggerDetection->isChecked());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), m_pUi->m_comboBox_triggerChannels->currentIndex());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), m_pUi->m_doubleSpinBox_detectionThresholdFirst->value());
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), m_pUi->m_spinBox_detectionThresholdSecond->value());

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    QMap<double, QColor>::const_iterator i = m_qMapTriggerColor.constBegin();
    while (i != m_qMapTriggerColor.constEnd()) {
         settings.setValue(QString::number(i.key()), i.value());
         ++i;
    }
    settings.endGroup();
}

//=============================================================================================================

void TriggerDetectionView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    m_pUi->m_checkBox_activateTriggerDetection->setChecked(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), false).toBool());
    m_pUi->m_comboBox_triggerChannels->setCurrentIndex(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), 0).toInt());
    m_pUi->m_doubleSpinBox_detectionThresholdFirst->setValue(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), 0.1).toDouble());
    m_pUi->m_spinBox_detectionThresholdSecond->setValue(settings.value(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), -1).toInt());

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
        double dKey = key.toDouble();
        m_qMapTriggerColor[dKey] = settings.value(key).value<QColor>();
    }
    settings.endGroup();

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TriggerDetectionView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
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
                            m_pUi->m_comboBox_triggerChannels->currentText(),
                            m_pUi->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, m_pUi->m_spinBox_detectionThresholdSecond->value()));

    saveSettings();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    QColor color = QColorDialog::getColor(m_qMapTriggerColor[m_pUi->m_comboBox_triggerColorType->currentText().toDouble()], this, "Set trigger color");

    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,color);
    m_pUi->m_pushButton_triggerColor->setPalette(*palette1);
    m_pUi->m_pushButton_triggerColor->update();

    m_qMapTriggerColor[m_pUi->m_comboBox_triggerColorType->currentText().toDouble()] = color;

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorTypeChanged(const QString &value)
{
    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,m_qMapTriggerColor[value.toDouble()]);
    m_pUi->m_pushButton_triggerColor->setPalette(*palette1);
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
    if(m_pUi->m_comboBox_triggerChannels->currentText() == ""){
        return;
    }
    emit detectTriggers(m_pUi->m_comboBox_triggerChannels->currentText(),
                        m_pUi->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, m_pUi->m_spinBox_detectionThresholdSecond->value()));
}

//=============================================================================================================

QString TriggerDetectionView::getSelectedStimChannel()
{
    return m_pUi->m_comboBox_triggerChannels->currentText();
}

//=============================================================================================================

void TriggerDetectionView::clearView()
{

}
