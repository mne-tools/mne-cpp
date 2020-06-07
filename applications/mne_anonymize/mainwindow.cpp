#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <settingscontrollergui.h>
#include <iostream>

#include <QMessageBox>
#include <QCloseEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QDateTime>

using namespace MNEANONYMIZE;

MainWindow::MainWindow(MNEANONYMIZE::SettingsControllerGui *c)
: m_bIdFileVersionFound(false)
, m_bIdMeasurementDateFound(false)
, m_bIdMacAddressFound(false)
, m_bFileMeasurementDateFound(false)
, m_bFileExperimenterFound(false)
, m_bFileCommentFound(false)
, m_bSubjectIdFound(false)
, m_bSubjectFirstNameFound(false)
, m_bSubjectMiddleNameFound(false)
, m_bSubjectLastNameFound(false)
, m_bSubjectBirthdayFound(false)
, m_bSubjectSexFound(false)
, m_bSubjectHandFound(false)
, m_bSubjectWeightFound(false)
, m_bSubjectHeightFound(false)
, m_bSubjectCommentFound(false)
, m_bSubjectHisIdFound(false)
, m_bProjectIdFound(false)
, m_bProjectAimFound(false)
, m_bProjectNameFound(false)
, m_bProjectPersonsFound(false)
, m_bProjectCommentFound(false)
, m_bHideEachField(true)
, m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);
    setDefautlStateUi();
    setupConections();
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

void MainWindow::setDefautlStateUi()
{
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);

    m_pUi->frameFile->setHidden(m_bHideEachField);
    m_pUi->frameSubject->setHidden(m_bHideEachField);
    m_pUi->frameProject->setHidden(m_bHideEachField);

    m_pUi->lineEditFileVersionExtra->setEnabled(false);
    m_pUi->lineEditMACAddressExtra->setEnabled(false);

    m_pUi->comboBoxSubjectSexExtra->addItems(QStringList() << "Unknown" << "Male" << "Female");
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectSexExtra->setEditable(false);

    m_pUi->comboBoxSubjectHandExtra->addItems(QStringList() << "Unknown" << "Right" << "Left");
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(0);
    m_pUi->comboBoxSubjectHandExtra->setEditable(false);

    m_bIdFileVersionFound = false;
    m_bIdMeasurementDateFound = false;
    m_bIdMacAddressFound = false;
    m_bFileMeasurementDateFound = false;
    m_bFileExperimenterFound = false;
    m_bFileCommentFound = false;
    m_bSubjectIdFound = false;
    m_bSubjectFirstNameFound = false;
    m_bSubjectMiddleNameFound = false;
    m_bSubjectLastNameFound = false;
    m_bSubjectBirthdayFound = false;
    m_bSubjectSexFound = false;
    m_bSubjectHandFound = false;
    m_bSubjectWeightFound = false;
    m_bSubjectHeightFound = false;
    m_bSubjectCommentFound = false;
    m_bSubjectHisIdFound = false;
    m_bProjectIdFound = false;
    m_bProjectAimFound = false;
    m_bProjectNameFound = false;
    m_bProjectPersonsFound = false;
    m_bProjectCommentFound = false;

}

void MainWindow::setupConections()
{
    QObject::connect(m_pUi->checkBoxEditExtra,&QCheckBox::stateChanged,
                     this,&MainWindow::setExtraObjectstoState);
    QObject::connect(m_pUi->checkBoxEditExtra,&QCheckBox::stateChanged,
                     this,&MainWindow::setCheckBoxEditExtraUnmutable);

    QObject::connect(m_pUi->seeExtraInfoButton,&QToolButton::clicked,
                     this,&MainWindow::seeExtraInformationClicked);
    QObject::connect(m_pUi->buttonMenu,&QDialogButtonBox::helpRequested,
                     this,&MainWindow::helpButtonClicked);

    QObject::connect(m_pUi->lineEditInFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditInFileEditingFinished);

    QObject::connect(m_pUi->lineEditOutFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditOutFileEditingFinished);
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

void MainWindow::setExtraObjectstoState()
{
    m_pUi->lineEditFileVersionExtra->setEnabled(m_bIdFileVersionFound);
    m_pUi->dateTimeIdMeasurementDateExtra->setEnabled(m_bIdMeasurementDateFound);
    m_pUi->lineEditMACAddressExtra->setEnabled(m_bIdMacAddressFound);
    m_pUi->dateTimeFileMeasurementDateExtra->setEnabled(m_bFileMeasurementDateFound);

    m_pUi->lineEditExperimenterExtra->setEnabled(m_bFileExperimenterFound);
    m_pUi->plainTextFileCommentExtra->setEnabled(m_bFileCommentFound);

    m_pUi->spinBoxSubjectIDExtra->setEnabled(m_bSubjectIdFound);
    m_pUi->lineEditSubjectFirstNameExtra->setEnabled(m_bSubjectFirstNameFound);
    m_pUi->lineEditSubjectMiddleNameExtra->setEnabled(m_bSubjectMiddleNameFound);
    m_pUi->lineEditSubjectLastNameExtra->setEnabled(m_bSubjectLastNameFound);
    m_pUi->dateEditSubjectBirthdayExtra->setEnabled(m_bSubjectBirthdayFound);
    m_pUi->comboBoxSubjectSexExtra->setEnabled(m_bSubjectSexFound);
    m_pUi->comboBoxSubjectHandExtra->setEnabled(m_bSubjectHandFound);
    m_pUi->doubleSpinBoxSubjectWeightExtra->setEnabled(m_bSubjectWeightFound);
    m_pUi->doubleSpinBoxSubjectHeightExtra->setEnabled(m_bSubjectHeightFound);
    m_pUi->plainTextEditSubjectCommentExtra->setEnabled(m_bSubjectCommentFound);
    m_pUi->lineEditSubjectHisIdExtra->setEnabled(m_bSubjectHisIdFound);

    m_pUi->spinBoxProjectIDExtra->setEnabled(m_bProjectIdFound);
    m_pUi->lineEditProjectAimExtra->setEnabled(m_bProjectAimFound);
    m_pUi->lineEditProjectNameExtra->setEnabled(m_bProjectNameFound);
    m_pUi->lineEditProjectPersonsExtra->setEnabled(m_bProjectPersonsFound);
    m_pUi->plainTextEditProjectCommentExtra->setEnabled(m_bProjectCommentFound);
}

void MainWindow::setLineEditInFile(const QString &f)
{
    m_pUi->lineEditInFile->setText(f);
}

void MainWindow::setLineEditOutFile(const QString &f)
{
    m_pUi->lineEditOutFile->setText(f);
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


//public slots for extra information
void MainWindow::setIdFileVersion(double v)
{
//    m_pUi->lineEditFileVersionExtra->setVisible(true);
//    m_pUi->labelFileVersionExtra->setVisible(true);
    m_bIdFileVersionFound = true;
    m_pUi->lineEditFileVersionExtra->setText(QString::number(v));
}

void MainWindow::setIdMeasurementDate(QDateTime d)
{
//    m_pUi->labelIdMeasurementDateExtra->setVisible(true);
//    m_pUi->dateTimeIdMeasurementDateExtra->setVisible(true);
    m_bIdMeasurementDateFound = true;
    m_pUi->dateTimeIdMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setIdMacAddress(QString mac)
{
//    m_pUi->labelMACAddressExtra->setVisible(true);
//    m_pUi->lineEditMACAddressExtra->setVisible(true);
    m_bIdMacAddressFound = true;
    m_pUi->lineEditMACAddressExtra->setText(mac);
}

void MainWindow::setFileMeasurementDate(QDateTime d)
{
//    m_pUi->labelMeasDate->setVisible(true);
//    m_pUi->dateTimeFileMeasurementDateExtra->setVisible(true);
    m_bFileMeasurementDateFound = true;
    m_pUi->dateTimeFileMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setFileComment(QString c)
{
//    m_pUi->labelFileCommentExtra->setVisible(true);
//    m_pUi->plainTextFileCommentExtra->setVisible(true);
    m_bFileCommentFound = true;
    m_pUi->plainTextFileCommentExtra->setPlainText(c);
}

void MainWindow::setFileExperimenter(QString e)
{
//    m_pUi->labelExperimenterExtra->setVisible(true);
//    m_pUi->lineEditExperimenterExtra->setVisible(true);
    m_bFileExperimenterFound = true;
    m_pUi->lineEditExperimenterExtra->setText(e);
}

void MainWindow::setSubjectId(int i)
{
//    m_pUi->labelSubjectIDExtra->setVisible(true);
//    m_pUi->spinBoxSubjectIDExtra->setVisible(true);
    m_bSubjectIdFound = true;
    m_pUi->spinBoxSubjectIDExtra->setValue(i);
}

void MainWindow::setSubjectFirstName(QString fn)
{
//    m_pUi->labelSubjectFirstNameExtra->setVisible(true);
//    m_pUi->lineEditSubjectFirstNameExtra->setVisible(true);
    m_bSubjectFirstNameFound = true;
    m_pUi->lineEditSubjectFirstNameExtra->setText(fn);
}

void MainWindow::setSubjectMiddleName(QString mn)
{
    m_bSubjectMiddleNameFound = true;
    m_pUi->lineEditSubjectMiddleNameExtra->setText(mn);
}

void MainWindow::setSubjectLastName(QString ln)
{
    m_bSubjectLastNameFound = true;
    m_pUi->lineEditSubjectLastNameExtra->setText(ln);
}

void MainWindow::setSubjectBirthday(QDateTime b)
{
    m_bSubjectBirthdayFound = true;
    m_pUi->dateTimeBirthdayDate->setDateTime(b);
}

void MainWindow::setSubjectSex(int s)
{
    m_bSubjectSexFound = true;
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(s);
}

void MainWindow::setSubjectHand(int h)
{
    m_bSubjectHandFound = true;
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(h);
}

void MainWindow::setSubjectWeight(float w)
{
    m_bSubjectWeightFound = true;
    double wd(static_cast<double>(w));
    m_pUi->doubleSpinBoxSubjectWeightExtra->setValue(wd);
}

void MainWindow::setSubjectHeight(float h)
{
    m_bSubjectHeightFound = true;
    double hd(static_cast<double>(h));
    m_pUi->doubleSpinBoxSubjectHeightExtra->setValue(hd);
}

void MainWindow::setSubjectComment(QString c)
{
    m_bSubjectCommentFound = true;
    m_pUi->plainTextEditSubjectCommentExtra->setPlainText(c);
}

void MainWindow::setSubjectHisId(QString his)
{
    m_bSubjectHisIdFound = true;
    m_pUi->lineEditSubjectHisIdExtra->setText(his);
}

void MainWindow::setProjectId(int id)
{
    m_bProjectIdFound = true;
    m_pUi->spinBoxProjectIDExtra->setValue(id);
}

void MainWindow::setProjectName(QString p)
{
    m_bProjectNameFound = true;
    m_pUi->lineEditProjectNameExtra->setText(p);
}

void MainWindow::setProjectAim(QString p)
{
    m_bProjectAimFound = true;
    m_pUi->lineEditProjectAimExtra->setText(p);
}

void MainWindow::setProjectPersons(QString p)
{
    m_bProjectPersonsFound = true;
    m_pUi->lineEditProjectPersonsExtra->setText(p);
}

void MainWindow::setProjectComment(QString c)
{
    m_bProjectCommentFound = true;
    m_pUi->plainTextEditProjectCommentExtra->setPlainText(c);
}

void MainWindow::seeExtraInformationClicked()
{
    m_bHideEachField = !m_bHideEachField;
    m_pUi->frameFile->setHidden(m_bHideEachField);
    m_pUi->frameSubject->setHidden(m_bHideEachField);
    m_pUi->frameProject->setHidden(m_bHideEachField);
}

void MainWindow::helpButtonClicked()
{
    QDesktopServices::openUrl( QUrl("https://mne-cpp.github.io/pages/learn/mneanonymize.html",
                               QUrl::TolerantMode) );
}

void MainWindow::setCheckBoxEditExtraUnmutable()
{
    m_pUi->checkBoxEditExtra->setDisabled(true);
}

// /////////////////////////////// ui private slots

void MainWindow::lineEditInFileEditingFinished()
{
    //std::printf("\n%s\n",m_pUi->lineEditInFile->text().toUtf8().data());
    emit fileInChanged(m_pUi->lineEditInFile->text());
}

void MainWindow::lineEditOutFileEditingFinished()
{
    emit fileOutChanged(m_pUi->lineEditOutFile->text());
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


