//=============================================================================================================
/**
 * @file     neurofeedbackbsettingsview.cpp
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
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
 * @brief    Definition of the NeurofeedbackBSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedbackbsettingsview.h"
#include "ui_neurofeedbackbsettingsview.h"

#include "neurofeedbackfsettingsview.h"
#include "ui_neurofeedbackfsettingsview.h"

#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include <QSettings>
#include <QFileDialog>

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeurofeedbackBSettingsView::NeurofeedbackBSettingsView(const QString& sSettingsPath, int ibMax, int ibMin, QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_pui(new Ui::NeurofeedbackBSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_ibMax(ibMax)
, m_ibMin(ibMin)
{
    m_pui->setupUi(this);

    this->setWindowTitle("Neurofeedback Settings");
    this->setMinimumWidth(550);
    this->setMaximumWidth(550);

    loadSettings(m_sSettingsPath);

    m_pui->lineEdit_DirBackground->setReadOnly(true);
    m_pui->lineEdit_DirObject->setReadOnly(true);

    onUpdateSpinBoxMax(m_ibMax);
    onUpdateSpinBoxMin(m_ibMin);

    connect(m_pui->pushButton_Background, &QPushButton::released, this, &NeurofeedbackBSettingsView::clickedLoadBackground);
    connect(m_pui->pushButton_Object, &QPushButton::released, this, &NeurofeedbackBSettingsView::clickedLoadObject);
    connect(m_pui->spinBox_Max,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackBSettingsView::onUpdateSpinBoxMax);
    connect(m_pui->spinBox_Min,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackBSettingsView::onUpdateSpinBoxMin);
    connect(m_pui->pushButton_StdMax, &QPushButton::released, this, &NeurofeedbackBSettingsView::clickedStdMax);
    connect(m_pui->pushButton_StdMin, &QPushButton::released, this, &NeurofeedbackBSettingsView::clickedStdMin);
    connect(m_pui->pushButton_ResetSettings, &QPushButton::released, this, &NeurofeedbackBSettingsView::clickedResetSettings);

    emitSignals();
    updateDisplay();

}
//=============================================================================================================

NeurofeedbackBSettingsView::~NeurofeedbackBSettingsView()
{
    saveSettings(m_sSettingsPath);

    delete m_pui;
}

//=============================================================================================================

void NeurofeedbackBSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Store Settings
    QSettings settings;

    settings.setValue(settingsPath + QString("/ibMax"), m_ibMax);
    settings.setValue(settingsPath + QString("/ibMin"), m_ibMin);
    settings.setValue(settingsPath + QString("/sDirBackground"), m_sDirBackground);
    settings.setValue(settingsPath + QString("/sDirObject"), m_sDirObject);

}

//=============================================================================================================

void NeurofeedbackBSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings;

    m_ibMax = settings.value(settingsPath + QString("/ibMax"), m_ibMax).toInt();
    m_ibMin = settings.value(settingsPath + QString("/ibMin"), m_ibMin).toInt();
    m_sDirBackground = settings.value(settingsPath + QString("/sDirBackground"), m_sDirBackground).toString();
    m_sDirObject = settings.value(settingsPath + QString("/sDirObject"), m_sDirObject).toString();

}

//=============================================================================================================

void NeurofeedbackBSettingsView::clickedResetSettings()
{
    m_ibMax = 25;
    m_ibMin = 0;
    m_sDirBackground = "";
    m_sDirObject = "";

    saveSettings(m_sSettingsPath);
    emitSignals();
    updateDisplay();
}

//=============================================================================================================

void NeurofeedbackBSettingsView::updateDisplay()
{
    m_pui->lineEdit_DirBackground->setText(m_sDirBackground);
    m_pui->lineEdit_DirObject->setText(m_sDirObject);
    m_pui->spinBox_Max->setValue(m_ibMax);
    m_pui->spinBox_Min->setValue(m_ibMin);
    updateStatusBackground();
    updateStatusObject();
}

//=============================================================================================================

void NeurofeedbackBSettingsView::emitSignals()
{
    emit changeMax(m_ibMax);
    emit changeMin(m_ibMin);
    emit changeImgBackground(m_imgBackground);
    emit changeImgObject(m_imgObject);
}


//=============================================================================================================

void NeurofeedbackBSettingsView::clickedLoadBackground()
{
    m_sDirBackground = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    m_pui->lineEdit_DirBackground->setText(m_sDirBackground);
    updateStatusBackground();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackBSettingsView::clickedLoadObject()
{
    m_sDirObject = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    m_pui->lineEdit_DirObject->setText(m_sDirObject);
    updateStatusObject();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackBSettingsView::onUpdateSpinBoxMax(int value)
{
    m_ibMax = value;
    emit changeMax(m_ibMax);
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackBSettingsView::updateStatusBackground()
{
    if(m_sDirBackground.isEmpty()){
        m_pui->label_StatusBackground->setText("not loaded");
        m_pui->label_StatusBackground->setStyleSheet("QLabel {color: black;}");
    }
    else {
        QPixmap img(m_sDirBackground);
        m_imgBackground = img;
        if(m_imgBackground.load(m_sDirBackground) == true){
            emit changeImgBackground(m_imgBackground);
            m_pui->label_StatusBackground->setText("loaded");
            m_pui->label_StatusBackground->setStyleSheet("QLabel {color: green;}");
        }
        else{
            m_pui->label_StatusBackground->setText("not loaded");
            m_pui->label_StatusBackground->setStyleSheet("QLabel {color: black;}");
        }

    }
}

//=============================================================================================================

void NeurofeedbackBSettingsView::updateStatusObject()
{
    if(m_sDirObject.isEmpty()){
        m_pui->label_StatusObject->setText("not loaded");
        m_pui->label_StatusObject->setStyleSheet("QLabel {color: black;}");
    }
    else {
        QImage image(m_sDirObject);
        image = image.convertToFormat(QImage::Format_ARGB32);
        for(int i = 0; i<image.width(); ++i){
            for(int j=0; j<image.height(); ++j){
                if(image.pixelColor(i, j) == Qt::white){
                    image.setPixelColor(i,j,QColorConstants::Transparent);
                }
            }
        }
        m_imgObject.convertFromImage(image, Qt::ColorOnly);
        if(image.load(m_sDirObject) == true){
            emit changeImgObject(m_imgObject);
            m_pui->label_StatusObject->setText("loaded");
            m_pui->label_StatusObject->setStyleSheet("QLabel {color: green;}");
        }
        else{
            m_pui->label_StatusObject->setText("not loaded");
            m_pui->label_StatusObject->setStyleSheet("QLabel {color: black;}");
        }
    }
}

//==============================================================================================================

void NeurofeedbackBSettingsView::onUpdateSpinBoxMin(int value)
{
    m_ibMin = value;
    emit changeMin(m_ibMin);
    saveSettings(m_sSettingsPath);
}

//===============================================================================================================

void NeurofeedbackBSettingsView::clickedStdMax()
{
    onUpdateSpinBoxMax(25);
    m_pui->spinBox_Max->setValue(25);
}

//===============================================================================================================

void NeurofeedbackBSettingsView::clickedStdMin()
{
    onUpdateSpinBoxMin(0);
    m_pui->spinBox_Min->setValue(0);
}
