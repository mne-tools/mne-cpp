//=============================================================================================================
/**
 * @file     triggerdetectionview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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
: QWidget(parent, f)
, ui(new Ui::TriggerDetectionViewWidget)
, m_sSettingsPath(sSettingsPath)
{
    ui->setupUi(this);

    this->setWindowTitle("Trigger Detection Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

TriggerDetectionView::~TriggerDetectionView()
{
    saveSettings(m_sSettingsPath);

    delete ui;
}

//=============================================================================================================

void TriggerDetectionView::init(const FiffInfo::SPtr pFiffInfo)
{
    if(pFiffInfo) {
        m_pFiffInfo = pFiffInfo;
        //Trigger detection
        connect(ui->m_checkBox_activateTriggerDetection, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
            if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                ui->m_comboBox_triggerChannels->addItem(m_pFiffInfo->chs[i].ch_name);
            }
        }

        connect(ui->m_comboBox_triggerChannels, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(ui->m_comboBox_triggerColorType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
                this, &TriggerDetectionView::onRealTimeTriggerColorTypeChanged);

        connect(ui->m_pushButton_triggerColor, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onRealTimeTriggerColorChanged);

        ui->m_pushButton_triggerColor->setAutoFillBackground(true);
        ui->m_pushButton_triggerColor->setFlat(true);
        QPalette* palette1 = new QPalette();
        palette1->setColor(QPalette::Button,QColor(177,0,0));
        ui->m_pushButton_triggerColor->setPalette(*palette1);
        ui->m_pushButton_triggerColor->update();

        connect(ui->m_doubleSpinBox_detectionThresholdFirst, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(ui->m_spinBox_detectionThresholdSecond, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &TriggerDetectionView::onTriggerInfoChanged);

        connect(ui->m_pushButton_resetNumberTriggers, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &TriggerDetectionView::onResetTriggerNumbers);

        loadSettings(m_sSettingsPath);
    }
}

//=============================================================================================================

void TriggerDetectionView::setNumberDetectedTriggersAndTypes(int numberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers)
{
    //if(m_bTriggerDetection) {
        ui->m_label_numberDetectedTriggers->setText(QString("%1").arg(numberDetections));
    //}

    //Set trigger types
    QMapIterator<int,QList<QPair<int,double> > > i(mapDetectedTriggers);
    while (i.hasNext()) {
        i.next();

        for(int j = 0; j < i.value().size(); ++j) {
            if(ui->m_comboBox_triggerColorType->findText(QString::number(i.value().at(j).second)) == -1) {
                ui->m_comboBox_triggerColorType->addItem(QString::number(i.value().at(j).second));
            }
        }
    }
}

//=============================================================================================================

void TriggerDetectionView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/triggerDetectionActivated"), ui->m_checkBox_activateTriggerDetection->isChecked());
    settings.setValue(settingsPath + QString("/triggerDetectionChannelIndex"), ui->m_comboBox_triggerChannels->currentIndex());
    settings.setValue(settingsPath + QString("/triggerDetectionFirstThresholdValue"), ui->m_doubleSpinBox_detectionThresholdFirst->value());
    settings.setValue(settingsPath + QString("/triggerDetectionSecondThresholdValue"), ui->m_spinBox_detectionThresholdSecond->value());

    settings.beginGroup(settingsPath + QString("/triggerDetectionColors"));
    QMap<double, QColor>::const_iterator i = m_qMapTriggerColor.constBegin();
    while (i != m_qMapTriggerColor.constEnd()) {
         settings.setValue(QString::number(i.key()), i.value());
         ++i;
    }
    settings.endGroup();
}

//=============================================================================================================

void TriggerDetectionView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    ui->m_checkBox_activateTriggerDetection->setChecked(settings.value(settingsPath + QString("/triggerDetectionActivated"), false).toBool());
    ui->m_comboBox_triggerChannels->setCurrentIndex(settings.value(settingsPath + QString("/triggerDetectionChannelIndex"), 0).toInt());
    ui->m_doubleSpinBox_detectionThresholdFirst->setValue(settings.value(settingsPath + QString("/triggerDetectionFirstThresholdValue"), 0.1).toDouble());
    ui->m_spinBox_detectionThresholdSecond->setValue(settings.value(settingsPath + QString("/triggerDetectionSecondThresholdValue"), -1).toInt());

    settings.beginGroup(settingsPath + QString("/triggerDetectionColors"));
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
        double dKey = key.toDouble();
        m_qMapTriggerColor[dKey] = settings.value(key).value<QColor>();
    }
    settings.endGroup();

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::onTriggerInfoChanged()
{
    emit triggerInfoChanged(m_qMapTriggerColor,
                            ui->m_checkBox_activateTriggerDetection->isChecked(),
                            ui->m_comboBox_triggerChannels->currentText(),
                            ui->m_doubleSpinBox_detectionThresholdFirst->value()*pow(10, ui->m_spinBox_detectionThresholdSecond->value()));

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    QColor color = QColorDialog::getColor(m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()], this, "Set trigger color");

    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,color);
    ui->m_pushButton_triggerColor->setPalette(*palette1);
    ui->m_pushButton_triggerColor->update();

    m_qMapTriggerColor[ui->m_comboBox_triggerColorType->currentText().toDouble()] = color;

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorTypeChanged(const QString &value)
{
    //Change color of pushbutton
    QPalette* palette1 = new QPalette();
    palette1->setColor(QPalette::Button,m_qMapTriggerColor[value.toDouble()]);
    ui->m_pushButton_triggerColor->setPalette(*palette1);
    ui->m_pushButton_triggerColor->update();
}

//=============================================================================================================

void TriggerDetectionView::onResetTriggerNumbers()
{
    ui->m_label_numberDetectedTriggers->setText(QString("0"));
    ui->m_comboBox_triggerColorType->clear();

    emit resetTriggerCounter();

    //emit updateConnectedView();
}
