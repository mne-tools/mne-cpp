//=============================================================================================================
/**
 * @file     eegosportssetupprojectwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     July 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsSetupProjectWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportssetupprojectwidget.h"
#include "ui_eegosportssetupprojectwidget.h"
#include "../eegosports.h"

#include <utils/layoutloader.h>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QDate>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace UTILSLIB;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsSetupProjectWidget::EEGoSportsSetupProjectWidget(EEGoSports* pEEGoSports, QWidget *parent)
: QWidget(parent)
, m_pUi(new Ui::EEGoSportsSetupProjectWidget)
, m_pEEGoSports(pEEGoSports)
{
    m_pUi->setupUi(this);

    // Connect write to file options
    connect(m_pUi->m_qPushButton_NewProject, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::addProject);
    connect(m_pUi->m_qPushButton_NewSubject, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::addSubject);
    connect(m_pUi->m_qPushButton_FiffRecordFile, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::changeOutputFile);

    // Connect drop down menus
    connect(m_pUi->m_qComboBox_SubjectSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupProjectWidget::generateFilePath);
    connect(m_pUi->m_qComboBox_ProjectSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupProjectWidget::generateFilePath);

    // Connect EEG hat
    connect(m_pUi->m_qPushButton_EEGCap, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::changeCap);
    connect(m_pUi->m_qLineEdit_EEGCap, &QLineEdit::textChanged, this, &EEGoSportsSetupProjectWidget::updateCardinalComboBoxes);

    // Connect cardinal combo boxes and shift spin boxes
    connect(m_pUi->m_comboBox_cardinalMode, &QComboBox::currentTextChanged, this, &EEGoSportsSetupProjectWidget::changeCardinalMode);

    connect(m_pUi->m_comboBox_LPA, &QComboBox::currentTextChanged, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);
    connect(m_pUi->m_comboBox_RPA, &QComboBox::currentTextChanged, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);
    connect(m_pUi->m_comboBox_Nasion, &QComboBox::currentTextChanged, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);
    connect(m_pUi->m_doubleSpinBox_LPA, &QDoubleSpinBox::editingFinished, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);
    connect(m_pUi->m_doubleSpinBox_RPA, &QDoubleSpinBox::editingFinished, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);
    connect(m_pUi->m_doubleSpinBox_Nasion, &QDoubleSpinBox::editingFinished, this, &EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged);

    connect(m_pUi->m_pushButton_cardinalFile, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::changeCardinalFile);

    // Connect QLineEdit's
    connect(m_pUi->m_qLineEdit_EEGCap, static_cast<void (QLineEdit::*)(const QString &)>(&QLineEdit::textEdited),
            this, &EEGoSportsSetupProjectWidget::changeQLineEdits);
    connect(m_pUi->m_qLineEdit_FiffRecordFile, static_cast<void (QLineEdit::*)(const QString &)>(&QLineEdit::textEdited),
            this, &EEGoSportsSetupProjectWidget::changeQLineEdits);

    initGui();
}


//=============================================================================================================

EEGoSportsSetupProjectWidget::~EEGoSportsSetupProjectWidget()
{
    delete m_pUi;
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::initGui()
{
    // Init output file path
    m_pUi->m_qLineEdit_FiffRecordFile->setText(m_pEEGoSports->m_sOutputFilePath);

    // Init location of layout file
    m_pUi->m_qLineEdit_EEGCap->setText(m_pEEGoSports->m_sElcFilePath);

    updateCardinalComboBoxes(m_pEEGoSports->m_sElcFilePath);

    m_pUi->m_doubleSpinBox_LPA->setValue(1e2*m_pEEGoSports->m_dLPAShift);
    m_pUi->m_doubleSpinBox_RPA->setValue(1e2*m_pEEGoSports->m_dRPAShift);
    m_pUi->m_doubleSpinBox_Nasion->setValue(1e2*m_pEEGoSports->m_dNasionShift);

    m_pUi->m_comboBox_LPA->setCurrentText(m_pEEGoSports->m_sLPA);
    m_pUi->m_comboBox_RPA->setCurrentText(m_pEEGoSports->m_sRPA);
    m_pUi->m_comboBox_Nasion->setCurrentText(m_pEEGoSports->m_sNasion);

    m_pUi->m_lineEdit_cardinalFile->setText(m_pEEGoSports->m_sCardinalFilePath);

    // Init project and subject menus
    m_pUi->m_qComboBox_ProjectSelection->addItem("Sequence_01");
    m_pUi->m_qComboBox_SubjectSelection->addItem("Subject_01");
    m_pUi->m_qComboBox_ProjectSelection->addItem("Sequence_02");
    m_pUi->m_qComboBox_SubjectSelection->addItem("Subject_02");

    // Init file name
    generateFilePath();

    //Init cardinal support
    if(m_pEEGoSports->m_bUseTrackedCardinalMode) {
        m_pUi->m_comboBox_cardinalMode->setCurrentText("Use tracked cardinals");
        changeCardinalMode("Use tracked cardinals");
    } else if (m_pEEGoSports->m_bUseElectrodeShiftMode) {
        m_pUi->m_comboBox_cardinalMode->setCurrentText("Use electrode shift");
        changeCardinalMode("Use electrode shift");
    }
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::changeCardinalMode(const QString& text)
{
    if(text == "Use tracked cardinals") {
        m_pUi->m_label_cardinal->show();
        m_pUi->m_lineEdit_cardinalFile->show();
        m_pUi->m_pushButton_cardinalFile->show();

        m_pUi->m_label_LPA->hide();
        m_pUi->m_doubleSpinBox_LPA->hide();
        m_pUi->m_comboBox_LPA->hide();
        m_pUi->m_label_RPA->hide();
        m_pUi->m_doubleSpinBox_RPA->hide();
        m_pUi->m_comboBox_RPA->hide();
        m_pUi->m_label_Nasion->hide();
        m_pUi->m_doubleSpinBox_Nasion->hide();
        m_pUi->m_comboBox_Nasion->hide();

        m_pEEGoSports->m_bUseTrackedCardinalMode = true;
        m_pEEGoSports->m_bUseElectrodeShiftMode = false;
    } else if(text == "Use electrode shift") {
        m_pUi->m_label_cardinal->hide();
        m_pUi->m_lineEdit_cardinalFile->hide();
        m_pUi->m_pushButton_cardinalFile->hide();

        m_pUi->m_label_LPA->show();
        m_pUi->m_doubleSpinBox_LPA->show();
        m_pUi->m_comboBox_LPA->show();
        m_pUi->m_label_RPA->show();
        m_pUi->m_doubleSpinBox_RPA->show();
        m_pUi->m_comboBox_RPA->show();
        m_pUi->m_label_Nasion->show();
        m_pUi->m_doubleSpinBox_Nasion->show();
        m_pUi->m_comboBox_Nasion->show();

        m_pEEGoSports->m_bUseTrackedCardinalMode = false;
        m_pEEGoSports->m_bUseElectrodeShiftMode = true;
    }

    this->adjustSize();
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::onCardinalComboBoxChanged()
{
    QString sLPA = m_pUi->m_comboBox_LPA->currentText();
    double dLPAShift = m_pUi->m_doubleSpinBox_LPA->value()*1e-2;
    QString sRPA = m_pUi->m_comboBox_RPA->currentText();
    double dRPAShift = m_pUi->m_doubleSpinBox_RPA->value()*1e-2;
    QString sNasion = m_pUi->m_comboBox_Nasion->currentText();
    double dNasionShift = m_pUi->m_doubleSpinBox_Nasion->value()*1e-2;

    emit cardinalPointsChanged(sLPA, dLPAShift, sRPA, dRPAShift, sNasion, dNasionShift);
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::updateCardinalComboBoxes(const QString& sPath)
{
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(sPath, elcChannelNames, elcLocation3D, elcLocation2D, unit)) {
        qCritical() << "Unable to read elc file.";
        return;
    }

    m_pUi->m_comboBox_LPA->clear();
    m_pUi->m_comboBox_RPA->clear();
    m_pUi->m_comboBox_Nasion->clear();

    m_pUi->m_comboBox_LPA->addItems(elcChannelNames);
    m_pUi->m_comboBox_RPA->addItems(elcChannelNames);
    m_pUi->m_comboBox_Nasion->addItems(elcChannelNames);
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::addProject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Project Directory"),
                                                     m_pEEGoSports->m_qStringResourcePath,
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    // Split string to get created or existing target dir with the name of the project
    QStringList list = path.split("/");

    // Add to combo box
    m_pUi->m_qComboBox_ProjectSelection->addItem(list.at(list.size()-1));
    m_pUi->m_qComboBox_ProjectSelection->setCurrentIndex(m_pUi->m_qComboBox_ProjectSelection->count()-1);
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::addSubject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Subject Directory"),
                                                     m_pEEGoSports->m_qStringResourcePath,
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    // Split string to get created or existing target dir with the name of the project
    QStringList list = path.split("/");

    // Add to combo box
    m_pUi->m_qComboBox_SubjectSelection->addItem(list.at(list.size()-1));
    m_pUi->m_qComboBox_SubjectSelection->setCurrentIndex(m_pUi->m_qComboBox_SubjectSelection->count()-1);
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::changeOutputFile()
{
    QString path = QFileDialog::getSaveFileName(
                this,
                "Save to fif file",
                "resources/mne_scan/plugins/eegosports/EEG_data_001_raw.fif",
                 tr("Fif files (*.fif)"));

    if(path==NULL){
        path = m_pUi->m_qLineEdit_FiffRecordFile->text();
    }

    m_pUi->m_qLineEdit_FiffRecordFile->setText(path);
    m_pEEGoSports->m_sOutputFilePath = m_pUi->m_qLineEdit_FiffRecordFile->text();
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                "resources/mne_scan/plugins/eegosports/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL){
        path = m_pUi->m_qLineEdit_EEGCap->text();
    }

    m_pUi->m_qLineEdit_EEGCap->setText(path);
    m_pEEGoSports->m_sElcFilePath = m_pUi->m_qLineEdit_EEGCap->text();
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::changeCardinalFile()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change cardinal file",
                                                "resources/mne_scan/plugins/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL)
        path = m_pUi->m_lineEdit_cardinalFile->text();

    m_pUi->m_lineEdit_cardinalFile->setText(path);
    m_pEEGoSports->m_sCardinalFilePath = m_pUi->m_lineEdit_cardinalFile->text();
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::generateFilePath(int index)
{
    Q_UNUSED(index);

    // Generate file name with timestamp
    QDate date;
    QString fileName = QString ("%1_%2_%3_EEG_001_raw.fif").arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day());

    // Append new file name, subject and project
    QString resourcePath = m_pEEGoSports->m_qStringResourcePath;
    resourcePath.append(m_pUi->m_qComboBox_ProjectSelection->currentText());
    resourcePath.append("/");
    resourcePath.append(m_pUi->m_qComboBox_SubjectSelection->currentText());
    resourcePath.append("/");
    resourcePath.append(fileName);

    m_pUi->m_qLineEdit_FiffRecordFile->setText(resourcePath);
    m_pEEGoSports->m_sOutputFilePath = resourcePath;
}


//=============================================================================================================

void EEGoSportsSetupProjectWidget::changeQLineEdits()
{
    m_pEEGoSports->m_sElcFilePath = m_pUi->m_qLineEdit_EEGCap->text();
    m_pEEGoSports->m_sOutputFilePath = m_pUi->m_qLineEdit_FiffRecordFile->text();
}
