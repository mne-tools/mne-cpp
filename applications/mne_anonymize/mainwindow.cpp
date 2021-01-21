//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.3
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Wayne Mead, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, John C. Mosher. All rights reserved.
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
 * @brief    Mainwindow class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingscontrollergui.h"
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>
#include <QCloseEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow()
: m_bOptionsVisibility(false)
, m_iDefaultWindowHeight(222)
, m_iDefaultWindowHeightLarge(666)
, m_bShowWraningMsgBoxInWasm(true)
, m_sDefaultWasmInFile("/in.fif")
, m_sDefaultWasmOutFile("/out.fif")
, m_pUi(new Ui::MainWindow)
{
    m_pUi->setupUi(this);
    setDefautlStateUi();
    setDefaultStateExtraInfo();
    setupConnections();
}

//=============================================================================================================

MainWindow::~MainWindow()
{
    delete m_pUi;
}

//=============================================================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(confirmClose())
    {
        event->accept();
    } else {
        event->ignore();
    }
}

//=============================================================================================================

bool MainWindow::confirmClose()
{
    return true;
//    const QMessageBox::StandardButton ret
//            = QMessageBox::warning(this, tr("Application"),
//                                   tr("Are you sure you want to exit?\n"),
//                                   QMessageBox::Yes | QMessageBox::Cancel);
//    switch (ret) {
//    case QMessageBox::Yes:
//        return true;
//    case QMessageBox::Cancel:
//        return false;
//    default:
//        break;
//    }
//    return false;
}

//=============================================================================================================

void MainWindow::setDefautlStateUi()
{
    this->setWindowTitle(qApp->organizationName() + " ~ " + qApp->applicationName() + " ~ " + qApp->applicationVersion());
    #ifdef WASMBUILD
    m_pUi->lineEditInFile->setReadOnly(true);
    m_pUi->labelOutFile->setVisible(false);
    m_pUi->openOutFileWindowButton->setVisible(false);
    m_pUi->lineEditOutFile->setVisible(false);
    m_bOptionsVisibility = true;
    #endif
    resize(m_iDefaultWindowHeightLarge,m_iDefaultWindowHeight);
    setMaximumHeight(m_iDefaultWindowHeight);

    if(m_bOptionsVisibility)
    {
        m_pUi->checkBoxShowOptions->setCheckState(Qt::Checked);
    } else {
        m_pUi->checkBoxShowOptions->setCheckState(Qt::Unchecked);
    }

    m_pUi->tabWidget->setCurrentIndex(0);

    m_pUi->frameOptionsAndExtraInfo->setVisible(m_bOptionsVisibility);
    m_pUi->pushButtonReadData->setVisible(m_bOptionsVisibility);
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);
    m_pUi->spinBoxMeasurementDateOffset->setValue(0);
    m_pUi->spinBoxBirthdayDateOffset->setValue(0);

    m_pUi->comboBoxSubjectSexExtra->addItems(QStringList() << "Unknown" << "Male" << "Female");
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectSexExtra->setEditable(false);

    m_pUi->comboBoxSubjectHandExtra->addItems(QStringList() << "Unknown" << "Right" << "Left");
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectHandExtra->setEditable(false);

    m_pUi->lineEditIdFileVersionExtra->setReadOnly(true);
    m_pUi->lineEditIdMACAddressExtra->setReadOnly(true);
    m_pUi->dateTimeIdMeasurementDateExtra->setReadOnly(true);

    m_pUi->dateTimeFileMeasurementDateExtra->setReadOnly(true);

    m_pUi->lineEditFileExperimenterExtra->setReadOnly(true);
    m_pUi->plainTextFileCommentExtra->setReadOnly(true);

    m_pUi->spinBoxSubjectIDExtra->setReadOnly(true);
    m_pUi->lineEditSubjectFirstNameExtra->setReadOnly(true);
    m_pUi->lineEditSubjectMiddleNameExtra->setReadOnly(true);
    m_pUi->lineEditSubjectLastNameExtra->setReadOnly(true);
    m_pUi->dateEditSubjectBirthdayExtra->setReadOnly(true);
    m_pUi->doubleSpinBoxSubjectWeightExtra->setReadOnly(true);
    m_pUi->doubleSpinBoxSubjectHeightExtra->setReadOnly(true);
    m_pUi->lineEditSubjectCommentExtra->setReadOnly(true);
    m_pUi->lineEditSubjectHisIdExtra->setReadOnly(true);

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->setReadOnly(true);
    m_pUi->lineEditProjectAimExtra->setReadOnly(true);
    m_pUi->lineEditProjectNameExtra->setReadOnly(true);
    m_pUi->lineEditProjectPersonsExtra->setReadOnly(true);
    m_pUi->plainTextEditProjectCommentExtra->setReadOnly(true);

    m_pUi->lineEditMNEEnvironmentWorkingDirExtra->setReadOnly(true);
    m_pUi->lineEditMNEEnvironmentCommandExtra->setReadOnly(true);

    //tooltips
    m_pUi->checkBoxShowOptions->setToolTip("Show the options menu.");
    m_pUi->checkBoxBruteMode->setToolTip("Advanced anonymization. Anonymize also weight, height and some other fields. See Help.");
    m_pUi->labelMeasDate->setToolTip("Specify the measurement date.");
    m_pUi->dateTimeMeasurementDate->setToolTip("Specify the measurement date.");
    m_pUi->checkBoxMeasurementDateOffset->setToolTip("Specify number of days to subtract to the measurement.");
    m_pUi->spinBoxMeasurementDateOffset->setToolTip("Specify number of days to subtract to the measurement.");

    m_pUi->labelSubjectBirthday->setToolTip("Specify the subject’s birthday.");
    m_pUi->checkBoxBirthdayDateOffset->setToolTip("Specify a number of to subtract from the subject's birthday.");
    m_pUi->spinBoxBirthdayDateOffset->setToolTip("Specify a number of to subtract from the subject's birthday.");

    m_pUi->labelSubjectHisId->setToolTip("Specify the Subject’s ID within the Hospital system.");
    m_pUi->lineEditSubjectHisId->setToolTip("Specify the Subject’s ID within the Hospital system.");
    m_pUi->checkBoxMNEEnvironment->setToolTip("Make information related to MNE Toolbox also anonymized.");

    m_pUi->moreInfoButton->setToolTip("See the MNE-CPP project's web for this application.");
    m_pUi->pushButtonAnonymizeFile->setToolTip("Anonymize the input file.");
    m_pUi->labelInFile->setToolTip("File to anonymize");
    m_pUi->lineEditInFile->setToolTip("File to anonymize.");
    m_pUi->openInFileWindowButton->setToolTip("Select a file to anonymize.");
    m_pUi->labelOutFile->setToolTip("Output anonymized file. By default a \"_anonymized\" suffix is added to the name of the input file.");
    m_pUi->lineEditOutFile->setToolTip("Output anonymized file. By default a \"_anonymized\" suffix is added to the name of the input file.");
    m_pUi->openOutFileWindowButton->setToolTip("Select a folder or a file where to save the output anonymized fif file.");

    m_pUi->lineEditIdFileVersionExtra->setToolTip("This value is not modified. It is shown for completion.");
    m_pUi->lineEditIdMACAddressExtra->setToolTip("MAC address of the main acquisition system. Substitution value: 00:00:00:00:00:00:00:00");
    m_pUi->dateTimeIdMeasurementDateExtra->setToolTip("Default substitution value: 01/01/2000 00:01:01");
    m_pUi->dateTimeFileMeasurementDateExtra->setToolTip("Default substitution value: 01/01/2000 00:01:01");
    m_pUi->lineEditFileExperimenterExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->plainTextFileCommentExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->spinBoxSubjectIDExtra->setToolTip("Default substitution value: 0");
    m_pUi->lineEditSubjectFirstNameExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->lineEditSubjectMiddleNameExtra->setToolTip("Default substitution value: mne-cpp");
    m_pUi->lineEditSubjectLastNameExtra->setToolTip("Default substitution value: mne_anonyze");
    m_pUi->dateEditBirthdayDate->setToolTip("Default substitution value: 01/01/2000");
    m_pUi->spinBoxBirthdayDateOffset->setToolTip("Default substitution value: 0");
    m_pUi->lineEditSubjectCommentExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->comboBoxSubjectSexExtra->setToolTip("Default substitution value: unknown");
    m_pUi->comboBoxSubjectHandExtra->setToolTip("Default substitution value: unknown");
    m_pUi->doubleSpinBoxSubjectWeightExtra->setToolTip("Default substitution value: 0.0");
    m_pUi->doubleSpinBoxSubjectHeightExtra->setToolTip("Default substitution value: 0.0");
    m_pUi->lineEditSubjectHisIdExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->spinBoxProjectIDExtra->setToolTip("Default substitution value: 0");
    m_pUi->lineEditProjectNameExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->lineEditProjectAimExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->lineEditProjectPersonsExtra->setToolTip("Default substitution value: mne_anonymize");
    m_pUi->plainTextEditProjectCommentExtra->setToolTip("Default substitution value: mne_anonymize");

    m_pUi->lineEditMNEEnvironmentCommandExtra->setToolTip("Command used to modify the fif file with the MNE toolbox. Default substitution value: mne_anonymize");
    m_pUi->lineEditMNEEnvironmentWorkingDirExtra->setToolTip("Working directory within MNE toolbox. Default substitution value: mne_anonymize");
}

//=============================================================================================================

void MainWindow::setDefaultStateExtraInfo()
{
    m_pUi->lineEditIdFileVersionExtra->clear();
    m_pUi->lineEditIdMACAddressExtra->clear();
    m_pUi->dateTimeIdMeasurementDateExtra->clear();

    m_pUi->dateTimeFileMeasurementDateExtra->clear();

    m_pUi->lineEditFileExperimenterExtra->clear();
    m_pUi->plainTextFileCommentExtra->clear();

    m_pUi->spinBoxSubjectIDExtra->clear();
    m_pUi->lineEditSubjectFirstNameExtra->clear();
    m_pUi->lineEditSubjectMiddleNameExtra->clear();
    m_pUi->lineEditSubjectLastNameExtra->clear();
    m_pUi->dateEditSubjectBirthdayExtra->clear();

    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(0);
    m_pUi->doubleSpinBoxSubjectWeightExtra->clear();
    m_pUi->doubleSpinBoxSubjectHeightExtra->clear();
    m_pUi->lineEditSubjectCommentExtra->clear();
    m_pUi->lineEditSubjectHisIdExtra->clear();

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->clear();
    m_pUi->lineEditProjectAimExtra->clear();
    m_pUi->lineEditProjectNameExtra->clear();
    m_pUi->lineEditProjectPersonsExtra->clear();
    m_pUi->plainTextEditProjectCommentExtra->clear();

    m_pUi->lineEditIdFileVersionExtra->setEnabled(false);
    m_pUi->lineEditIdMACAddressExtra->setEnabled(false);
    m_pUi->dateTimeIdMeasurementDateExtra->setEnabled(false);

    m_pUi->dateTimeFileMeasurementDateExtra->setEnabled(false);

    m_pUi->plainTextFileCommentExtra->setEnabled(false);
    m_pUi->lineEditFileExperimenterExtra->setEnabled(false);

    m_pUi->spinBoxSubjectIDExtra->setEnabled(false);
    m_pUi->lineEditSubjectFirstNameExtra->setEnabled(false);
    m_pUi->lineEditSubjectMiddleNameExtra->setEnabled(false);
    m_pUi->lineEditSubjectLastNameExtra->setEnabled(false);
    m_pUi->dateEditSubjectBirthdayExtra->setEnabled(false);
    m_pUi->comboBoxSubjectSexExtra->setEnabled(false);
    m_pUi->comboBoxSubjectHandExtra->setEnabled(false);
    m_pUi->doubleSpinBoxSubjectWeightExtra->setEnabled(false);
    m_pUi->doubleSpinBoxSubjectHeightExtra->setEnabled(false);
    m_pUi->lineEditSubjectCommentExtra->setEnabled(false);
    m_pUi->lineEditSubjectHisIdExtra->setEnabled(false);

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->setEnabled(false);
    m_pUi->lineEditProjectAimExtra->setEnabled(false);
    m_pUi->lineEditProjectNameExtra->setEnabled(false);
    m_pUi->lineEditProjectPersonsExtra->setEnabled(false);
    m_pUi->plainTextEditProjectCommentExtra->setEnabled(false);

    m_pUi->lineEditMNEEnvironmentWorkingDirExtra->setEnabled(false);
    m_pUi->lineEditMNEEnvironmentCommandExtra->setEnabled(false);
}

//=============================================================================================================

void MainWindow::setupConnections()
{

    //from gui to mainwindow class
    QObject::connect(m_pUi->checkBoxShowOptions,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxShowOptionsChanged);

    QObject::connect(m_pUi->pushButtonReadData,&QPushButton::clicked,
                     this,&MainWindow::readInputDataButtonClicked);

    QObject::connect(m_pUi->pushButtonAnonymizeFile,&QPushButton::clicked,
                     this,&MainWindow::saveOutputFileClicked);

    QObject::connect(m_pUi->moreInfoButton,&QToolButton::clicked,
                     this,&MainWindow::helpButtonClicked);

    QObject::connect(m_pUi->lineEditInFile,&QLineEdit::editingFinished,
                     this,&MainWindow::inFileEditingFinished);
    QObject::connect(m_pUi->lineEditOutFile,&QLineEdit::editingFinished,
                     this,&MainWindow::outFileEditingFinished);

    QObject::connect(m_pUi->openInFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openInFileDialog);
    QObject::connect(m_pUi->openOutFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openOutFileDialog);

    QObject::connect(m_pUi->checkBoxBruteMode,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxBruteModeChanged);

    QObject::connect(m_pUi->checkBoxMeasurementDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxMeasurementDateOffsetStateChanged);
    QObject::connect(m_pUi->spinBoxMeasurementDateOffset,QOverload<int>::of(&QSpinBox::valueChanged),
                     this,&MainWindow::spinBoxMeasurementDateOffsetValueChanged);
    QObject::connect(m_pUi->dateTimeMeasurementDate,&QDateTimeEdit::dateTimeChanged,
                     this,&MainWindow::dateTimeMeasurementDateDateTimeChanged);

    QObject::connect(m_pUi->checkBoxBirthdayDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxBirthdayDateOffsetStateChanged);
    QObject::connect(m_pUi->dateEditBirthdayDate,&QDateEdit::dateChanged,
                     this,&MainWindow::dateEditBirthdayDateDateChanged);

    QObject::connect(m_pUi->spinBoxBirthdayDateOffset,QOverload<int>::of(&QSpinBox::valueChanged),
                     this,&MainWindow::spinBoxBirthdayDateOffsetValueChanged);

    QObject::connect(m_pUi->lineEditSubjectHisId,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditSubjectHisIdEditingFinished);   
}

//=============================================================================================================

void MainWindow::setInFile(const QString &s)
{
    m_fiInFile.setFile(s);
    m_pUi->lineEditInFile->setText(m_fiInFile.absoluteFilePath());
}

//=============================================================================================================

void MainWindow::setOutFile(const QString &s)
{
    m_fiOutFile.setFile(s);
    m_pUi->lineEditOutFile->setText(m_fiOutFile.absoluteFilePath());
}

//=============================================================================================================

void MainWindow::setCheckBoxBruteMode(bool b)
{
    m_pUi->checkBoxBruteMode->setChecked(b);
}

//=============================================================================================================

void MainWindow::setMeasurementDate(const QDateTime& dt)
{
    m_pUi->dateTimeMeasurementDate->setDateTime(dt);
}

//=============================================================================================================

void MainWindow::setCheckBoxMeasurementDateOffset(bool o)
{
    m_pUi->checkBoxMeasurementDateOffset->setChecked(o);
}

//=============================================================================================================

void MainWindow::setMeasurementDateOffset(int d)
{
    m_pUi->spinBoxMeasurementDateOffset->setValue(d);
}

//=============================================================================================================

void MainWindow::setSubjectBirthday(const QDate &d)
{
    m_pUi->dateEditBirthdayDate->setDate(d);
}

//=============================================================================================================

void MainWindow::setCheckBoxSubjectBirthdayOffset(bool b)
{
    m_pUi->checkBoxBirthdayDateOffset->setChecked(b);
}

//=============================================================================================================

void MainWindow::setSubjectBirthdayOffset(int d)
{
    m_pUi->spinBoxBirthdayDateOffset->setValue(d);
}

//=============================================================================================================

void MainWindow::setSubjectHis(const QString& h)
{
    m_pUi->lineEditSubjectHisId->setText(h);
}

//=============================================================================================================

void MainWindow::setLineEditIdFileVersion(double v)
{
    m_pUi->lineEditIdFileVersionExtra->setEnabled(true);
    m_pUi->lineEditIdFileVersionExtra->setText(QString::number(v));
}

//=============================================================================================================

void MainWindow::setLineEditIdMeasurementDate(const QDateTime& d)
{
    m_pUi->dateTimeIdMeasurementDateExtra->setEnabled(true);
    m_pUi->dateTimeIdMeasurementDateExtra->setDateTime(d);
}

//=============================================================================================================

void MainWindow::setLineEditIdMacAddress(const QString& mac)
{
    m_pUi->lineEditIdMACAddressExtra->setEnabled(true);
    m_pUi->lineEditIdMACAddressExtra->setText(mac);
}

//=============================================================================================================

void MainWindow::setLineEditFileMeasurementDate(const QDateTime& d)
{
    m_pUi->dateTimeFileMeasurementDateExtra->setEnabled(true);
    m_pUi->dateTimeFileMeasurementDateExtra->setDateTime(d);
}

//=============================================================================================================

void MainWindow::setLineEditFileComment(const QString& c)
{
    m_pUi->plainTextFileCommentExtra->setEnabled(true);
    m_pUi->plainTextFileCommentExtra->setPlainText(c);
}

//=============================================================================================================

void MainWindow::setLineEditFileExperimenter(const QString& e)
{
    m_pUi->lineEditFileExperimenterExtra->setEnabled(true);
    m_pUi->lineEditFileExperimenterExtra->setText(e);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectId(int i)
{
    m_pUi->spinBoxSubjectIDExtra->setEnabled(true);
    m_pUi->spinBoxSubjectIDExtra->setValue(i);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectFirstName(const QString& fn)
{
    m_pUi->lineEditSubjectFirstNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectFirstNameExtra->setText(fn);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectMiddleName(const QString& mn)
{
    m_pUi->lineEditSubjectMiddleNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectMiddleNameExtra->setText(mn);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectLastName(const QString& ln)
{
    m_pUi->lineEditSubjectLastNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectLastNameExtra->setText(ln);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectBirthday(QDate b)
{
    m_pUi->dateEditBirthdayDate->setEnabled(true);
    m_pUi->dateEditBirthdayDate->setDate(b);
}

//=============================================================================================================

void MainWindow::setComboBoxSubjectSex(int s)
{
    m_pUi->comboBoxSubjectSexExtra->setEnabled(true);
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(s);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectHand(int h)
{
    m_pUi->comboBoxSubjectHandExtra->setEnabled(true);
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(h);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectWeight(float w)
{
    m_pUi->doubleSpinBoxSubjectWeightExtra->setEnabled(true);
    double wd(static_cast<double>(w));
    m_pUi->doubleSpinBoxSubjectWeightExtra->setValue(wd);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectHeight(float h)
{
    m_pUi->doubleSpinBoxSubjectHeightExtra->setEnabled(true);
    double hd(static_cast<double>(h));
    m_pUi->doubleSpinBoxSubjectHeightExtra->setValue(hd);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectComment(const QString& c)
{
    m_pUi->lineEditSubjectCommentExtra->setEnabled(true);
    m_pUi->lineEditSubjectCommentExtra->setText(c);
}

//=============================================================================================================

void MainWindow::setLineEditSubjectHisId(const QString& his)
{
    m_pUi->lineEditSubjectHisIdExtra->setEnabled(true);
    m_pUi->lineEditSubjectHisIdExtra->setText(his);
}

//=============================================================================================================

void MainWindow::setLineEditProjectId(int id)
{
    m_pUi->spinBoxProjectIDExtra->setEnabled(true);
    m_pUi->spinBoxProjectIDExtra->setValue(id);
}

//=============================================================================================================

void MainWindow::setLineEditProjectName(const QString& p)
{
    m_pUi->lineEditProjectNameExtra->setEnabled(true);
    m_pUi->lineEditProjectNameExtra->setText(p);
}

//=============================================================================================================

void MainWindow::setLineEditProjectAim(const QString& p)
{
    m_pUi->lineEditProjectAimExtra->setEnabled(true);
    m_pUi->lineEditProjectAimExtra->setText(p);
}

//=============================================================================================================

void MainWindow::setLineEditProjectPersons(const QString& p)
{
    m_pUi->lineEditProjectPersonsExtra->setEnabled(true);
    m_pUi->lineEditProjectPersonsExtra->setText(p);
}

//=============================================================================================================

void MainWindow::setLineEditProjectComment(const QString& c)
{
    m_pUi->plainTextEditProjectCommentExtra->setEnabled(true);
    m_pUi->plainTextEditProjectCommentExtra->setPlainText(c);
}

//=============================================================================================================

void MainWindow::setLabelMriDataFoundVisible(bool b)
{
    m_pUi->labelSubjectMriDataFoundExtra->setVisible(b);
}

//=============================================================================================================

void MainWindow::setLineEditMNEWorkingDir(const QString& s)
{
    m_pUi->lineEditMNEEnvironmentWorkingDirExtra->setEnabled(true);
    m_pUi->lineEditMNEEnvironmentWorkingDirExtra->setText(s);
}

//=============================================================================================================

void MainWindow::setLineEditMNECommand(const QString& s)
{
    m_pUi->lineEditMNEEnvironmentCommandExtra->setEnabled(true);
    m_pUi->lineEditMNEEnvironmentCommandExtra->setText(s);
}

//=============================================================================================================

void MainWindow::openInFileDialog()
{
    #ifdef WASMBUILD
    m_pUi->pushButtonReadData->setDisabled(true);

    auto fileContentReady = [&](const QString &filePath, const QByteArray &fileContent) {
        if(!filePath.isNull()) {

            QFile fileIn(m_sDefaultWasmInFile);
            fileIn.open(QIODevice::WriteOnly);
            fileIn.write(fileContent);
            fileIn.close();

            m_fiInFile.setFile(m_sDefaultWasmInFile);
            m_pUi->lineEditInFile->setText(filePath);
            emit fileInChanged(m_fiInFile.absoluteFilePath());

            m_fiOutFile.setFile(m_sDefaultWasmOutFile);
            emit fileOutChanged(m_fiOutFile.absoluteFilePath());

            m_pUi->pushButtonReadData->setDisabled(false);
        }
    };

    if(m_bShowWraningMsgBoxInWasm)
    {
        m_bShowWraningMsgBoxInWasm = false;
        m_pUi->labelInFile->setText(m_pUi->labelInFile->text() + " [Max. 500MB]. ");
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Warning on FIFF maximum size.");
        msgBox.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        msgBox.setText("Warning. Development version.\nBrowser-based MNE Anonymize is compatible with FIFF files up to 500MB.\n"
                       "For bigger files, download MNE-CPP suite from <a href='https://mne-cpp.github.io/pages/install/binaries.html'>here</a>.\n"
                       "                Sincerely, the development team @ MNE-CPP.");
        msgBox.exec();
    }

    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)",  fileContentReady);
    #else
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Fiff file (*.fif *.fiff)"));
    dialog.setDirectory(QDir::currentPath());
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        m_fiInFile.setFile(fileNames.at(0));
        m_pUi->lineEditInFile->setText(m_fiInFile.absoluteFilePath());
        emit fileInChanged(m_fiInFile.absoluteFilePath());
    }
    #endif
}

//=============================================================================================================

void MainWindow::openOutFileDialog()
{
    QFileInfo inFile(m_pUi->lineEditInFile->text());
    QDir inDir;
    if(inFile.isFile())
    {
        inDir.setPath(inFile.absolutePath());
    } else {
        inDir.setPath(QDir::currentPath());
    }
    QFileDialog dialog(this);
    dialog.setDirectory(inDir);
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        m_fiOutFile.setFile(fileNames.at(0));
        m_pUi->lineEditOutFile->setText(m_fiOutFile.absoluteFilePath());
        emit fileOutChanged(m_fiOutFile.absoluteFilePath());
    }
}

//=============================================================================================================

void MainWindow::outputFileReady()
{
    #ifdef WASMBUILD
    //we need to give space to the output file to be copied.
    QFile::remove(m_fiInFile.absoluteFilePath());

    QFile fileOut(m_fiOutFile.absoluteFilePath());
//    qDebug() << "file out:" << m_fiOutFile.absoluteFilePath();
//    qDebug() << "fileout size: " << QString::number(fileOut.size());

    fileOut.open(QIODevice::ReadWrite);
    QByteArray  outFileContent;

    outFileContent = fileOut.readAll();

//    qDebug() << "fileout size (after read): " << QString::number(fileOut.size());

    QFileInfo fiInFile(m_pUi->lineEditInFile->text());
    QString fileOutName(fiInFile.baseName() + "_anonymized." + fiInFile.completeSuffix());

    QFileDialog::saveFileContent(outFileContent,fileOutName);

    //we reset the input file textbox (and the according member var) because we want to
    //make explicit to the user that the input file has been deleted. Reading or anonymization
    //will not work.
    setInFile("");
    #else
    statusMsg("Your file is ready!");
    #endif
}

//=============================================================================================================

void MainWindow::helpButtonClicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(qApp->organizationName() + " ~ " + qApp->applicationName() + " ~ " + qApp->applicationVersion());
    msgBox.setTextFormat(Qt::RichText);   //this is what makes the links clickable
    msgBox.setText("<p>June 2020<br>mne_anonymize <br>version: " + qApp->applicationVersion() + "</p>"
                   "<p>This applcation allows to anonymize and deidentify FIFF files.</p>"
                   "<p>For more information please visit "
                   "<a href='https://mne-cpp.github.io/pages/documentation/anonymize.html'>mne_anonymize's documentation web</a>.</p>"
                   "<p style=""text-align:right"">Sincerely, the development team @ MNE-CPP.</p>");
    msgBox.exec();
}

//=============================================================================================================

void MainWindow::inFileEditingFinished()
{
    m_fiInFile.setFile(m_pUi->lineEditInFile->text());
    emit fileInChanged(m_fiInFile.absoluteFilePath());
}

//=============================================================================================================

void MainWindow::outFileEditingFinished()
{
    m_fiOutFile.setFile(m_pUi->lineEditOutFile->text());
    emit fileOutChanged(m_fiOutFile.absoluteFilePath());
}

//=============================================================================================================

void MainWindow::checkBoxBruteModeChanged()
{
    bool state(m_pUi->checkBoxBruteMode->isChecked());
    emit bruteModeChanged(state);
    if(state)
    {
        statusMsg("Brute mode selected",2000);
    } else {
        statusMsg("Brute mode deselected",2000);
    }
}

//=============================================================================================================

void MainWindow::checkBoxMeasurementDateOffsetStateChanged(int arg)
{
    Q_UNUSED(arg)
    bool state(m_pUi->checkBoxMeasurementDateOffset->isChecked());
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(state);
    emit useMeasurementOffset(state);
    m_pUi->dateTimeMeasurementDate->setEnabled(!state);
    if(state)
    {
        statusMsg("Specify a measurement date offset.",2000);
    } else {
        statusMsg("Specify the measurement date.",2000);
    }
}

//=============================================================================================================

void MainWindow::checkBoxBirthdayDateOffsetStateChanged(int arg)
{
    Q_UNUSED(arg)
    bool state(m_pUi->checkBoxBirthdayDateOffset->isChecked());
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(state);
    emit useBirthdayOffset(state);
    m_pUi->dateEditBirthdayDate->setEnabled(!state);
    if(state)
    {
        statusMsg("Specify a subject's birthday offset.",2000);
    } else {
        statusMsg("Specify the subject's birthday.",2000);
    }
}

//=============================================================================================================

void MainWindow::dateTimeMeasurementDateDateTimeChanged(const QDateTime& dateTime)
{
    emit measurementDateChanged(dateTime);
}

//=============================================================================================================

void MainWindow::spinBoxMeasurementDateOffsetValueChanged(int offset)
{
    emit measurementDateOffsetChanged(offset);
}

//=============================================================================================================

void MainWindow::dateEditBirthdayDateDateChanged(const QDate& date)
{
    emit birthdayDateChanged(date);
}

//=============================================================================================================

void MainWindow::spinBoxBirthdayDateOffsetValueChanged(int offset)
{
    emit birthdayOffsetChanged(offset);
}

//=============================================================================================================

void MainWindow::lineEditSubjectHisIdEditingFinished()
{
    emit subjectHisIdChanged(m_pUi->lineEditSubjectHisId->text());
}

//=============================================================================================================

void MainWindow::winPopup(const QString& s)
{
    QMessageBox msgBox;
    msgBox.setText(s);
    msgBox.exec();
    return;
}

//=============================================================================================================

void MainWindow::statusMsg(const QString& s,int to)
{
    m_pUi->statusbar->clearMessage();
    m_pUi->statusbar->showMessage(s,to);
}

//=============================================================================================================

void MainWindow::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
#ifndef WASMBUILD
    checkSmallGui();
#endif
//    statusMsg("width: " + QString::number(m_pUi->centralwidget->width()));
}

//=============================================================================================================

void MainWindow::checkSmallGui()
{
    int criticalWidth(350);

    if( !m_pUi->lineEditInFile->text().isEmpty())
    {
        if(m_pUi->lineEditInFile->width() < criticalWidth)
        {
            m_pUi->lineEditInFile->setText("(...)/" + m_fiInFile.fileName());
        } else {
            m_pUi->lineEditInFile->setText(m_fiInFile.absoluteFilePath());
        }
    }

    if( !m_pUi->lineEditOutFile->text().isEmpty())
    {
        if(m_pUi->lineEditOutFile->width() < criticalWidth)
        {
            m_pUi->lineEditOutFile->setText("(...)/" + m_fiOutFile.fileName());
        } else {
            m_pUi->lineEditOutFile->setText(m_fiOutFile.absoluteFilePath());
        }
    }
}

//=============================================================================================================

void MainWindow::checkBoxShowOptionsChanged()
{
    m_bOptionsVisibility = m_pUi->checkBoxShowOptions->isChecked();
    if(m_bOptionsVisibility)
    {
        setMaximumHeight(10*m_iDefaultWindowHeight);
        if(height() < m_iDefaultWindowHeightLarge)
        {
            resize(width(),m_iDefaultWindowHeightLarge);
        }
    } else {
        if(height() > m_iDefaultWindowHeight)
        {
            resize(width(),m_iDefaultWindowHeight);
        }
        setMaximumHeight(m_iDefaultWindowHeight);
    }

    m_pUi->frameOptionsAndExtraInfo->setVisible(m_bOptionsVisibility);
    m_pUi->pushButtonReadData->setVisible(m_bOptionsVisibility);
}

//=============================================================================================================

void MainWindow::repaintTabWdiget()
{
    m_pUi->tabWidget->repaint();
}
