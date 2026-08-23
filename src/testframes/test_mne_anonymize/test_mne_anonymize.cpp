
//=============================================================================================================
/**
 * @file     test_mne_anonymize.cpp
 * @author   Lorenz Esch <lorenzesch@hotmail.com>;
 * @since    0.1.0
 * @date     September, 2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Test for anonymizing a fiff raw file
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include "../../tools/preprocessing/mne_anonymize/fiffanonymizer.h"
#include "../../tools/preprocessing/mne_anonymize/settingscontrollercl.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QTimeZone>

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNEANONYMIZE;

class TestSettingsControllerCl : public SettingsControllerCl
{
public:
    using SettingsControllerCl::SettingsControllerCl;

    FiffAnonymizer* anonymizer() const
    {
        return m_pAnonymizer.data();
    }

    QString randomFileName()
    {
        return generateRandomFileName();
    }

    bool deleteInputFileAfter() const
    {
        return m_bDeleteInputFileAfter;
    }

    bool deleteInputFileConfirmation() const
    {
        return m_bDeleteInputFileConfirmation;
    }
};

//=============================================================================================================
/**
 * DECLARE CLASS TestMneAnonymize
 *
 * @brief The TestMneAnonymize class provides fiff anonymizing verification tests
 *
 */
class TestMneAnonymize: public QObject
{
    Q_OBJECT

public:
    TestMneAnonymize();

private slots:
    //test app behaviour
    void initTestCase();
    void testAnonymizerDefaultsAndRequiredFiles();
    void testAnonymizerConfiguration();
    void testAnonymizerCopyAndMove();
    void testCommandLineConfiguration();
    void testCommandLineOffsetsAndInvalidPaths();
    void testDefaultOutput();
    void testDeleteInputFile();
    void testInPlace();

    //test anonymization
    void testDefaultAnonymizationOfTags();
    void compareBirthdayOffsetOption();
    void compareMeasureDateOffsetOption();
    void cleanupTestCase();

private:
    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;

    void verifyTags(FIFFLIB::FiffStream::SPtr &outStream,
                    QString testArg="blank");
    int mDaysToOffsetDates;
    QDateTime mDefaultMeasDate;
};

//=============================================================================================================

TestMneAnonymize::TestMneAnonymize()
: mDaysToOffsetDates(35)
, mDefaultMeasDate(QDate(2000,1,1), QTime(1, 1, 0), QTimeZone::LocalTime)
{
}

//=============================================================================================================

void TestMneAnonymize::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
}

//=============================================================================================================

void TestMneAnonymize::testAnonymizerDefaultsAndRequiredFiles()
{
    FiffAnonymizer anonymizer;

    QVERIFY(!anonymizer.isFileInSet());
    QVERIFY(!anonymizer.isFileOutSet());
    QVERIFY(anonymizer.getFileNameIn().isEmpty());
    QVERIFY(anonymizer.getFileNameOut().isEmpty());
    QVERIFY(!anonymizer.getVerboseMode());
    QVERIFY(!anonymizer.getBruteMode());
    QVERIFY(!anonymizer.getMNEEnvironmentMode());
    QCOMPARE(anonymizer.getMeasurementDate().date(), QDate(2000, 1, 1));
    QCOMPARE(anonymizer.getMeasurementDate().time(), QTime(1, 1, 0));
    QCOMPARE(anonymizer.getMeasurementDayOffset(), 0);
    QVERIFY(!anonymizer.getUseMeasurementDayOffset());
    QCOMPARE(anonymizer.getSubjectBirthday(), QDate(2000, 1, 1));
    QCOMPARE(anonymizer.getSubjectBirthdayOffset(), 0);
    QVERIFY(!anonymizer.getUseSubjectBirthdayOffset());
    QCOMPARE(anonymizer.getSubjectHisID(), QString("mne_anonymize"));

    QCOMPARE(anonymizer.anonymizeFile(), 1);
    QCOMPARE(anonymizer.setInFile("input.fif"), 0);
    QCOMPARE(anonymizer.anonymizeFile(), 1);
}

//=============================================================================================================

void TestMneAnonymize::testAnonymizerConfiguration()
{
    FiffAnonymizer anonymizer;
    const QDateTime measurementDate(QDate(1998, 7, 6), QTime(14, 30));
    const QDate birthday(1975, 3, 2);

    anonymizer.setVerboseMode(true);
    anonymizer.setBruteMode(true);
    anonymizer.setMNEEnvironmentMode(true);
    anonymizer.setMeasurementDate(measurementDate);
    anonymizer.setMeasurementDateOffset(42);
    anonymizer.setUseMeasurementDateOffset(true);
    anonymizer.setSubjectBirthday(birthday);
    anonymizer.setSubjectBirthdayOffset(17);
    anonymizer.setUseSubjectBirthdayOffset(true);
    anonymizer.setSubjectHisId("subject-001");

    QVERIFY(anonymizer.getVerboseMode());
    QVERIFY(anonymizer.getBruteMode());
    QVERIFY(anonymizer.getMNEEnvironmentMode());
    QCOMPARE(anonymizer.getMeasurementDate().date(), measurementDate.date());
    QCOMPARE(anonymizer.getMeasurementDate().time(), QTime(1, 1, 0));
    QCOMPARE(anonymizer.getMeasurementDayOffset(), 42);
    QVERIFY(anonymizer.getUseMeasurementDayOffset());
    QCOMPARE(anonymizer.getSubjectBirthday(), birthday);
    QCOMPARE(anonymizer.getSubjectBirthdayOffset(), 17);
    QVERIFY(anonymizer.getUseSubjectBirthdayOffset());
    QCOMPARE(anonymizer.getSubjectHisID(), QString("subject-001"));

    anonymizer.setMeasurementDate("05042001");
    anonymizer.setSubjectBirthday("03021980");
    QCOMPARE(anonymizer.getMeasurementDate().date(), QDate(2001, 4, 5));
    QCOMPARE(anonymizer.getSubjectBirthday(), QDate(1980, 2, 3));

    QCOMPARE(anonymizer.setInFile("input.fif"), 0);
    QVERIFY(anonymizer.isFileInSet());
    QCOMPARE(anonymizer.setOutFile("input.fif"), 1);
    QVERIFY(!anonymizer.isFileOutSet());
    QCOMPARE(anonymizer.setOutFile("output.fif"), 0);
    QVERIFY(anonymizer.isFileOutSet());
    QVERIFY(QFileInfo(anonymizer.getFileNameIn()).isAbsolute());
    QVERIFY(QFileInfo(anonymizer.getFileNameOut()).isAbsolute());
}

//=============================================================================================================

void TestMneAnonymize::testAnonymizerCopyAndMove()
{
    FiffAnonymizer original;
    original.setBruteMode(true);
    original.setMeasurementDateOffset(23);
    original.setSubjectHisId("subject-002");

    FiffAnonymizer copy(original);
    QCOMPARE(copy.getBruteMode(), original.getBruteMode());
    QCOMPARE(copy.getMeasurementDayOffset(), original.getMeasurementDayOffset());
    QCOMPARE(copy.getSubjectHisID(), original.getSubjectHisID());

    FiffAnonymizer moved(std::move(copy));
    QCOMPARE(moved.getBruteMode(), original.getBruteMode());
    QCOMPARE(moved.getMeasurementDayOffset(), original.getMeasurementDayOffset());
    QCOMPARE(moved.getSubjectHisID(), original.getSubjectHisID());
}

//=============================================================================================================

void TestMneAnonymize::testCommandLineConfiguration()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString inputFile = QCoreApplication::applicationDirPath()
                              + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    const QString outputFile = temporaryDir.filePath("explicit-output.fif");
    const QStringList arguments{
        "mne_anonymize", "--no-gui", "--in", inputFile, "--out", outputFile,
        "--verbose", "--silent", "--brute", "--delete_input_file_after",
        "--avoid_delete_confirmation", "--measurement_date", "05042001",
        "--subject_birthday", "03021980", "--his", "subject-007", "--mne_environment"
    };

    TestSettingsControllerCl controller(arguments);
    QCOMPARE(controller.getQFiInFile().absoluteFilePath(), QFileInfo(inputFile).absoluteFilePath());
    QCOMPARE(controller.getQFiOutFile().absoluteFilePath(), QFileInfo(outputFile).absoluteFilePath());
    QVERIFY(controller.deleteInputFileAfter());
    QVERIFY(!controller.deleteInputFileConfirmation());
    QVERIFY(!controller.anonymizer()->getVerboseMode());
    QVERIFY(controller.anonymizer()->getBruteMode());
    QVERIFY(controller.anonymizer()->getMNEEnvironmentMode());
    QCOMPARE(controller.anonymizer()->getMeasurementDate().date(), QDate(2001, 4, 5));
    QCOMPARE(controller.anonymizer()->getSubjectBirthday(), QDate(1980, 2, 3));
    QCOMPARE(controller.anonymizer()->getSubjectHisID(), QString("subject-007"));

    const QRegularExpression randomNamePattern("^mne_anonymize_[a-l1-9]{12}\\.fif$");
    QVERIFY(randomNamePattern.match(controller.randomFileName()).hasMatch());

    TestSettingsControllerCl sameFileController(
        {"mne_anonymize", "--no-gui", "--in", inputFile, "--out", inputFile});
    QVERIFY(sameFileController.getQFiOutFile().absoluteFilePath()
            != sameFileController.getQFiInFile().absoluteFilePath());
    QVERIFY(randomNamePattern.match(sameFileController.getQFiOutFile().fileName()).hasMatch());
}

//=============================================================================================================

void TestMneAnonymize::testCommandLineOffsetsAndInvalidPaths()
{
    const QString inputFile = QCoreApplication::applicationDirPath()
                              + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    TestSettingsControllerCl offsetController(
        {"mne_anonymize", "--no-gui", "--in", inputFile,
         "--measurement_date_offset", "41", "--subject_birthday_offset", "17"});
    QVERIFY(offsetController.anonymizer()->getUseMeasurementDayOffset());
    QCOMPARE(offsetController.anonymizer()->getMeasurementDayOffset(), 41);
    QVERIFY(offsetController.anonymizer()->getUseSubjectBirthdayOffset());
    QCOMPARE(offsetController.anonymizer()->getSubjectBirthdayOffset(), 17);
    QVERIFY(offsetController.getQFiOutFile().fileName().endsWith("_anonymized.fif"));

    TestSettingsControllerCl missingInputController(
        {"mne_anonymize", "--no-gui", "--in", "/path/that/does/not/exist.fif"});
    QVERIFY(!missingInputController.getQFiInFile().isFile());
    QCOMPARE(missingInputController.run(), 1);

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    TestSettingsControllerCl directoryOutputController(
        {"mne_anonymize", "--no-gui", "--in", inputFile, "--out", temporaryDir.path()});
    QVERIFY(directoryOutputController.getQFiOutFile().isDir());

    TestSettingsControllerCl defaultController;
    QVERIFY(defaultController.getQFiInFile().filePath().isEmpty());
    QVERIFY(defaultController.getQFiOutFile().filePath().isEmpty());
    QCOMPARE(defaultController.run(), 1);
}

//=============================================================================================================

void TestMneAnonymize::testDefaultOutput()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------testDefaultOutput-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--no-gui";

    qInfo() << "arguments" << arguments;

    if(QFile::exists(sFileOut)) {
        QFile::remove(sFileOut);
    }

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();
    QVERIFY(QFile::exists(sFileOut));

    QFile::remove(sFileOut);

}

//=============================================================================================================

void TestMneAnonymize::testDeleteInputFile()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/testing0.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/testing0_anonymized.fif");

    qInfo() << "\n\n-------------------------testDeleteInputFile-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--no-gui";
    arguments << "--delete_input_file_after";
    arguments << "--avoid_delete_confirmation";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    QVERIFY(!QFile::exists(sFileInTest));
    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testInPlace()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() +  "/../resources/data/mne-cpp-test-data/MEG/sample/testing1.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/testing1.fif");

    qInfo() << "\n\n-------------------------testInPlace-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--no-gui";
    arguments << "--avoid_delete_confirmation";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();
    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testDefaultAnonymizationOfTags()
{
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------testDefaultAnonymizationOfTags-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--no-gui";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream);

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::compareBirthdayOffsetOption()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------compareBirthdayOffsetOption-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;
    qInfo() << "sFileOut" << sFileOut;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--subject_birthday_offset" << QString::number(mDaysToOffsetDates);
    arguments << "--verbose";
    arguments << "--no-gui";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "SubjBirthdayOffset");

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::compareMeasureDateOffsetOption()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------compareMeasureDateOffsetOption-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;
    qInfo() << "sFileOut" << sFileOut;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--measurement_date_offset" << QString::number(mDaysToOffsetDates);
    arguments << "--verbose";
    arguments << "--no-gui";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    QFile fFileOut(sFileOut);

    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "MeasDateOffset");

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::verifyTags(FIFFLIB::FiffStream::SPtr &stream,
                                  QString testArg)
{

    FiffTag::UPtr pTag = std::make_unique<FiffTag>();

    stream->device()->seek(0);

    while(pTag->next != -1) {
        stream->read_tag(pTag);

        switch (pTag->kind) {
        //all these 'kinds' of tags contain a fileID struct, which contains info related to
        //measurement date
        case FIFF_FILE_ID:
        case FIFF_BLOCK_ID:
        case FIFF_PARENT_FILE_ID:
        case FIFF_PARENT_BLOCK_ID:
        case FIFF_REF_FILE_ID:
        case FIFF_REF_BLOCK_ID:
        {
            FiffId inId = pTag->toFiffID();

            if(testArg != "MeasDateOffset"){
                QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(inId.time.secs, QTimeZone::LocalTime));

                QVERIFY(inMeasDate == mDefaultMeasDate);
                QVERIFY(inId.time.secs == static_cast<int32_t>(mDefaultMeasDate.toSecsSinceEpoch()));
                QVERIFY(inId.time.usecs == 0);
            }

            QVERIFY(inId.machid[0] == 0);
            QVERIFY(inId.machid[1] == 0);

            break;
        }
        case FIFF_MEAS_DATE:
        {
            QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*pTag->toInt(), QTimeZone::LocalTime));
            QDateTime originalDateInFile(QDate(2002,12,3), QTime(14, 1, 10));

            qInfo() << "InMeasDate: " << inMeasDate;
            qInfo() << "originalDateInFile: " << originalDateInFile;
            qInfo() << "days in between: " << inMeasDate.daysTo(originalDateInFile);
            qInfo() << "days used as offset: " << mDaysToOffsetDates;

            if(testArg == "MeasDateOffset"){
                QVERIFY(inMeasDate.daysTo(originalDateInFile) == mDaysToOffsetDates);
            } else {
                QVERIFY(inMeasDate == mDefaultMeasDate);
            }

            break;
        }
        case FIFF_COMMENT:
        {
            if(m_pBlockTypeList->first() == FIFFB_MEAS_INFO) {
                QString defaultComment("mne_anonymize");
                QString anonFiffInfoComment(pTag->data());
                QVERIFY(anonFiffInfoComment == defaultComment);
            }

            break;
        }
        case FIFF_EXPERIMENTER:
        {
            QString defaultComment("mne_anonymize");
            QString anonFiffExperimenter(pTag->data());
            QVERIFY(anonFiffExperimenter == defaultComment);

           break;
        }
        case FIFF_SUBJ_ID:
        {
            fiff_int_t intAnnonSubjId(*pTag->toInt());
            QVERIFY(intAnnonSubjId == 0);

            break;
        }
        case FIFF_SUBJ_FIRST_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonSubjFirstName(pTag->data());
            QVERIFY(anonSubjFirstName == defaultComment);

            break;
        }
        case FIFF_SUBJ_MIDDLE_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonSubjMiddleName(pTag->data());
            QVERIFY(anonSubjMiddleName == defaultComment);

            break;
        }
        case FIFF_SUBJ_LAST_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonSubjLastName(pTag->data());
            QVERIFY(anonSubjLastName == defaultComment);

            break;
        }
        case FIFF_SUBJ_BIRTH_DAY:
        {
            QDate defaultDate(QDate(2000,1,1));
            QDate inBirthday(QDate::fromJulianDay(*pTag->toJulian()));
            QDate offSetBirtday(defaultDate.addDays(-35));

            if(testArg == "SubjBirthdayOffset") {
                QVERIFY(defaultDate == offSetBirtday);
            } else {
                QVERIFY(defaultDate == inBirthday);
            }

            break;
        }
        case FIFF_SUBJ_SEX:
        {
            fiff_int_t intAnonSubjSex(*pTag->toInt());
            QVERIFY(intAnonSubjSex == 0);

            break;
        }
        case FIFF_SUBJ_HAND:
        {
            fiff_int_t intAnonSubjHand(*pTag->toInt());
            QVERIFY(intAnonSubjHand == 0);

            break;
        }
        case FIFF_SUBJ_WEIGHT:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonSubjWeight(*pTag->toInt());
                QVERIFY(intAnonSubjWeight == 0);
            }
            break;
        }
        case FIFF_SUBJ_HEIGHT:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonSubjHeight(*pTag->toInt());
                QVERIFY(intAnonSubjHeight == 0);
            }
            break;
        }
        case FIFF_SUBJ_COMMENT:
        {
            QString defaultComment("mne_anonymize");
            QString anonSubjComment(pTag->data());
            QVERIFY(anonSubjComment == defaultComment);

            break;
        }
        case FIFF_SUBJ_HIS_ID:
        {
            QString defaultComment("mne_anonymize");
            QString anonSubjHis(pTag->data());
            QVERIFY(anonSubjHis == defaultComment);
            break;
        }
        case FIFF_PROJ_ID:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonProjId(*pTag->toInt());
                QVERIFY(intAnonProjId == 0);
            }
            break;
        }
        case FIFF_PROJ_NAME:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString intAnonProjName(pTag->data());
                QVERIFY(intAnonProjName == defaultComment);
            }
            break;
        }
        case FIFF_PROJ_AIM:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString intAnonProjAim(pTag->data());
                QVERIFY(intAnonProjAim == defaultComment);
            }
            break;
        }
        case FIFF_PROJ_PERSONS:
        {
            QString defaultComment("mne_anonymize");
            QString intAnonProjPersons(pTag->data());
            QVERIFY(intAnonProjPersons == defaultComment);

            break;
        }
        case FIFF_PROJ_COMMENT:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString intAnonProjComment(pTag->toString());
                QVERIFY(intAnonProjComment == defaultComment);
            }
            break;
        }
//        case FIFF_MRI_PIXEL_DATA:
//        {
//            if(!m_bQuietMode) {
//                qDebug() << " ";
//                qDebug() << "WARNING. The input fif file contains MRI data.";
//                qDebug() << "Beware that a subject''s face can be reconstructed from it";
//                qDebug() << "This software can not anonymize MRI data, at the moment.";
//                qDebug() << "Contanct the authors for more information.";
//                qDebug() << " ";
//            }
//            break;
//        }
        default:{
        }
        }
    }
}

//=============================================================================================================

void TestMneAnonymize::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneAnonymize)
#include "test_mne_anonymize.moc"
