#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <settingscontrollergui.h>
#include <iostream>

#include <QMessageBox>
#include <QCloseEvent>
#include <QUrl>
#include <QDesktopServices>

using namespace MNEANONYMIZE;

MainWindow::MainWindow(MNEANONYMIZE::SettingsControllerGui *c)
: m_bDataModified(true)
, m_bHideEachField(false)
, m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);

////    //maybe use getters from fiff_anonymizer obj.?
//    m_pUi->checkBoxDeleteInputFile->setEnabled(false);
//    m_pUi->checkBoxAvoidDeleteConfirmation->setEnabled(true);

//    m_pUi->dateTimeMeasurementDate->setEnabled(true);
//    m_pUi->checkBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);

//    m_pUi->dateTimeBirthdayDate->setEnabled(true);
//    m_pUi->checkBoxBirthdayDateOffset->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);

//    m_pUi->lineEditSubjectHisId->setEnabled(true);
    m_pUi->frameFile->setHidden(m_bHideEachField);
    m_pUi->frameSubject->setHidden(m_bHideEachField);
    m_pUi->frameProject->setHidden(m_bHideEachField);
    m_pUi->lineEditFileVersionExtra->setEnabled(false);
    m_pUi->lineEditMACAddressExtra->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete m_pUi;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(confirmClose())
    {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::confirmClose()
{
    if (!m_bDataModified)
    {
        return true;
    }
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return true; //save();
    case QMessageBox::Cancel:
        return false;
    case QMessageBox::Discard:
        return true;
    default:
        break;
    }
    return true;
}

void MainWindow::setController(MNEANONYMIZE::SettingsControllerGui *c)
{
    m_pController = c;
}

MNEANONYMIZE::SettingsControllerGui* MainWindow::getController() const
{
    return m_pController;
}

//todo
// set the delete input file
// set the avoid delete input file confirmation.

//generate signals for each of these setters
//emit whenever there is a change.
//have the controller not configuring the actual anonymizer state directly.
//but configure the signals=>slots and then just actively configure the setteers
//
//generate a section which is hidden initially named> See and edit all the information
// then create a reader in the controller gui which will populate the info.
// then create another column with the actual output to be writtern to the output file
// syncrhonize the initial controls with these ones.
//
// generate a link between Anonymize file and the actual anonymize member in anonymizer
// solve the issues with input output file the same etc...
// i think that would be a first approach.

void MainWindow::setLineEditInFile(const QString &f)
{
    m_pUi->lineEditInFile->setText(f);
//    std::printf("\n%s\n",f.toUtf8().data());
}

void MainWindow::setLineEditOutFile(const QString &f)
{
    m_pUi->lineEditOutFile->setText(f);
    // std::printf("\n%s\n",f.toUtf8().data());
}

void MainWindow::setCheckBoxBruteMode(bool b)
{
    m_pUi->checkBoxBruteMode->setChecked(b);
}

void MainWindow::setCheckBoxDeleteInputFileAfter(bool b)
{
    m_pUi->checkBoxDeleteInputFile->setChecked(b);
}

void MainWindow::setCheckBoxAvoidDeleteConfirmation(bool b)
{
    m_pUi->checkBoxAvoidDeleteConfirmation->setChecked(b);
}

void MainWindow::setMeasurementDate(const QString& d)
{
    m_pUi->dateTimeMeasurementDate->setDateTime(
            QDateTime(QDate::fromString(d,"ddMMyyyy"),QTime(1,1,0)));
}

void MainWindow::setMeasurementDate(const QDateTime& dt)
{
    m_pUi->dateTimeMeasurementDate->setDateTime(dt);
}

void MainWindow::setCheckBoxMeasurementDateOffset(bool o)
{
    m_pUi->checkBoxMeasurementDateOffset->setCheckState(Qt::CheckState(o));
}

void MainWindow::setMeasurementDateOffset(int d)
{
    m_pUi->spinBoxMeasurementDateOffset->setValue(d);
}

void MainWindow::setSubjectBirthday(const QString& d)
{
    m_pUi->dateTimeBirthdayDate->setDateTime(
                QDateTime(QDate::fromString(d,"ddMMyyyy"),QTime(1,1,0)));
}

void MainWindow::setCheckBoxSubjectBirthdayOffset(bool b)
{
    m_pUi->checkBoxBirthdayDateOffset->setChecked(b);
}

void MainWindow::setSubjectBirthdayOffset(int d)
{
    m_pUi->spinBoxBirthdayDateOffset->setValue(d);
}

void MainWindow::setSubjectHis(const QString& h)
{
    m_pUi->lineEditSubjectHisId->setText(h);
}

// /////////////////////////////// slots

void MNEANONYMIZE::MainWindow::on_lineEditInFile_editingFinished()
{
    //std::printf("\n%s\n",m_pUi->lineEditInFile->text().toUtf8().data());
    emit fileInChanged(m_pUi->lineEditInFile->text());
}

void MNEANONYMIZE::MainWindow::on_lineEditOutFile_editingFinished()
{
    emit fileOutChanged(m_pUi->lineEditOutFile->text());
}

void MNEANONYMIZE::MainWindow::on_checkBoxMeasurementDateOffset_stateChanged(int state)
{
     m_pUi->spinBoxMeasurementDateOffset->setEnabled(state);
     emit useMeasurementOffset(!!state);
     m_pUi->dateTimeMeasurementDate->setEnabled(!state);
}

void MNEANONYMIZE::MainWindow::on_checkBoxBirthdayDateOffset_stateChanged(int state)
{
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(state);
    emit useBirthdayOffset(!!state);
    m_pUi->dateTimeBirthdayDate->setEnabled(!state);
}

void MNEANONYMIZE::MainWindow::on_dateTimeMeasurementDate_dateTimeChanged(const QDateTime &dateTime)
{
    emit measurementDateChanged(dateTime);
}

void MNEANONYMIZE::MainWindow::on_spinBoxMeasurementDateOffset_valueChanged(int offset)
{
    emit measurementDateOffsetChanged(offset);
}

void MNEANONYMIZE::MainWindow::on_dateTimeBirthdayDate_dateTimeChanged(const QDateTime &dateTime)
{
    emit birthdayDateChanged(dateTime);
}

void MNEANONYMIZE::MainWindow::on_spinBoxBirthdayDateOffset_valueChanged(int offset)
{
    emit birthdayOffsetChanged(offset);
}

void MNEANONYMIZE::MainWindow::on_lineEditSubjectHisId_editingFinished()
{
    emit subjectHisIdChanged(m_pUi->lineEditSubjectHisId->text());
}

void MNEANONYMIZE::MainWindow::on_toolButton_clicked()
{
    m_bHideEachField = !m_bHideEachField;
    m_pUi->frameFile->setHidden(m_bHideEachField);
    m_pUi->frameSubject->setHidden(m_bHideEachField);
    m_pUi->frameProject->setHidden(m_bHideEachField);
}

void MNEANONYMIZE::MainWindow::on_buttonSaveFile_helpRequested()
{
    QDesktopServices::openUrl( QUrl("https://mne-cpp.github.io/pages/learn/mneanonymize.html",
                               QUrl::TolerantMode) );
}
