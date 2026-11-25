//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_anonymize_gui.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Drives the main window of mne_anonymize_gui: read a file, set options, anonymize.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include "settingscontrollergui.h"
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCheckBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace MNEANONYMIZE;

namespace
{

MainWindow* mainWindow()
{
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (auto* window = qobject_cast<MainWindow*>(widget)) {
            return window;
        }
    }
    return nullptr;
}

template<typename T>
T* child(const char* name)
{
    MainWindow* window = mainWindow();
    return window ? window->findChild<T*>(QString::fromLatin1(name)) : nullptr;
}

// Writes a small FIFF file with a subject block.
bool writeSubjectFile(const QString& path)
{
    const std::unique_ptr<FiffStream> stream = FiffStream::start_file(path.toStdU16String());
    if (!stream) {
        return false;
    }
    stream->start_block(FIFFB_MEAS);
    stream->start_block(FIFFB_SUBJECT);
    stream->write_string(FIFF_SUBJ_FIRST_NAME, "Jane");
    stream->write_string(FIFF_SUBJ_LAST_NAME, "Doe");
    stream->write_string(FIFF_SUBJ_HIS_ID, "patient-1");
    stream->end_block(FIFFB_SUBJECT);
    stream->end_block(FIFFB_MEAS);
    stream->end_file();
    return true;
}

std::optional<std::string> stringTag(const QString& file, fiff_int_t kind)
{
    FiffStream stream(file.toStdU16String());
    if (!stream.open()) {
        return std::nullopt;
    }
    std::optional<FiffTag> tag;
    while ((tag = stream.read_tag()) && tag->kind != kind) {
    }
    if (!tag) {
        return std::nullopt;
    }
    return tag->to_string();
}

// Answers the next modal dialog: picks @p file in a file dialog, or closes a message box and records its text.
void answerNextDialog(const QString& file, QString* messageText)
{
    QTimer::singleShot(0, [file, messageText]() {
        QWidget* modal = QApplication::activeModalWidget();
        if (auto* dialog = qobject_cast<QFileDialog*>(modal)) {
            if (file.isEmpty()) {
                dialog->reject();
            } else {
                dialog->selectFile(file);
                QMetaObject::invokeMethod(dialog, "accept");
            }
        } else if (auto* box = qobject_cast<QMessageBox*>(modal)) {
            if (messageText) {
                *messageText = box->text();
            }
            box->accept();
        } else {
            answerNextDialog(file, messageText);
        }
    });
}

std::vector<std::string> arguments(const QStringList& list)
{
    std::vector<std::string> args{"mne_anonymize_gui"};
    for (const QString& argument : list) {
        args.push_back(argument.toStdString());
    }
    return args;
}

} // namespace

//=============================================================================================================
/**
 * Tests the graphical user interface of mne_anonymize.
 */
class TestMneAnonymizeGui : public QObject
{
    Q_OBJECT

private slots:
    void testPresetsFromCommandLine();
    void testReadAndAnonymize();
    void testInvalidPathsAreRejected();
    void testFileDialogs();
    void testAnonymizeWithoutFilesAsksForThem();
    void testOptionsPanelAndNarrowWindow();
    void cleanup();
};

//=============================================================================================================

void TestMneAnonymizeGui::cleanup()
{
    QApplication::closeAllWindows();
    QCoreApplication::processEvents();
}

//=============================================================================================================

void TestMneAnonymizeGui::testPresetsFromCommandLine()
{
    QTemporaryDir dir;
    const QString fileIn = dir.filePath("subject.fif");
    QVERIFY(writeSubjectFile(fileIn));

    SettingsControllerGui controller(arguments({"--in", fileIn, "--brute", "--his", "his-from-cli",
                                                "--measurement_date_offset", "3", "--subject_birthday_offset", "5"}));
    QVERIFY(mainWindow());
    mainWindow()->resize(1200, 800);
    QCoreApplication::processEvents();

    QCOMPARE(child<QLineEdit>("lineEditInFile")->text(), QFileInfo(fileIn).absoluteFilePath());
    QVERIFY(child<QCheckBox>("checkBoxBruteMode")->isChecked());
    QCOMPARE(child<QLineEdit>("lineEditSubjectHisId")->text(), QString("his-from-cli"));
    QVERIFY(child<QCheckBox>("checkBoxMeasurementDateOffset")->isChecked());
    QCOMPARE(child<QSpinBox>("spinBoxMeasurementDateOffset")->value(), 3);
    QVERIFY(child<QCheckBox>("checkBoxBirthdayDateOffset")->isChecked());
    QCOMPARE(child<QSpinBox>("spinBoxBirthdayDateOffset")->value(), 5);
}

//=============================================================================================================

void TestMneAnonymizeGui::testReadAndAnonymize()
{
    QTemporaryDir dir;
    const QString fileIn = dir.filePath("subject.fif");
    QVERIFY(writeSubjectFile(fileIn));

    SettingsControllerGui controller(arguments({"--in", fileIn}));
    MainWindow* window = mainWindow();
    QVERIFY(window);
    window->resize(1200, 800);

    child<QCheckBox>("checkBoxShowOptions")->setChecked(true);
    QTest::mouseClick(child<QPushButton>("pushButtonReadData"), Qt::LeftButton);
    QCOMPARE(child<QLineEdit>("lineEditSubjectFirstNameExtra")->text(), QString("Jane"));
    QCOMPARE(child<QLineEdit>("lineEditSubjectLastNameExtra")->text(), QString("Doe"));
    QCOMPARE(child<QLineEdit>("lineEditSubjectHisIdExtra")->text(), QString("patient-1"));

    child<QCheckBox>("checkBoxBruteMode")->setChecked(true);
    child<QCheckBox>("checkBoxBruteMode")->setChecked(false);
    child<QDateTimeEdit>("dateTimeMeasurementDate")->setDateTime(QDateTime(QDate(2001, 2, 3), QTime(4, 5, 6)));
    child<QCheckBox>("checkBoxMeasurementDateOffset")->setChecked(true);
    child<QSpinBox>("spinBoxMeasurementDateOffset")->setValue(7);
    child<QCheckBox>("checkBoxMeasurementDateOffset")->setChecked(false);
    child<QDateEdit>("dateEditBirthdayDate")->setDate(QDate(1990, 6, 7));
    child<QCheckBox>("checkBoxBirthdayDateOffset")->setChecked(true);
    child<QSpinBox>("spinBoxBirthdayDateOffset")->setValue(9);
    child<QCheckBox>("checkBoxBirthdayDateOffset")->setChecked(false);

    QLineEdit* hisId = child<QLineEdit>("lineEditSubjectHisId");
    hisId->setText("his-from-gui");
    emit hisId->editingFinished();

    QLineEdit* outFile = child<QLineEdit>("lineEditOutFile");
    const QString fileOut = QFileInfo(dir.filePath("chosen.fif")).absoluteFilePath();
    outFile->setText(fileOut);
    emit outFile->editingFinished();

    QTest::mouseClick(child<QPushButton>("pushButtonAnonymizeFile"), Qt::LeftButton);

    QVERIFY(QFileInfo::exists(fileOut));
    QCOMPARE(window->statusBar()->currentMessage(), QString("Your file is ready!"));
    QCOMPARE(stringTag(fileOut, FIFF_SUBJ_HIS_ID), std::optional<std::string>("his-from-gui"));
    QCOMPARE(stringTag(fileOut, FIFF_SUBJ_FIRST_NAME), std::optional<std::string>("mne_anonymize"));
    // The temporary file written while reading the input data is removed again
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 2);
}

//=============================================================================================================

void TestMneAnonymizeGui::testInvalidPathsAreRejected()
{
    QTemporaryDir dir;
    const QString fileIn = QFileInfo(dir.filePath("subject.fif")).absoluteFilePath();
    QVERIFY(writeSubjectFile(fileIn));

    SettingsControllerGui controller(arguments({"--in", fileIn}));
    QVERIFY(mainWindow());
    mainWindow()->resize(1200, 800);

    QLineEdit* inFile = child<QLineEdit>("lineEditInFile");
    QLineEdit* outFile = child<QLineEdit>("lineEditOutFile");
    const QString defaultOut = outFile->text();

    inFile->setText(dir.path());
    emit inFile->editingFinished();
    QCOMPARE(inFile->text(), fileIn);
    QVERIFY(mainWindow()->statusBar()->currentMessage().contains("directory"));

    inFile->setText(dir.filePath("subject.txt"));
    emit inFile->editingFinished();
    QCOMPARE(inFile->text(), fileIn);

    inFile->setText(dir.filePath("missing.fif"));
    emit inFile->editingFinished();
    QCOMPARE(inFile->text(), fileIn);

    outFile->setText(dir.filePath("out.txt"));
    emit outFile->editingFinished();
    QCOMPARE(outFile->text(), defaultOut);

    outFile->setText(dir.path());
    emit outFile->editingFinished();
    QVERIFY(outFile->text().endsWith("subject_anonymized.fif"));
}

//=============================================================================================================

void TestMneAnonymizeGui::testFileDialogs()
{
    QTemporaryDir dir;
    const QString fileIn = QFileInfo(dir.filePath("subject.fif")).absoluteFilePath();
    QVERIFY(writeSubjectFile(fileIn));
    const QString fileOut = QFileInfo(dir.filePath("picked_out.fif")).absoluteFilePath();

    SettingsControllerGui controller(arguments({}));
    QVERIFY(mainWindow());
    mainWindow()->resize(1200, 800);

    answerNextDialog(fileIn, nullptr);
    QTest::mouseClick(child<QToolButton>("openInFileWindowButton"), Qt::LeftButton);
    QCOMPARE(child<QLineEdit>("lineEditInFile")->text(), fileIn);
    QCOMPARE(child<QLineEdit>("lineEditOutFile")->text(), QFileInfo(dir.filePath("subject_anonymized.fif")).absoluteFilePath());

    answerNextDialog(fileOut, nullptr);
    QTest::mouseClick(child<QToolButton>("openOutFileWindowButton"), Qt::LeftButton);
    QCOMPARE(child<QLineEdit>("lineEditOutFile")->text(), fileOut);

    // Cancelling keeps the paths
    answerNextDialog(QString(), nullptr);
    QTest::mouseClick(child<QToolButton>("openInFileWindowButton"), Qt::LeftButton);
    QCOMPARE(child<QLineEdit>("lineEditInFile")->text(), fileIn);

    QString about;
    answerNextDialog(QString(), &about);
    QTest::mouseClick(child<QToolButton>("moreInfoButton"), Qt::LeftButton);
    QVERIFY(about.contains("mne_anonymize"));
}

//=============================================================================================================

void TestMneAnonymizeGui::testAnonymizeWithoutFilesAsksForThem()
{
    SettingsControllerGui controller(arguments({}));
    QVERIFY(mainWindow());

    QString message;
    answerNextDialog(QString(), &message);
    QTest::mouseClick(child<QPushButton>("pushButtonAnonymizeFile"), Qt::LeftButton);
    QCOMPARE(message, QString("Please specify a valid input file first."));

    message.clear();
    answerNextDialog(QString(), &message);
    child<QCheckBox>("checkBoxShowOptions")->setChecked(true);
    QTest::mouseClick(child<QPushButton>("pushButtonReadData"), Qt::LeftButton);
    QCOMPARE(message, QString("Cannot read data. Please select a valid input file first."));
}

//=============================================================================================================

void TestMneAnonymizeGui::testOptionsPanelAndNarrowWindow()
{
    QTemporaryDir dir;
    const QString fileIn = QFileInfo(dir.filePath("subject.fif")).absoluteFilePath();
    QVERIFY(writeSubjectFile(fileIn));

    SettingsControllerGui controller(arguments({"--in", fileIn}));
    MainWindow* window = mainWindow();
    QVERIFY(window);

    QCheckBox* showOptions = child<QCheckBox>("checkBoxShowOptions");
    showOptions->setChecked(true);
    QVERIFY(!child<QPushButton>("pushButtonReadData")->isHidden());
    showOptions->setChecked(false);
    QVERIFY(child<QPushButton>("pushButtonReadData")->isHidden());

    // A narrow window shortens the paths, a wide one shows them in full
    window->setMinimumWidth(0);
    window->resize(200, window->height());
    QCoreApplication::processEvents();
    QVERIFY(child<QLineEdit>("lineEditInFile")->text().startsWith("(...)/"));
    window->resize(1600, window->height());
    QCoreApplication::processEvents();
    QCOMPARE(child<QLineEdit>("lineEditInFile")->text(), fileIn);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestMneAnonymizeGui)
#include "test_mne_anonymize_gui.moc"
