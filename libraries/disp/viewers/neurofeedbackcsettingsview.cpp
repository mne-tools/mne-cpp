//=============================================================================================================
/**
 * @file     neurofeedbackcsettingsview.cpp
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
 * @brief    Definition of the NeurofeedbackCSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedbackcsettingsview.h"
#include "ui_neurofeedbackcsettingsview.h"

#include "neurofeedbackfsettingsview.h"
#include "ui_neurofeedbackfsettingsview.h"

#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QPixmap>
#include <QSlider>
#include <QSettings>
#include <QFileDialog>
#include <QButtonGroup>

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

NeurofeedbackCSettingsView::NeurofeedbackCSettingsView(const QString& sSettingsPath, const QString& sClass0, const QString& sClass1, const QString& sClass2,
                                             const QString& sClass3, const QString& sClass4, int iClass0, int iClass1,
                                             int iClass2, int iClass3, int iClass4, int iNumbofClass, bool bIsRunning,
                                             QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_pui(new Ui::NeurofeedbackCSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_sClass0(sClass0)
, m_sClass1(sClass1)
, m_sClass2(sClass2)
, m_sClass3(sClass3)
, m_sClass4(sClass4)
, m_iClass0(iClass0)
, m_iClass1(iClass1)
, m_iClass2(iClass2)
, m_iClass3(iClass3)
, m_iClass4(iClass4)
, m_iNumbofClass(iNumbofClass)
, m_bIsRunning(bIsRunning)
{
    m_pui->setupUi(this);

    this->setWindowTitle("Neurofeedback Settings");
    this->setMinimumWidth(850);
    this->setMaximumWidth(850);

    loadSettings(m_sSettingsPath);

    m_pui->spinBox_C0->setValue(m_iClass0);
    m_pui->spinBox_C1->setValue(m_iClass1);
    m_pui->spinBox_C2->setValue(m_iClass2);
    m_pui->spinBox_C3->setValue(m_iClass3);
    m_pui->spinBox_C4->setValue(m_iClass4);

    m_vSpinboxClass.push_back(m_pui->spinBox_C0);
    m_vSpinboxClass.push_back(m_pui->spinBox_C1);
    m_vSpinboxClass.push_back(m_pui->spinBox_C2);
    m_vSpinboxClass.push_back(m_pui->spinBox_C3);
    m_vSpinboxClass.push_back(m_pui->spinBox_C4);

    m_pui->lineEdit_Class0->setText(m_sClass0);
    m_pui->lineEdit_Class1->setText(m_sClass1);
    m_pui->lineEdit_Class2->setText(m_sClass2);
    m_pui->lineEdit_Class3->setText(m_sClass3);
    m_pui->lineEdit_Class4->setText(m_sClass4);

    QButtonGroup* buttonGroupC0 = new QButtonGroup();
    buttonGroupC0->addButton(m_pui->radioButton_C0b);
    buttonGroupC0->addButton(m_pui->radioButton_C0g);
    buttonGroupC0->setId(m_pui->radioButton_C0b,0);
    buttonGroupC0->setId(m_pui->radioButton_C0g,1);
    m_vRadioButtonClassb.push_back(m_pui->radioButton_C0b);
    m_vRadioButtonClassg.push_back(m_pui->radioButton_C0g);

    QButtonGroup* buttonGroupC1 = new QButtonGroup();
    buttonGroupC1->addButton(m_pui->radioButton_C1b);
    buttonGroupC1->addButton(m_pui->radioButton_C1g);
    buttonGroupC1->setId(m_pui->radioButton_C1b,0);
    buttonGroupC1->setId(m_pui->radioButton_C1g,1);
    m_vRadioButtonClassb.push_back(m_pui->radioButton_C1b);
    m_vRadioButtonClassg.push_back(m_pui->radioButton_C1g);

    QButtonGroup* buttonGroupC2 = new QButtonGroup();
    buttonGroupC2->addButton(m_pui->radioButton_C2b);
    buttonGroupC2->addButton(m_pui->radioButton_C2g);
    buttonGroupC2->setId(m_pui->radioButton_C2b,0);
    buttonGroupC2->setId(m_pui->radioButton_C2g,1);
    m_vRadioButtonClassb.push_back(m_pui->radioButton_C2b);
    m_vRadioButtonClassg.push_back(m_pui->radioButton_C2g);

    QButtonGroup* buttonGroupC3 = new QButtonGroup();
    buttonGroupC3->addButton(m_pui->radioButton_C3b);
    buttonGroupC3->addButton(m_pui->radioButton_C3g);
    buttonGroupC3->setId(m_pui->radioButton_C3b,0);
    buttonGroupC3->setId(m_pui->radioButton_C3g,1);
    m_vRadioButtonClassb.push_back(m_pui->radioButton_C3b);
    m_vRadioButtonClassg.push_back(m_pui->radioButton_C3g);

    QButtonGroup* buttonGroupC4 = new QButtonGroup();
    buttonGroupC4->addButton(m_pui->radioButton_C4b);
    buttonGroupC4->addButton(m_pui->radioButton_C4g);
    buttonGroupC4->setId(m_pui->radioButton_C4b,0);
    buttonGroupC4->setId(m_pui->radioButton_C4g,1);
    m_vRadioButtonClassb.push_back(m_pui->radioButton_C4b);
    m_vRadioButtonClassg.push_back(m_pui->radioButton_C4g);

    m_vLineEditsClass.push_back(m_pui->lineEdit_Class0);
    m_vLineEditsClass.push_back(m_pui->lineEdit_Class1);
    m_vLineEditsClass.push_back(m_pui->lineEdit_Class2);
    m_vLineEditsClass.push_back(m_pui->lineEdit_Class3);
    m_vLineEditsClass.push_back(m_pui->lineEdit_Class4);

    m_vLabelClass.push_back(m_pui->label_Class0);
    m_vLabelClass.push_back(m_pui->label_Class1);
    m_vLabelClass.push_back(m_pui->label_Class2);
    m_vLabelClass.push_back(m_pui->label_Class3);
    m_vLabelClass.push_back(m_pui->label_Class4);

    m_pui->lineEdit_DirClass0->setReadOnly(true);
    m_pui->lineEdit_DirClass1->setReadOnly(true);
    m_pui->lineEdit_DirClass2->setReadOnly(true);
    m_pui->lineEdit_DirClass3->setReadOnly(true);
    m_pui->lineEdit_DirClass4->setReadOnly(true);

    m_vLabelStatusClass.push_back(m_pui->label_StatusClass0);
    m_vLabelStatusClass.push_back(m_pui->label_StatusClass1);
    m_vLabelStatusClass.push_back(m_pui->label_StatusClass2);
    m_vLabelStatusClass.push_back(m_pui->label_StatusClass3);
    m_vLabelStatusClass.push_back(m_pui->label_StatusClass4);

    m_vLabelImgClass.push_back(m_pui->label_imgClass0);
    m_vLabelImgClass.push_back(m_pui->label_imgClass1);
    m_vLabelImgClass.push_back(m_pui->label_imgClass2);
    m_vLabelImgClass.push_back(m_pui->label_imgClass3);
    m_vLabelImgClass.push_back(m_pui->label_imgClass4);

    m_vLineEditsDirClass.push_back(m_pui->lineEdit_DirClass0);
    m_vLineEditsDirClass.push_back(m_pui->lineEdit_DirClass1);
    m_vLineEditsDirClass.push_back(m_pui->lineEdit_DirClass2);
    m_vLineEditsDirClass.push_back(m_pui->lineEdit_DirClass3);
    m_vLineEditsDirClass.push_back(m_pui->lineEdit_DirClass4);

    m_vPushButtonsClass.push_back(m_pui->pushButton_Class0);
    m_vPushButtonsClass.push_back(m_pui->pushButton_Class1);
    m_vPushButtonsClass.push_back(m_pui->pushButton_Class2);
    m_vPushButtonsClass.push_back(m_pui->pushButton_Class3);
    m_vPushButtonsClass.push_back(m_pui->pushButton_Class4);

    m_vDeleteButtonsClass.push_back(m_pui->pushButton_DClass0);
    m_vDeleteButtonsClass.push_back(m_pui->pushButton_DClass1);
    m_vDeleteButtonsClass.push_back(m_pui->pushButton_DClass2);
    m_vDeleteButtonsClass.push_back(m_pui->pushButton_DClass3);
    m_vDeleteButtonsClass.push_back(m_pui->pushButton_DClass4);

    m_pui->spinBoxnC->setMinimum(2);
    m_pui->spinBoxnC->setMaximum(5);
    m_pui->spinBoxnC->setValue(m_iNumbofClass);
    onUpdateSpinBoxNumbofClass(m_iNumbofClass);


    connect(m_pui->spinBoxnC, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinBoxNumbofClass);
    connect(m_pui->lineEdit_Class0, static_cast<void (QLineEdit::*)(const QString &)> (&QLineEdit::textEdited), this, &NeurofeedbackCSettingsView::onUpdateLineEditClass0);
    connect(m_pui->lineEdit_Class1, static_cast<void (QLineEdit::*)(const QString &)> (&QLineEdit::textEdited), this, &NeurofeedbackCSettingsView::onUpdateLineEditClass1);
    connect(m_pui->lineEdit_Class2, static_cast<void (QLineEdit::*)(const QString &)> (&QLineEdit::textEdited), this, &NeurofeedbackCSettingsView::onUpdateLineEditClass2);
    connect(m_pui->lineEdit_Class3, static_cast<void (QLineEdit::*)(const QString &)> (&QLineEdit::textEdited), this, &NeurofeedbackCSettingsView::onUpdateLineEditClass3);
    connect(m_pui->lineEdit_Class4, static_cast<void (QLineEdit::*)(const QString &)> (&QLineEdit::textEdited), this, &NeurofeedbackCSettingsView::onUpdateLineEditClass4);
    connect(buttonGroupC0, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackCSettingsView::onUpdateButtonGroupC0);
    connect(buttonGroupC1, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackCSettingsView::onUpdateButtonGroupC1);
    connect(buttonGroupC2, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackCSettingsView::onUpdateButtonGroupC2);
    connect(buttonGroupC3, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackCSettingsView::onUpdateButtonGroupC3);
    connect(buttonGroupC4, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this, &NeurofeedbackCSettingsView::onUpdateButtonGroupC4);
    connect(m_pui->spinBox_C0, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinboxClass0);
    connect(m_pui->spinBox_C1, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinboxClass1);
    connect(m_pui->spinBox_C2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinboxClass2);
    connect(m_pui->spinBox_C3, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinboxClass3);
    connect(m_pui->spinBox_C4, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackCSettingsView::onUpdateSpinboxClass4);
    connect(m_pui->pushButton_Class0, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedLoadButton0);
    connect(m_pui->pushButton_Class1, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedLoadButton1);
    connect(m_pui->pushButton_Class2, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedLoadButton2);
    connect(m_pui->pushButton_Class3, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedLoadButton3);
    connect(m_pui->pushButton_Class4, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedLoadButton4);
    connect(m_pui->pushButton_DClass0, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedDeleteButton0);
    connect(m_pui->pushButton_DClass1, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedDeleteButton1);
    connect(m_pui->pushButton_DClass2, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedDeleteButton2);
    connect(m_pui->pushButton_DClass3, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedDeleteButton3);
    connect(m_pui->pushButton_DClass4, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedDeleteButton4);
    connect(m_pui->pushButton_ResetSettings, &QPushButton::released, this, &NeurofeedbackCSettingsView::clickedResetSettings);

    updateDisplay();
}
//=============================================================================================================

NeurofeedbackCSettingsView::~NeurofeedbackCSettingsView()
{
    saveSettings(m_sSettingsPath);
    delete m_pui;
}

//=============================================================================================================

void NeurofeedbackCSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Store Settings
    QSettings settings;

    settings.setValue(settingsPath + QString("/numbofClass"), m_iNumbofClass);
    settings.setValue(settingsPath + QString("/Class0"), m_sClass0);
    settings.setValue(settingsPath + QString("/Class1"), m_sClass1);
    settings.setValue(settingsPath + QString("/Class2"), m_sClass2);
    settings.setValue(settingsPath + QString("/Class3"), m_sClass3);
    settings.setValue(settingsPath + QString("/Class4"), m_sClass4);
    settings.setValue(settingsPath + QString("/iClass0"), m_iClass0);
    settings.setValue(settingsPath + QString("/iClass1"), m_iClass1);
    settings.setValue(settingsPath + QString("/iClass2"), m_iClass2);
    settings.setValue(settingsPath + QString("/iClass3"), m_iClass3);
    settings.setValue(settingsPath + QString("/iClass4"), m_iClass4);
    settings.setValue(settingsPath + QString("/sDirClass0"), m_sDirClass0);
    settings.setValue(settingsPath + QString("/sDirClass1"), m_sDirClass1);
    settings.setValue(settingsPath + QString("/sDirClass2"), m_sDirClass2);
    settings.setValue(settingsPath + QString("/sDirClass3"), m_sDirClass3);
    settings.setValue(settingsPath + QString("/sDirClass4"), m_sDirClass4);
    settings.setValue(settingsPath + QString("/sColClass0"), m_sColClass0);
    settings.setValue(settingsPath + QString("/sColClass1"), m_sColClass1);
    settings.setValue(settingsPath + QString("/sColClass2"), m_sColClass2);
    settings.setValue(settingsPath + QString("/sColClass3"), m_sColClass3);
    settings.setValue(settingsPath + QString("/sColClass4"), m_sColClass4);


}

//=============================================================================================================

void NeurofeedbackCSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings;

    m_iNumbofClass = settings.value(settingsPath + QString("/numbofClass"), m_iNumbofClass).toInt();
    m_sClass0 = settings.value(settingsPath + QString("/Class0"), m_sClass0).toString();
    m_sClass1 = settings.value(settingsPath + QString("/Class1"), m_sClass1).toString();
    m_sClass2 = settings.value(settingsPath + QString("/Class2"), m_sClass2).toString();
    m_sClass3 = settings.value(settingsPath + QString("/Class3"), m_sClass3).toString();
    m_sClass4 = settings.value(settingsPath + QString("/Class4"), m_sClass4).toString();
    m_iClass0 = settings.value(settingsPath + QString("/iClass0"), m_iClass0).toInt();
    m_iClass1 = settings.value(settingsPath + QString("/iClass1"), m_iClass1).toInt();
    m_iClass2 = settings.value(settingsPath + QString("/iClass2"), m_iClass2).toInt();
    m_iClass3 = settings.value(settingsPath + QString("/iClass3"), m_iClass3).toInt();
    m_iClass4 = settings.value(settingsPath + QString("/iClass4"), m_iClass4).toInt();
    m_sDirClass0 = settings.value(settingsPath + QString("/sDirClass0"), m_sDirClass0).toString();
    m_sDirClass1 = settings.value(settingsPath + QString("/sDirClass1"), m_sDirClass1).toString();
    m_sDirClass2 = settings.value(settingsPath + QString("/sDirClass2"), m_sDirClass2).toString();
    m_sDirClass3 = settings.value(settingsPath + QString("/sDirClass3"), m_sDirClass3).toString();
    m_sDirClass4 = settings.value(settingsPath + QString("/sDirClass4"), m_sDirClass4).toString();
    m_sColClass0 = settings.value(settingsPath + QString("/sColClass0"), m_sColClass0).toString();
    m_sColClass1 = settings.value(settingsPath + QString("/sColClass1"), m_sColClass1).toString();
    m_sColClass2 = settings.value(settingsPath + QString("/sColClass2"), m_sColClass2).toString();
    m_sColClass3 = settings.value(settingsPath + QString("/sColClass3"), m_sColClass3).toString();
    m_sColClass4 = settings.value(settingsPath + QString("/sColClass4"), m_sColClass4).toString();

}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedResetSettings()
{
    m_iNumbofClass = 2;

    m_sClass0 = "Class0";
    m_sClass1 = "Class1";
    m_sClass2 = "Class2";
    m_sClass3 = "Class3";
    m_sClass4 = "Class4";

    m_iClass0 = 0;
    m_iClass1 = 1;
    m_iClass2 = 2;
    m_iClass3 = 3;
    m_iClass4 = 4;

    m_sColClass0 = "black";
    m_sColClass1 = "black";
    m_sColClass2 = "black";
    m_sColClass3 = "black";
    m_sColClass4 = "black";

    m_sDirClass0 = "";
    m_sDirClass1 = "";
    m_sDirClass2 = "";
    m_sDirClass3 = "";
    m_sDirClass4 = "";


    saveSettings(m_sSettingsPath);
    emitSignals();
    updateDisplay();

}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateDisplay()
{
    onUpdateSpinBoxNumbofClass(m_iNumbofClass);
    m_pui->spinBoxnC->setValue(m_iNumbofClass);

    m_pui->lineEdit_Class0->setText(m_sClass0);
    m_pui->lineEdit_Class1->setText(m_sClass1);
    m_pui->lineEdit_Class2->setText(m_sClass2);
    m_pui->lineEdit_Class3->setText(m_sClass3);
    m_pui->lineEdit_Class4->setText(m_sClass4);

    m_pui->spinBox_C0->setValue(m_iClass0);
    m_pui->spinBox_C1->setValue(m_iClass1);
    m_pui->spinBox_C2->setValue(m_iClass2);
    m_pui->spinBox_C3->setValue(m_iClass3);
    m_pui->spinBox_C4->setValue(m_iClass4);

    if(m_sColClass0 == "red"){
        onUpdateButtonGroupC0(0);
        m_pui->radioButton_C0b->setChecked(true);
    }
    else if(m_sColClass0 == "green"){
        onUpdateButtonGroupC0(1);
        m_pui->radioButton_C0g->setChecked(true);
    }
    else if(m_sColClass0 == "black"){        

    }

    if(m_sColClass1 == "red"){
        onUpdateButtonGroupC1(0);
        m_pui->radioButton_C1b->setChecked(true);
    }
    else if(m_sColClass1 == "green"){
        onUpdateButtonGroupC1(1);
        m_pui->radioButton_C1g->setChecked(true);
    }
    else if(m_sColClass1 == "black"){

    }

    if(m_sColClass2 == "red"){
        onUpdateButtonGroupC2(0);
        m_pui->radioButton_C2b->setChecked(true);
    }
    else if(m_sColClass2 == "green"){
        onUpdateButtonGroupC2(1);
        m_pui->radioButton_C2g->setChecked(true);
    }
    else if(m_sColClass2 == "black"){

    }

    if(m_sColClass3 == "red"){
        onUpdateButtonGroupC3(0);
        m_pui->radioButton_C3b->setChecked(true);
    }
    else if(m_sColClass3 == "green"){
        onUpdateButtonGroupC3(1);
        m_pui->radioButton_C3g->setChecked(true);
    }
    else if(m_sColClass3 == "black"){

    }

    if(m_sColClass4 == "red"){
        onUpdateButtonGroupC4(0);
        m_pui->radioButton_C4b->setChecked(true);
    }
    else if(m_sColClass4 == "green"){
        onUpdateButtonGroupC4(1);
        m_pui->radioButton_C4g->setChecked(true);
    }
    else if(m_sColClass4 == "black"){

    }

    updateImgClass0();
    updateImgClass1();
    updateImgClass2();
    updateImgClass3();
    updateImgClass4();

}

//=============================================================================================================

void NeurofeedbackCSettingsView::emitSignals()
{
    emit changeNumbofClass(m_iNumbofClass);
    emit changeClass0(m_sClass0);
    emit changeClass1(m_sClass1);
    emit changeClass2(m_sClass2);
    emit changeClass3(m_sClass3);
    emit changeClass4(m_sClass4);
    emit changeiClass0(m_iClass0);
    emit changeiClass1(m_iClass1);
    emit changeiClass2(m_iClass2);
    emit changeiClass3(m_iClass3);
    emit changeiClass4(m_iClass4);
    emit changeImgClass0(m_imgClass0);
    emit changeImgClass1(m_imgClass1);
    emit changeImgClass2(m_imgClass2);
    emit changeImgClass3(m_imgClass3);
    emit changeImgClass4(m_imgClass4);
    emit changeButtonGroupC0(m_sColClass0);
    emit changeButtonGroupC1(m_sColClass1);
    emit changeButtonGroupC2(m_sColClass2);
    emit changeButtonGroupC3(m_sColClass3);
    emit changeButtonGroupC4(m_sColClass4);
}

//============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinBoxNumbofClass(int value){
    m_iNumbofClass = value;
    emit changeNumbofClass(m_iNumbofClass);

    saveSettings(m_sSettingsPath);

    for(int i = 0; i<m_vLineEditsClass.size(); ++i){
        if(i<m_iNumbofClass){
            m_vLineEditsClass[i]->show();
            m_vLabelClass[i]->show();
            m_vSpinboxClass[i]->show();
            m_vLabelStatusClass[i]->show();
            m_vLineEditsDirClass[i]->show();
            m_vPushButtonsClass[i]->show();
            m_vRadioButtonClassb[i]->show();
            m_vRadioButtonClassg[i]->show();
            m_vDeleteButtonsClass[i]->show();
            m_vLabelImgClass[i]->show();
        }
        else{
            m_vLineEditsClass[i]->hide();
            m_vLabelClass[i]->hide();
            m_vSpinboxClass[i]->hide();
            m_vLabelStatusClass[i]->hide();
            m_vLineEditsDirClass[i]->hide();
            m_vPushButtonsClass[i]->hide();
            m_vRadioButtonClassb[i]->hide();
            m_vRadioButtonClassg[i]->hide();
            m_vDeleteButtonsClass[i]->hide();
            m_vLabelImgClass[i]->hide();
        }
    }
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateLineEditClass0()
{

    m_sClass0 = m_pui->lineEdit_Class0->text();

    emit changeClass0(m_sClass0);

    saveSettings(m_sSettingsPath);

}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateLineEditClass1()
{

    m_sClass1 = m_pui->lineEdit_Class1->text();

    emit changeClass1(m_sClass1);

    saveSettings(m_sSettingsPath);

}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateLineEditClass2()
{

    m_sClass2 = m_pui->lineEdit_Class2->text();

    emit changeClass2(m_sClass2);

    saveSettings(m_sSettingsPath);

}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateLineEditClass3()
{

    m_sClass3 = m_pui->lineEdit_Class3->text();

    emit changeClass3(m_sClass3);

    saveSettings(m_sSettingsPath);

}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateLineEditClass4()
{

    m_sClass4 = m_pui->lineEdit_Class4->text();

    emit changeClass4(m_sClass4);

    saveSettings(m_sSettingsPath);

}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateButtonGroupC0(int value)
{
    if(value==0){
        m_sColClass0 = "red";
    }
    else if(value==1){
        m_sColClass0 = "green";
    }
    emit changeButtonGroupC0(m_sColClass0);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateButtonGroupC1(int value)
{
    if(value==0){
        m_sColClass1 = "red";
    }
    else if(value==1){
        m_sColClass1 = "green";
    }
    emit changeButtonGroupC1(m_sColClass1);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateButtonGroupC2(int value)
{
    if(value==0){
        m_sColClass2 = "red";
    }
    else if(value==1){
        m_sColClass2 = "green";
    }
    emit changeButtonGroupC2(m_sColClass2);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateButtonGroupC3(int value)
{
    if(value==0){
        m_sColClass3 = "red";
    }
    else if(value==1){
        m_sColClass3 = "green";
    }
    emit changeButtonGroupC3(m_sColClass3);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateButtonGroupC4(int value)
{
    if(value==0){
        m_sColClass4 = "red";
    }
    else if(value==1){
        m_sColClass4 = "green";
    }
    emit changeButtonGroupC4(m_sColClass4);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinboxClass0(int value)
{

    m_iClass0 = value;

    emit changeiClass0(m_iClass0);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinboxClass1(int value)
{

    m_iClass1 = value;

    emit changeiClass1(m_iClass1);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinboxClass2(int value)
{

    m_iClass2 = value;

    emit changeiClass2(m_iClass2);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinboxClass3(int value)
{

    m_iClass3 = value;

    emit changeiClass3(m_iClass3);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::onUpdateSpinboxClass4(int value)
{

    m_iClass4 = value;

    emit changeiClass4(m_iClass4);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedLoadButton0()
{
    m_sDirClass0 = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    m_pui->lineEdit_DirClass0->setEnabled(true);
    updateImgClass0();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedLoadButton1()
{
    m_sDirClass1 = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    updateImgClass1();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedLoadButton2()
{
    m_sDirClass2 = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    updateImgClass2();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedLoadButton3()
{
    m_sDirClass3 = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    updateImgClass3();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedLoadButton4()
{
    m_sDirClass4 = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg)"));
    updateImgClass4();
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateImgClass0()
{
    m_pui->lineEdit_DirClass0->setText(m_sDirClass0);
    if(m_sDirClass0.isEmpty()){
        clickedDeleteButton0();
    }
    else {
        QPixmap image(m_sDirClass0);
        m_imgClass0 = image;
        if(m_imgClass0.load(m_sDirClass0) == true){
            emit changeImgClass0(m_imgClass0);
            m_pui->label_StatusClass0->setText("loaded");
            m_pui->label_StatusClass0->setStyleSheet("QLabel {color: green;}");
            m_pui->label_imgClass0->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
            m_pui->lineEdit_Class0->setEnabled(false);
            m_pui->lineEdit_DirClass0->setEnabled(true);
        }
        else{
            m_pui->label_StatusClass0->setText("not loaded");
            m_pui->label_StatusClass0->setStyleSheet("QLabel {color: black;}");
            m_pui->label_imgClass0->setText("");
        }
    }
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateImgClass1()
{
    m_pui->lineEdit_DirClass1->setText(m_sDirClass1);
    if(m_sDirClass1.isEmpty()){
        clickedDeleteButton1();
    }
    else {
        QPixmap image(m_sDirClass1);
        m_imgClass1 = image;
        if(m_imgClass1.load(m_sDirClass1) == true){
            emit changeImgClass1(m_imgClass1);
            m_pui->label_StatusClass1->setText("loaded");
            m_pui->label_StatusClass1->setStyleSheet("QLabel {color: green;}");
            m_pui->label_imgClass1->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
            m_pui->lineEdit_Class1->setEnabled(false);
            m_pui->lineEdit_DirClass1->setEnabled(true);
        }
        else{
            m_pui->label_StatusClass1->setText("not loaded");
            m_pui->label_StatusClass1->setStyleSheet("QLabel {color: black;}");
            m_pui->label_imgClass1->setText("");
        }
    }
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateImgClass2()
{
    m_pui->lineEdit_DirClass2->setText(m_sDirClass2);
    if(m_sDirClass2.isEmpty()){
        clickedDeleteButton2();
    }
    else {
        QPixmap image(m_sDirClass2);
        m_imgClass2 = image;
        if(m_imgClass2.load(m_sDirClass2) == true){
            emit changeImgClass2(m_imgClass2);
            m_pui->label_StatusClass2->setText("loaded");
            m_pui->label_StatusClass2->setStyleSheet("QLabel {color: green;}");
            m_pui->label_imgClass2->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
            m_pui->lineEdit_Class2->setEnabled(false);
            m_pui->lineEdit_DirClass2->setEnabled(true);
        }
        else{
            m_pui->label_StatusClass2->setText("not loaded");
            m_pui->label_StatusClass2->setStyleSheet("QLabel {color: black;}");
            m_pui->label_imgClass2->setText("");
        }
    }
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateImgClass3()
{
    m_pui->lineEdit_DirClass3->setText(m_sDirClass3);
    if(m_sDirClass3.isEmpty()){
        clickedDeleteButton3();
    }
    else {
        QPixmap image(m_sDirClass3);
        m_imgClass3 = image;
        if(m_imgClass3.load(m_sDirClass3) == true){
            emit changeImgClass3(m_imgClass3);
            m_pui->label_StatusClass3->setText("loaded");
            m_pui->label_StatusClass3->setStyleSheet("QLabel {color: green;}");
            m_pui->label_imgClass3->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
            m_pui->lineEdit_Class3->setEnabled(false);
            m_pui->lineEdit_DirClass3->setEnabled(true);
        }
        else{
            m_pui->label_StatusClass3->setText("not loaded");
            m_pui->label_StatusClass3->setStyleSheet("QLabel {color: black;}");
            m_pui->label_imgClass3->setText("");
        }
    }
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::updateImgClass4()
{
    m_pui->lineEdit_DirClass4->setText(m_sDirClass4);
    if(m_sDirClass4.isEmpty()){
        clickedDeleteButton4();
    }
    else {
        QPixmap image(m_sDirClass4);
        m_imgClass4 = image;
        if(m_imgClass4.load(m_sDirClass4) == true){
            emit changeImgClass4(m_imgClass4);
            m_pui->label_StatusClass4->setText("loaded");
            m_pui->label_StatusClass4->setStyleSheet("QLabel {color: green;}");
            m_pui->label_imgClass4->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
            m_pui->lineEdit_Class4->setEnabled(false);
            m_pui->lineEdit_DirClass4->setEnabled(true);
        }
        else{
            m_pui->label_StatusClass4->setText("not loaded");
            m_pui->label_StatusClass4->setStyleSheet("QLabel {color: black;}");
            m_pui->label_imgClass4->setText("");
        }
    }
    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedDeleteButton0()
{
    m_pui->lineEdit_DirClass0->setText("");
    m_pui->lineEdit_DirClass0->setEnabled(false);
    m_pui->label_StatusClass0->setText("not loaded");
    m_pui->label_StatusClass0->setStyleSheet("QLabel {color: black;}");
    m_pui->label_imgClass0->clear();
    m_imgClass0 = QPixmap();
    m_sDirClass0= "";
    m_pui->lineEdit_Class0->setEnabled(true);
    emit changeImgClass0(m_imgClass0);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedDeleteButton1()
{
    m_pui->lineEdit_DirClass1->setText("");
    m_pui->lineEdit_DirClass1->setEnabled(false);
    m_pui->label_StatusClass1->setText("not loaded");
    m_pui->label_StatusClass1->setStyleSheet("QLabel {color: black;}");
    m_pui->label_imgClass1->clear();
    m_imgClass1 = QPixmap();
    m_sDirClass1 = "";
    m_pui->lineEdit_Class1->setEnabled(true);
    emit changeImgClass1(m_imgClass1);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedDeleteButton2()
{
    m_pui->lineEdit_DirClass2->setText("");
    m_pui->lineEdit_DirClass2->setEnabled(false);
    m_pui->label_StatusClass2->setText("not loaded");
    m_pui->label_StatusClass2->setStyleSheet("QLabel {color: black;}");
    m_pui->label_imgClass2->clear();
    m_imgClass2 = QPixmap();
    m_sDirClass2 = "";
    m_pui->lineEdit_Class2->setEnabled(true);
    emit changeImgClass2(m_imgClass2);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedDeleteButton3()
{
    m_pui->lineEdit_DirClass3->setText("");
    m_pui->lineEdit_DirClass3->setEnabled(false);
    m_pui->label_StatusClass3->setText("not loaded");
    m_pui->label_StatusClass3->setStyleSheet("QLabel {color: black;}");
    m_pui->label_imgClass3->clear();
    m_imgClass3 = QPixmap();
    m_sDirClass3 = "";
    m_pui->lineEdit_Class3->setEnabled(true);
    emit changeImgClass3(m_imgClass3);
}

//=============================================================================================================

void NeurofeedbackCSettingsView::clickedDeleteButton4()
{
    m_pui->lineEdit_DirClass4->setText("");
    m_pui->lineEdit_DirClass4->setEnabled(false);
    m_pui->label_StatusClass4->setText("not loaded");
    m_pui->label_StatusClass4->setStyleSheet("QLabel {color: black;}");
    m_pui->label_imgClass4->clear();
    m_imgClass4 = QPixmap();
    m_sDirClass4 = "";
    m_pui->lineEdit_Class4->setEnabled(true);
    emit changeImgClass4(m_imgClass4);
}
