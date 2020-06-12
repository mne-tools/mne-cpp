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
  m_bHideExtraInfoFields(true)
, m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);
    setDefautlStateUi();
    setDefaultStateExtraInfo();
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

void MainWindow::setDefaultStateExtraInfo()
{
    m_pUi->lineEditIdFileVersionExtra->clear();
    m_pUi->lineEditIdMACAddressExtra->clear();
    m_pUi->dateTimeIdMeasurementDateExtra->clear();

    m_pUi->dateTimeFileMeasurementDateExtra->clear();

    m_pUi->lineEditExperimenterExtra->clear();
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
    m_pUi->plainTextEditSubjectCommentExtra->clear();
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

    //tooltips

    m_pUi->checkBoxBruteMode->setToolTip("Advanced anonymization. Anonymize also weight, height and some other fields. See Help.");
//    m_pUi->checkBoxBruteMode->setToolTip("Apart from other fields already anonymized by default, \nalso anonymize subject’s weight, height, sex and handedness, and project’s ID, name, aim and comment.");
    m_pUi->labelMeasDate->setToolTip("Specify the measurement date.");
    m_pUi->dateTimeMeasurementDate->setToolTip("Specify the measurement date.");
    m_pUi->checkBoxMeasurementDateOffset->setToolTip("Specify number of days to subtract to the measurement.");
    m_pUi->spinBoxMeasurementDateOffset->setToolTip("Specify number of days to subtract to the measurement.");

    m_pUi->labelSubjectBirthday->setToolTip("Specify the subject’s birthday.");
    m_pUi->dateTimeBirthdayDate->setToolTip("Specify the subject’s birthday.");
    m_pUi->checkBoxBirthdayDateOffset->setToolTip("Specify a number of to subtract from the subject's birthday.");
    m_pUi->spinBoxBirthdayDateOffset->setToolTip("Specify a number of to subtract from the subject's birthday.");

    m_pUi->labelSubjectHisId->setToolTip("Specify the Subject’s ID within the Hospital system.");
    m_pUi->lineEditSubjectHisId->setToolTip("Specify the Subject’s ID within the Hospital system.");

    m_pUi->buttonMenu->setToolTip("[Help] See the MNE-CPP project's documentation website.\n[Save] Save the anonymized file.");

    m_pUi->labelInFile->setToolTip("File to anonymize");
    m_pUi->lineEditInFile->setToolTip("File to anonymize.");
    m_pUi->openInFileWindowButton->setToolTip("Select a file to anonymize.");
    m_pUi->labelOutFile->setToolTip("Output anonymized file. By default a \"_anonymized\" suffix is added to the name of the input file.");
    m_pUi->lineEditOutFile->setToolTip("Output anonymized file. By default a \"_anonymized\" suffix is added to the name of the input file.");
    m_pUi->openOutFileWindowButton->setToolTip("Select a folder or a file where to save the output anonymized fif file.");
}

void MainWindow::setDefautlStateUi()
{

    this->setWindowTitle(qApp->organizationName() + " ~ " + qApp->applicationName() + " ~ " + qApp->applicationVersion());
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(false);
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(false);
    m_pUi->spinBoxMeasurementDateOffset->setValue(0);
    m_pUi->spinBoxBirthdayDateOffset->setValue(0);

    m_pUi->frameExtraInfo->setVisible(m_bHideExtraInfoFields);

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

    m_pUi->lineEditExperimenterExtra->setReadOnly(true);
    m_pUi->plainTextFileCommentExtra->setReadOnly(true);

    m_pUi->spinBoxSubjectIDExtra->setReadOnly(true);
    m_pUi->lineEditSubjectFirstNameExtra->setReadOnly(true);
    m_pUi->lineEditSubjectMiddleNameExtra->setReadOnly(true);
    m_pUi->lineEditSubjectLastNameExtra->setReadOnly(true);
    m_pUi->dateEditSubjectBirthdayExtra->setReadOnly(true);
    m_pUi->doubleSpinBoxSubjectWeightExtra->setReadOnly(true);
    m_pUi->doubleSpinBoxSubjectHeightExtra->setReadOnly(true);
    m_pUi->plainTextEditSubjectCommentExtra->setReadOnly(true);
    m_pUi->lineEditSubjectHisIdExtra->setReadOnly(true);

    m_pUi->labelSubjectMriDataFoundExtra->setVisible(false);

    m_pUi->spinBoxProjectIDExtra->setReadOnly(true);
    m_pUi->lineEditProjectAimExtra->setReadOnly(true);
    m_pUi->lineEditProjectNameExtra->setReadOnly(true);
    m_pUi->lineEditProjectPersonsExtra->setReadOnly(true);
    m_pUi->plainTextEditProjectCommentExtra->setReadOnly(true);
}

void MainWindow::setupConnections()
{

    QObject::connect(m_pUi->seeExtraInfoButton,&QToolButton::clicked,
                     this,&MainWindow::showExtraInfo);

    QObject::connect(m_pUi->buttonMenu,&QDialogButtonBox::accepted,
                     m_pController,&SettingsControllerGui::executeAnonymizer);

    QObject::connect(m_pUi->buttonMenu,&QDialogButtonBox::helpRequested,
                     this,&MainWindow::helpButtonClicked);

    QObject::connect(m_pUi->lineEditInFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditInFileEditingFinished);
    QObject::connect(m_pUi->lineEditOutFile,&QLineEdit::editingFinished,
                     this,&MainWindow::lineEditOutFileEditingFinished);

    QObject::connect(m_pUi->openInFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openInFileDialog);
    QObject::connect(m_pUi->openOutFileWindowButton,&QToolButton::clicked,
                     this,&MainWindow::openOutFileDialog);

    QObject::connect(m_pUi->checkBoxBruteMode,&QCheckBox::stateChanged,
                     this,&MainWindow::checkboxBruteModeChanged);

    QObject::connect(m_pUi->checkBoxMeasurementDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxMeasurementDateOffsetStateChanged);
    QObject::connect(m_pUi->spinBoxMeasurementDateOffset,QOverload<int>::of(&QSpinBox::valueChanged),
                     this,&MainWindow::spinBoxMeasurementDateOffsetValueChanged);
    QObject::connect(m_pUi->dateTimeMeasurementDate,&QDateTimeEdit::dateTimeChanged,
                     this,&MainWindow::dateTimeMeasurementDateDateTimeChanged);

    QObject::connect(m_pUi->checkBoxBirthdayDateOffset,&QCheckBox::stateChanged,
                     this,&MainWindow::checkBoxBirthdayDateOffsetStateChanged);
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
    m_pUi->checkBoxMeasurementDateOffset->setChecked(o);
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
    m_pUi->lineEditIdFileVersionExtra->setEnabled(true);
    m_pUi->lineEditIdFileVersionExtra->setText(QString::number(v));
}

void MainWindow::setLineEditIdMeasurementDate(QDateTime d)
{
    m_pUi->dateTimeIdMeasurementDateExtra->setEnabled(true);
    m_pUi->dateTimeIdMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setLineEditIdMacAddress(QString mac)
{
    m_pUi->lineEditIdMACAddressExtra->setEnabled(true);
    m_pUi->lineEditIdMACAddressExtra->setText(mac);
}

void MainWindow::setLineEditFileMeasurementDate(QDateTime d)
{
    m_pUi->dateTimeFileMeasurementDateExtra->setEnabled(true);
    m_pUi->dateTimeFileMeasurementDateExtra->setDateTime(d);
}

void MainWindow::setLineEditFileComment(QString c)
{
    m_pUi->plainTextFileCommentExtra->setEnabled(true);
    m_pUi->plainTextFileCommentExtra->setPlainText(c);
}

void MainWindow::setLineEditFileExperimenter(QString e)
{
    m_pUi->lineEditExperimenterExtra->setEnabled(true);
    m_pUi->lineEditExperimenterExtra->setText(e);
}

void MainWindow::setLineEditSubjectId(int i)
{
    m_pUi->spinBoxSubjectIDExtra->setEnabled(true);
    m_pUi->spinBoxSubjectIDExtra->setValue(i);
}

void MainWindow::setLineEditSubjectFirstName(QString fn)
{
    m_pUi->lineEditSubjectFirstNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectFirstNameExtra->setText(fn);
}

void MainWindow::setLineEditSubjectMiddleName(QString mn)
{
    m_pUi->lineEditSubjectMiddleNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectMiddleNameExtra->setText(mn);
}

void MainWindow::setLineEditSubjectLastName(QString ln)
{
    m_pUi->lineEditSubjectLastNameExtra->setEnabled(true);
    m_pUi->lineEditSubjectLastNameExtra->setText(ln);
}

void MainWindow::setLineEditSubjectBirthday(QDateTime b)
{
    m_pUi->dateTimeBirthdayDate->setEnabled(true);
    m_pUi->dateTimeBirthdayDate->setDateTime(b);
}

void MainWindow::setComboBoxSubjectSex(int s)
{
    m_pUi->comboBoxSubjectSexExtra->setEnabled(true);
    m_pUi->comboBoxSubjectSexExtra->setCurrentIndex(s);
}

void MainWindow::setLineEditSubjectHand(int h)
{
    m_pUi->comboBoxSubjectHandExtra->setEnabled(true);
    m_pUi->comboBoxSubjectHandExtra->setCurrentIndex(h);
}

void MainWindow::setLineEditSubjectWeight(float w)
{
    m_pUi->doubleSpinBoxSubjectWeightExtra->setEnabled(true);
    double wd(static_cast<double>(w));
    m_pUi->doubleSpinBoxSubjectWeightExtra->setValue(wd);
}

void MainWindow::setLineEditSubjectHeight(float h)
{
    m_pUi->doubleSpinBoxSubjectHeightExtra->setEnabled(true);
    double hd(static_cast<double>(h));
    m_pUi->doubleSpinBoxSubjectHeightExtra->setValue(hd);
}

void MainWindow::setLineEditSubjectComment(QString c)
{
    m_pUi->plainTextEditSubjectCommentExtra->setEnabled(true);
    m_pUi->plainTextEditSubjectCommentExtra->setPlainText(c);
}

void MainWindow::setLineEditSubjectHisId(QString his)
{
    m_pUi->lineEditSubjectHisIdExtra->setEnabled(true);
    m_pUi->lineEditSubjectHisIdExtra->setText(his);
}

void MainWindow::setLineEditProjectId(int id)
{
    m_pUi->spinBoxProjectIDExtra->setEnabled(true);
    m_pUi->spinBoxProjectIDExtra->setValue(id);
}

void MainWindow::setLineEditProjectName(QString p)
{
    m_pUi->lineEditProjectNameExtra->setEnabled(true);
    m_pUi->lineEditProjectNameExtra->setText(p);
}

void MainWindow::setLineEditProjectAim(QString p)
{
    m_pUi->lineEditProjectAimExtra->setEnabled(true);
    m_pUi->lineEditProjectAimExtra->setText(p);
}

void MainWindow::setLineEditProjectPersons(QString p)
{
    m_pUi->lineEditProjectPersonsExtra->setEnabled(true);
    m_pUi->lineEditProjectPersonsExtra->setText(p);
}

void MainWindow::setLineEditProjectComment(QString c)
{
    m_pUi->plainTextEditProjectCommentExtra->setEnabled(true);
    m_pUi->plainTextEditProjectCommentExtra->setPlainText(c);
}

void MainWindow::setLabelMriDataFoundVisible(bool b)
{
    m_pUi->labelSubjectMriDataFoundExtra->setVisible(b);
}

void MainWindow::openInFileDialog()
{
#ifdef WASMBUILD
    auto fileContentReady = [&](const QString &filePath, const QByteArray &fileContent) {
        if(!filePath.isNull()) {
            // We need to prepend "wasm/" because QFileDialog::getOpenFileContent does not provide a full
            // path, which we need for organzing the different models in AnalyzeData
            m_pAnalyzeData->loadModel<FiffRawViewModel>("wasm/"+filePath, fileContent);
        }
    };
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
        setLineEditInFile(fileNames.at(0));
        lineEditInFileEditingFinished();
    }
#endif
}

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
        setLineEditOutFile(fileNames.at(0));
        lineEditOutFileEditingFinished();
    }
}

void MainWindow::showExtraInfo()
{
    m_bHideExtraInfoFields = !m_bHideExtraInfoFields;
    m_pUi->frameExtraInfo->setVisible(m_bHideExtraInfoFields);
}


void MainWindow::helpButtonClicked()
{
    QDesktopServices::openUrl( QUrl("https://mne-cpp.github.io/pages/learn/mneanonymize.html",
                               QUrl::TolerantMode) );
}

void MainWindow::lineEditInFileEditingFinished()
{
    emit fileInChanged(m_pUi->lineEditInFile->text());
}

void MainWindow::lineEditOutFileEditingFinished()
{
    emit fileOutChanged(m_pUi->lineEditOutFile->text());
}

void MainWindow::checkboxBruteModeChanged()
{
    bool state(m_pUi->checkBoxBruteMode->isChecked());
    emit bruteModeChanged(state);
    if(state)
    {
        statusMsg("Brute mode selected",700);
    } else {
        statusMsg("Brute mode deselected",700);
    }
}

void MainWindow::checkBoxMeasurementDateOffsetStateChanged(int arg)
{
    Q_UNUSED(arg)
    bool state(m_pUi->checkBoxMeasurementDateOffset->isChecked());
    m_pUi->spinBoxMeasurementDateOffset->setEnabled(state);
    emit useMeasurementOffset(state);
    m_pUi->dateTimeMeasurementDate->setEnabled(!state);
}

void MainWindow::checkBoxBirthdayDateOffsetStateChanged(int arg)
{
    Q_UNUSED(arg)
    bool state(m_pUi->checkBoxBirthdayDateOffset->isChecked());
    m_pUi->spinBoxBirthdayDateOffset->setEnabled(state);
    emit useBirthdayOffset(state);
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

void MainWindow::winPopup(QString s)
{
    QMessageBox msgBox;
    msgBox.setText(s);
    msgBox.exec();
    return;
}

void MainWindow::statusMsg(const QString &s,int to)
{
    if( to == 0 )
    {
        m_pUi->statusbar->showMessage(s);
    } else {
        m_pUi->statusbar->showMessage(s,to);
    }
}
