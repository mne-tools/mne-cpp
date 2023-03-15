//=============================================================================================================
/**
 * @file     brainampsetupprojectwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the BrainAMPSetupProjectWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampsetupprojectwidget.h"

#include "../brainamp.h"

#include "ui_brainampsetupprojectwidget.h"

#include <utils/layoutloader.h>
#include <utils/layoutmaker.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QDate>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMPSetupProjectWidget::BrainAMPSetupProjectWidget(BrainAMP* pBrainAMP, QWidget *parent)
: QWidget(parent)
, ui(new Ui::BrainAMPSetupProjectWidget)
, m_pBrainAMP(pBrainAMP)
{
    ui->setupUi(this);

    // Connect EEG hat
    connect(ui->m_qPushButton_EEGCap, &QPushButton::released, this, &BrainAMPSetupProjectWidget::changeCap);
    connect(ui->m_qLineEdit_EEGCap, &QLineEdit::textChanged, this, &BrainAMPSetupProjectWidget::updateCardinalComboBoxes);

    // Connect cardinal combo boxes and shift spin boxes
    connect(ui->m_comboBox_cardinalMode, &QComboBox::currentTextChanged, this, &BrainAMPSetupProjectWidget::changeCardinalMode);

    connect(ui->m_comboBox_LPA, &QComboBox::currentTextChanged, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);
    connect(ui->m_comboBox_RPA, &QComboBox::currentTextChanged, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);
    connect(ui->m_comboBox_Nasion, &QComboBox::currentTextChanged, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);
    connect(ui->m_doubleSpinBox_LPA, &QDoubleSpinBox::editingFinished, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);
    connect(ui->m_doubleSpinBox_RPA, &QDoubleSpinBox::editingFinished, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);
    connect(ui->m_doubleSpinBox_Nasion, &QDoubleSpinBox::editingFinished, this, &BrainAMPSetupProjectWidget::onCardinalComboBoxChanged);

    connect(ui->m_pushButton_cardinalFile, &QPushButton::released, this, &BrainAMPSetupProjectWidget::changeCardinalFile);

    // Connect QLineEdit's
    connect(ui->m_qLineEdit_EEGCap, static_cast<void (QLineEdit::*)(const QString &)>(&QLineEdit::textEdited),
            this, &BrainAMPSetupProjectWidget::changeQLineEdits);

    initGui();
}

//=============================================================================================================

BrainAMPSetupProjectWidget::~BrainAMPSetupProjectWidget()
{
    delete ui;
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::initGui()
{
    // Init location of layout file
    ui->m_qLineEdit_EEGCap->setText(m_pBrainAMP->m_sElcFilePath);

    updateCardinalComboBoxes(m_pBrainAMP->m_sElcFilePath);

    ui->m_doubleSpinBox_LPA->setValue(1e2*m_pBrainAMP->m_dLPAShift);
    ui->m_doubleSpinBox_RPA->setValue(1e2*m_pBrainAMP->m_dRPAShift);
    ui->m_doubleSpinBox_Nasion->setValue(1e2*m_pBrainAMP->m_dNasionShift);

    ui->m_comboBox_LPA->setCurrentText(m_pBrainAMP->m_sLPA);
    ui->m_comboBox_RPA->setCurrentText(m_pBrainAMP->m_sRPA);
    ui->m_comboBox_Nasion->setCurrentText(m_pBrainAMP->m_sNasion);

    ui->m_lineEdit_cardinalFile->setText(m_pBrainAMP->m_sCardinalFilePath);

    //Init cardinal support
    if(m_pBrainAMP->m_bUseTrackedCardinalMode) {
        ui->m_comboBox_cardinalMode->setCurrentText("Use tracked cardinals");
        changeCardinalMode("Use tracked cardinals");
    } else if (m_pBrainAMP->m_bUseElectrodeShiftMode) {
        ui->m_comboBox_cardinalMode->setCurrentText("Use electrode shift");
        changeCardinalMode("Use electrode shift");
    }
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::changeCardinalMode(const QString& text)
{
    if(text == "Use tracked cardinals") {
        ui->m_label_cardinal->show();
        ui->m_lineEdit_cardinalFile->show();
        ui->m_pushButton_cardinalFile->show();

        ui->m_label_LPA->hide();
        ui->m_doubleSpinBox_LPA->hide();
        ui->m_comboBox_LPA->hide();
        ui->m_label_RPA->hide();
        ui->m_doubleSpinBox_RPA->hide();
        ui->m_comboBox_RPA->hide();
        ui->m_label_Nasion->hide();
        ui->m_doubleSpinBox_Nasion->hide();
        ui->m_comboBox_Nasion->hide();

        m_pBrainAMP->m_bUseTrackedCardinalMode = true;
        m_pBrainAMP->m_bUseElectrodeShiftMode = false;
    } else if(text == "Use electrode shift") {
        ui->m_label_cardinal->hide();
        ui->m_lineEdit_cardinalFile->hide();
        ui->m_pushButton_cardinalFile->hide();

        ui->m_label_LPA->show();
        ui->m_doubleSpinBox_LPA->show();
        ui->m_comboBox_LPA->show();
        ui->m_label_RPA->show();
        ui->m_doubleSpinBox_RPA->show();
        ui->m_comboBox_RPA->show();
        ui->m_label_Nasion->show();
        ui->m_doubleSpinBox_Nasion->show();
        ui->m_comboBox_Nasion->show();

        m_pBrainAMP->m_bUseTrackedCardinalMode = false;
        m_pBrainAMP->m_bUseElectrodeShiftMode = true;
    }

    this->adjustSize();
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::onCardinalComboBoxChanged()
{
    QString sLPA = ui->m_comboBox_LPA->currentText();
    double dLPAShift = ui->m_doubleSpinBox_LPA->value()*1e-2;
    QString sRPA = ui->m_comboBox_RPA->currentText();
    double dRPAShift = ui->m_doubleSpinBox_RPA->value()*1e-2;
    QString sNasion = ui->m_comboBox_Nasion->currentText();
    double dNasionShift = ui->m_doubleSpinBox_Nasion->value()*1e-2;

    emit cardinalPointsChanged(sLPA, dLPAShift, sRPA, dRPAShift, sNasion, dNasionShift);
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::updateCardinalComboBoxes(const QString& sPath)
{
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(sPath, elcChannelNames, elcLocation3D, elcLocation2D, unit)) {
        qDebug() << "Error: Reading elc file.";
        return;
    }

    ui->m_comboBox_LPA->clear();
    ui->m_comboBox_RPA->clear();
    ui->m_comboBox_Nasion->clear();

    ui->m_comboBox_LPA->addItems(elcChannelNames);
    ui->m_comboBox_RPA->addItems(elcChannelNames);
    ui->m_comboBox_Nasion->addItems(elcChannelNames);
}
//=============================================================================================================

void BrainAMPSetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                ../resources/mne_scan/plugins/tmsi/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL){
        path = ui->m_qLineEdit_EEGCap->text();
    }

    ui->m_qLineEdit_EEGCap->setText(path);
    m_pBrainAMP->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::changeCardinalFile()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change cardinal file",
                                                ../resources/mne_scan/plugins/tmsi/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL)
        path = ui->m_lineEdit_cardinalFile->text();

    ui->m_lineEdit_cardinalFile->setText(path);
    m_pBrainAMP->m_sCardinalFilePath = ui->m_lineEdit_cardinalFile->text();
}

//=============================================================================================================

void BrainAMPSetupProjectWidget::changeQLineEdits()
{
    m_pBrainAMP->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}
