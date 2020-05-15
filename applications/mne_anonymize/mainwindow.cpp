#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <settingscontrollergui.h>
#include <iostream>

#include <QMessageBox>
#include <QCloseEvent>

using namespace MNEANONYMIZE;

MainWindow::MainWindow(MNEANONYMIZE::SettingsControllerGui *c)
: m_bDataModified(true)
, m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);

    //set confirm deletion to true TODO!!!
    m_pUi->dateTimeMeasurementDate->setEnabled(false);
    m_pUi->dateTimeBirthdayDate->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->lineEditHisValue->setEnabled(false);
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
//    std::printf("\n%s\n",f.toUtf8().data());
}

void MainWindow::setCheckBoxBruteMode(bool b)
{
    m_pUi->checkBoxBruteMode->setCheckState(Qt::CheckState(b));
}

void MainWindow::setMeasurementDate(const QString& d)
{
    m_pUi->checkBoxMeasurementDate->setCheckState(Qt::CheckState(true));
    m_pUi->dateTimeMeasurementDate->setDateTime(
            QDateTime(QDate::fromString(d,"ddMMyyyy"),QTime(1,1,0)));
}


void MainWindow::setMeasurementDateOffset(int d)
{
    m_pUi->checkBoxMeasurementDateOffset->setCheckState(Qt::CheckState(true));
    m_pUi->spinBoxMeasurementDateOffset->setValue(d);
}

void MainWindow::setSubjectBirthday(const QString& d)
{
    m_pUi->checkBoxBirthdayDate->setCheckState(Qt::CheckState(true));
    m_pUi->dateTimeBirthdayDate->setDateTime(
                QDateTime(QDate::fromString(d,"ddMMyyyy"),QTime(1,1,0)));
}

void MainWindow::setSubjectBirthdayOffset(int d)
{
    m_pUi->checkBoxBirthdayDateOffset->setCheckState(Qt::CheckState(true));
    m_pUi->spinBoxBirthdayDateOffset->setValue(d);
}

void MainWindow::setSubjectHis(const QString& h)
{
    m_pUi->checkBoxHisValue->setCheckState(Qt::CheckState(true));
    m_pUi->lineEditHisValue->setText(h);
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


void MNEANONYMIZE::MainWindow::on_checkBoxMeasurementDate_stateChanged(int arg1)
{
    std::printf("\n%i\n",arg1);
    if(arg1)
    {
        m_pUi->dateTimeMeasurementDate->setEnabled(true);
        m_pUi->checkBoxMeasurementDateOffset->setCheckState(Qt::CheckState(false));
    } else {
        m_pUi->dateTimeMeasurementDate->setEnabled(false);
    }
}

void MNEANONYMIZE::MainWindow::on_checkBoxMeasurementDateOffset_stateChanged(int arg1)
{
    if(arg1)
    {
        m_pUi->spinBoxMeasurementDateOffset->setEnabled(true);
        m_pUi->checkBoxMeasurementDate->setCheckState(Qt::CheckState(false));
    } else {
        m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    }
}

void MNEANONYMIZE::MainWindow::on_checkBoxBirthdayDate_stateChanged(int arg1)
{
    if(arg1)
    {
        m_pUi->dateTimeBirthdayDate->setEnabled(true);
        m_pUi->checkBoxBirthdayDateOffset->setCheckState(Qt::CheckState(false));
    } else {
        m_pUi->dateTimeBirthdayDate->setEnabled(false);
    }
}

void MNEANONYMIZE::MainWindow::on_checkBoxBirthdayDateOffset_stateChanged(int arg1)
{
    if(arg1)
    {
        m_pUi->spinBoxBirthdayDateOffset->setEnabled(true);
        m_pUi->checkBoxBirthdayDate->setCheckState(Qt::CheckState(false));
    } else {
        m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);
    }
}

void MNEANONYMIZE::MainWindow::on_checkBoxHisValue_clicked(bool checked)
{
    m_pUi->lineEditHisValue->setEnabled(checked);
}

