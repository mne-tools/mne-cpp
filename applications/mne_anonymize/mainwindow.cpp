#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <settingscontrollergui.h>
#include <iostream>

#include <QMessageBox>
#include <QCloseEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QDateTime>
#include <QFileDialog>

using namespace MNEANONYMIZE;

MainWindow::MainWindow(MNEANONYMIZE::SettingsControllerGui *c)
:
//  m_bIdFileVersionFound(false)
//, m_bIdMeasurementDateFound(false)
//, m_bIdMacAddressFound(false)
//, m_bFileMeasurementDateFound(false)
//, m_bFileExperimenterFound(false)
//, m_bFileCommentFound(false)
//, m_bSubjectIdFound(false)
//, m_bSubjectFirstNameFound(false)
//, m_bSubjectMiddleNameFound(false)
//, m_bSubjectLastNameFound(false)
//, m_bSubjectBirthdayFound(false)
//, m_bSubjectSexFound(false)
//, m_bSubjectHandFound(false)
//, m_bSubjectWeightFound(false)
//, m_bSubjectHeightFound(false)
//, m_bSubjectCommentFound(false)
//, m_bSubjectHisIdFound(false)
//, m_bProjectIdFound(false)
//, m_bProjectAimFound(false)
//, m_bProjectNameFound(false)
//, m_bProjectPersonsFound(false)
//, m_bProjectCommentFound(false)
//  m_bHideInfoFields(true)
 m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);
    setDefautlStateUi();
    setupConnections();
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
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("Application"),
                                   tr("Are you sure you want to exit?\n"),
                                   QMessageBox::Yes | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Yes:
        return true;
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return false;
}

void MainWindow::clearInfo()
{
    m_pUi->spinBoxMeasurementDateOffset->setValue(0);
    m_pUi->spinBoxBirthdayDateOffset->setValue(0);

    m_pUi->lineEditFileVersionExtra->clear();
    m_pUi->lineEditMACAddressExtra->clear();

    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(0);

    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(0);

    m_pUi->lineEditFileVersionExtra->clear();
    m_pUi->dateTimeIdMeasurementDateExtra->clear();
    m_pUi->lineEditMACAddressExtra->clear();
    m_pUi->dateTimeFileMeasurementDateExtra->clear();

    m_pUi->lineEditExperimenterExtra->clear();
    m_pUi->plainTextFileCommentExtra->clear();

    m_pUi->spinBoxSubjectIDExtra->clear();
    m_pUi->lineEditSubjectFirstNameExtra->clear();
    m_pUi->lineEditSubjectMiddleNameExtra->clear();
    m_pUi->lineEditSubjectLastNameExtra->clear();
    m_pUi->dateEditSubjectBirthdayExtra->clear();

    m_pUi->doubleSpinBoxSubjectWeightExtra->clear();
    m_pUi->doubleSpinBoxSubjectHeightExtra->clear();
    m_pUi->plainTextEditSubjectCommentExtra->clear();
    m_pUi->lineEditSubjectHisIdExtra->clear();

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->clear();
    m_pUi->lineEditProjectAimExtra->clear();
    m_pUi->lineEditProjectNameExtra->clear();
    m_pUi->lineEditProjectPersonsExtra->clear();
    m_pUi->plainTextEditProjectCommentExtra->clear();
}

void MainWindow::setDefautlStateUi()
{
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);

//    m_pUi->frameFile->setHidden(m_bHideInfoFields);
//    m_pUi->frameSubject->setHidden(m_bHideInfoFields);
//    m_pUi->frameProject->setHidden(m_bHideInfoFields);

    m_pUi->lineEditFileVersionExtra->setEnabled(false);
    m_pUi->lineEditMACAddressExtra->setEnabled(false);

    m_pUi->comboBoxSubjectSexExtra->addItems(QStringList() << "Unknown" << "Male" << "Female");
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectSexExtra->setEditable(false);

    m_pUi->comboBoxSubjectHandExtra->addItems(QStringList() << "Unknown" << "Right" << "Left");
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectHandExtra->setEditable(false);

    m_pUi->lineEditFileVersionExtra->setEnabled(false);
    m_pUi->dateTimeIdMeasurementDateExtra->setEnabled(false);
    m_pUi->lineEditMACAddressExtra->setEnabled(false);
    m_pUi->dateTimeFileMeasurementDateExtra->setEnabled(false);

    m_pUi->lineEditExperimenterExtra->setEnabled(false);
    m_pUi->plainTextFileCommentExtra->setEnabled(false);

    m_pUi->spinBoxSubjectIDExtra->setEnabled(false);
    m_pUi->lineEditSubjectFirstNameExtra->setEnabled(false);
    m_pUi->lineEditSubjectMiddleNameExtra->setEnabled(false);
    m_pUi->lineEditSubjectLastNameExtra->setEnabled(false);
    m_pUi->dateEditSubjectBirthdayExtra->setEnabled(false);
    m_pUi->comboBoxSubjectSexExtra->setEnabled(false);
    m_pUi->comboBoxSubjectHandExtra->setEnabled(false);
    m_pUi->doubleSpinBoxSubjectWeightExtra->setEnabled(false);
    m_pUi->doubleSpinBoxSubjectHeightExtra->setEnabled(false);
    m_pUi->plainTextEditSubjectCommentExtra->setEnabled(false);
    m_pUi->lineEditSubjectHisIdExtra->setEnabled(false);

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->setEnabled(false);
    m_pUi->lineEditProjectAimExtra->setEnabled(false);
    m_pUi->lineEditProjectNameExtra->setEnabled(false);
    m_pUi->lineEditProjectPersonsExtra->setEnabled(false);
    m_pUi->plainTextEditProjectCommentExtra->setEnabled(false);

}

void MainWindow::setupConnections()
{

    QObject::connect(m_pUi->buttonMenu,&QDialogButtonBox::accepted,
                     m_pController,&SettingsControllerGui::executeAnonymizer);

    QObject::connect(m_pUi->buttonMenu,&QDialogButtonBox::helpRequested,
                     this,&MainWindow::helpButtonClicked);

    QObject::connect(m_pUi->lineEditInFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditInFileEditingFinished);
    QObject::connect(m_pUi->lineEditOutFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditOutFileEditingFinished);

    QObject::connect(m_pUi->checkBoxBruteMode,&QCheckBox::stateChanged,
                     this,&MainWindow::checkboxBruteModeChanged);

    QObject::connect(m_pUi->openInFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openInFileDialog);
    QObject::connect(m_pUi->openOutFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openOutFileDialog);

    QObject::connect(m_pUi->checkBoxMeasurementDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxMeasurementDateOffsetStateChanged);
    QObject::connect(m_pUi->checkBoxBirthdayDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxBirthdayDateOffsetStateChanged);
    QObject::connect(m_pUi->dateTimeMeasurementDate,&QDateTimeEdit::dateTimeChanged,
                     this,&MainWindow::dateTimeMeasurementDateDateTimeChanged);
    QObject::connect(m_pUi->spinBoxMeasurementDateOffset,QOverload<int>::of(&QSpinBox::valueChanged),
                     this,&MainWindow::spinBoxMeasurementDateOffsetValueChanged);
    QObject::connect(m_pUi->dateTimeBirthdayDate,&QDateTimeEdit::dateTimeChanged,
                     this,&MainWindow::dateTimeBirthdayDateDateTimeChanged);
    QObject::connect(m_pUi->spinBoxBirthdayDateOffset,QOverload<int>::of(&QSpinBox::valueChanged),
                     this,&MainWindow::spinBoxBirthdayDateOffsetValueChanged);
    QObject::connect(m_pUi->lineEditSubjectHisId,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditSubjectHisIdEditingFinished);
}


void MainWindow::setLineEditInFile(const QString &s)
{
    m_pUi->lineEditInFile->setText(s);
}

void MainWindow::setLineEditOutFile(const QString &s)
{
    m_pUi->lineEditOutFile->setText(s);
}

void MainWindow::setCheckBoxBruteMode(bool b)
{
    m_pUi->checkBoxBruteMode->setChecked(b);
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


//public slots for extra information

void MainWindow::setLineEditIdFileVersion(double v)
{
//    m_pUi->lineEditFileVersionExtra->setVisible(true);
//    m_pUi->labelFileVersionExtra->setVisible(true);
//    m_bIdFileVersionFound = true;
    m_pUi->lineEditFileVersionExtra->setText(QString::number(v));
}

void MainWindow::setLineEditIdMeasurementDate(QDateTime d)
{
//    m_pUi->labelIdMeasurementDateExtra->setVisible(true);
//    m_pUi->dateTimeIdMeasurementDateExtra->setVisible(true);
//    m_bIdMeasurementDateFound = true;
    m_pUi->dateTimeIdMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setLineEditIdMacAddress(QString mac)
{
//    m_pUi->labelMACAddressExtra->setVisible(true);
//    m_pUi->lineEditMACAddressExtra->setVisible(true);
//    m_bIdMacAddressFound = true;
    m_pUi->lineEditMACAddressExtra->setText(mac);
}

void MainWindow::setLineEditFileMeasurementDate(QDateTime d)
{
//    m_pUi->labelMeasDate->setVisible(true);
//    m_pUi->dateTimeFileMeasurementDateExtra->setVisible(true);
//    m_bFileMeasurementDateFound = true;
    m_pUi->dateTimeFileMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setLineEditFileComment(QString c)
{
//    m_pUi->labelFileCommentExtra->setVisible(true);
//    m_pUi->plainTextFileCommentExtra->setVisible(true);
//    m_bFileCommentFound = true;
    m_pUi->plainTextFileCommentExtra->setPlainText(c);
}

void MainWindow::setLineEditFileExperimenter(QString e)
{
//    m_pUi->labelExperimenterExtra->setVisible(true);
//    m_pUi->lineEditExperimenterExtra->setVisible(true);
//    m_bFileExperimenterFound = true;
    m_pUi->lineEditExperimenterExtra->setText(e);
}

void MainWindow::setLineEditSubjectId(int i)
{
//    m_pUi->labelSubjectIDExtra->setVisible(true);
//    m_pUi->spinBoxSubjectIDExtra->setVisible(true);
//    m_bSubjectIdFound = true;
    m_pUi->spinBoxSubjectIDExtra->setValue(i);
}

void MainWindow::setLineEditSubjectFirstName(QString fn)
{
//    m_pUi->labelSubjectFirstNameExtra->setVisible(true);
//    m_pUi->lineEditSubjectFirstNameExtra->setVisible(true);
//    m_bSubjectFirstNameFound = true;
    m_pUi->lineEditSubjectFirstNameExtra->setText(fn);
}

void MainWindow::setLineEditSubjectMiddleName(QString mn)
{
//    m_bSubjectMiddleNameFound = true;
    m_pUi->lineEditSubjectMiddleNameExtra->setText(mn);
}

void MainWindow::setLineEditSubjectLastName(QString ln)
{
//    m_bSubjectLastNameFound = true;
    m_pUi->lineEditSubjectLastNameExtra->setText(ln);
}

void MainWindow::setLineEditSubjectBirthday(QDateTime b)
{
//    m_bSubjectBirthdayFound = true;
    m_pUi->dateTimeBirthdayDate->setDateTime(b);
}

void MainWindow::setLineEditSubjectSex(int s)
{
//    m_bSubjectSexFound = true;
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(s);
}

void MainWindow::setLineEditSubjectHand(int h)
{
//    m_bSubjectHandFound = true;
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(h);
}

void MainWindow::setLineEditSubjectWeight(float w)
{
//    m_bSubjectWeightFound = true;
    double wd(static_cast<double>(w));
    m_pUi->doubleSpinBoxSubjectWeightExtra->setValue(wd);
}

void MainWindow::setLineEditSubjectHeight(float h)
{
//    m_bSubjectHeightFound = true;
    double hd(static_cast<double>(h));
    m_pUi->doubleSpinBoxSubjectHeightExtra->setValue(hd);
}

void MainWindow::setLineEditSubjectComment(QString c)
{
//    m_bSubjectCommentFound = true;
    m_pUi->plainTextEditSubjectCommentExtra->setPlainText(c);
}

void MainWindow::setLineEditSubjectHisId(QString his)
{
//    m_bSubjectHisIdFound = true;
    m_pUi->lineEditSubjectHisIdExtra->setText(his);
}

void MainWindow::setLineEditProjectId(int id)
{
//    m_bProjectIdFound = true;
    m_pUi->spinBoxProjectIDExtra->setValue(id);
}

void MainWindow::setLineEditProjectName(QString p)
{
//    m_bProjectNameFound = true;
    m_pUi->lineEditProjectNameExtra->setText(p);
}

void MainWindow::setLineEditProjectAim(QString p)
{
//    m_bProjectAimFound = true;
    m_pUi->lineEditProjectAimExtra->setText(p);
}

void MainWindow::setLineEditProjectPersons(QString p)
{
//    m_bProjectPersonsFound = true;
    m_pUi->lineEditProjectPersonsExtra->setText(p);
}

void MainWindow::setLineEditProjectComment(QString c)
{
//    m_bProjectCommentFound = true;
    m_pUi->plainTextEditProjectCommentExtra->setPlainText(c);
}

void MainWindow::setLabelMriDataFoundVisible(bool b)
{
    m_pUi->labelSubjectMriDataFoundExtra->setVisible(b);
}

void MainWindow::openInFileDialog()
{
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Fif files (*.fif)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        setLineEditInFile(fileNames.at(0));
        lineEditInFileEditingFinished();
    }
}

void MainWindow::openOutFileDialog()
{
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Fif files (*.fif)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::AnyFile);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        setLineEditOutFile(fileNames.at(0));
        lineEditOutFileEditingFinished();
    }
}

void MainWindow::helpButtonClicked()
{
    QDesktopServices::openUrl( QUrl("https://mne-cpp.github.io/pages/learn/mneanonymize.html",
                               QUrl::TolerantMode) );
}

void MainWindow::lineEditInFileEditingFinished()
{
    QFileInfo inFile(m_pUi->lineEditInFile->text());
    if(!inFile.isFile())
    {
        QMessageBox msgBox;
        msgBox.setText("The input file must be a valid file name.");
        msgBox.exec();
        m_pUi->lineEditInFile->clear();
        return;
    }

    if(QString::compare(inFile.suffix(),QString("fif")) != 0)
    {
        QMessageBox msgBox;
        msgBox.setText("The input file extension must be \".fif\" 0.");
        msgBox.exec();
        m_pUi->lineEditInFile->clear();
        return;
    }

    emit fileInChanged(inFile.absoluteFilePath());
}

void MainWindow::lineEditOutFileEditingFinished()
{
    QFileInfo outFile(m_pUi->lineEditOutFile->text());
    if(outFile.isDir())
    {
        QMessageBox msgBox;
        msgBox.setText("The output file must not be a folder.");
        msgBox.exec();
        m_pUi->lineEditOutFile->clear();
        return;
    }

    if(QString::compare(outFile.suffix(),QString("fif")) != 0)
    {
        QMessageBox msgBox;
        msgBox.setText("The output file extension must be \".fif\" 0.");
        msgBox.exec();
        m_pUi->lineEditOutFile->clear();
        return;
    }
    emit fileOutChanged(outFile.absoluteFilePath());
}


void MainWindow::checkboxBruteModeChanged()
{
    bool state(m_pUi->checkBoxBruteMode->isChecked());
    emit bruteModeChanged(state);
}

void MainWindow::checkBoxMeasurementDateOffsetStateChanged(int state)
{
     m_pUi->spinBoxMeasurementDateOffset->setEnabled(state);
     emit useMeasurementOffset(!!state);
     m_pUi->dateTimeMeasurementDate->setEnabled(!state);
}

void MainWindow::checkBoxBirthdayDateOffsetStateChanged(int state)
{
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(state);
    emit useBirthdayOffset(!!state);
    m_pUi->dateTimeBirthdayDate->setEnabled(!state);
}

void MainWindow::dateTimeMeasurementDateDateTimeChanged(const QDateTime &dateTime)
{
    emit measurementDateChanged(dateTime);
}

void MainWindow::spinBoxMeasurementDateOffsetValueChanged(int offset)
{
    emit measurementDateOffsetChanged(offset);
}

void MainWindow::dateTimeBirthdayDateDateTimeChanged(const QDateTime &dateTime)
{
    emit birthdayDateChanged(dateTime);
}

void MainWindow::spinBoxBirthdayDateOffsetValueChanged(int offset)
{
    emit birthdayOffsetChanged(offset);
}

void MainWindow::lineEditSubjectHisIdEditingFinished()
{
    emit subjectHisIdChanged(m_pUi->lineEditSubjectHisId->text());
}


