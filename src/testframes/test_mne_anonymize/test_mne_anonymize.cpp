//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     test_mne_anonymize.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2019
 * @brief    Test for anonymizing a fiff raw file
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../tools/preprocessing/mne_anonymize/fiffanonymizer.h"
#include "../../tools/preprocessing/mne_anonymize/settingscontrollercl.h"
#include <base/log.h>

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

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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

namespace
{

QString sampleRawFile()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

// Outputs land next to the input by default, so work on a copy to keep the test data read-only.
QString copyInputTo(const QTemporaryDir& dir)
{
    const QString copy = dir.filePath("sample_audvis_trunc_raw.fif");
    if (!dir.isValid() || !QFile::copy(sampleRawFile(), copy)) {
        return QString();
    }
    return copy;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestMneAnonymize
 *
 * @brief The TestMneAnonymize class provides fiff anonymizing verification tests
 *
 */
class TestMneAnonymize : public QObject
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
    void testNonAsciiHisId();

    //test anonymization
    void testDefaultAnonymizationOfTags();
    void compareBirthdayOffsetOption();
    void compareMeasureDateOffsetOption();
    void cleanupTestCase();

private:
    QSharedPointer<QStack<int32_t>> m_pBlockTypeList;

    void verifyTags(mne::FiffStream& outStream,
                    QString testArg = "blank");
    int mDaysToOffsetDates;
    QDateTime mDefaultMeasDate;
};

//=============================================================================================================

TestMneAnonymize::TestMneAnonymize()
: mDaysToOffsetDates(35)
, mDefaultMeasDate(QDate(2000, 1, 1), QTime(1, 1, 0), QTimeZone::LocalTime)
{
}

//=============================================================================================================

void TestMneAnonymize::initTestCase()
{
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

void TestMneAnonymize::testNonAsciiHisId()
{
    // A HIS ID with characters outside ASCII is written whole, as UTF-8
    QTemporaryDir tempDir;
    const QString fileIn = tempDir.filePath("his_id.fif");
    const QString fileOut = tempDir.filePath("his_id_anonymized.fif");
    {
        const std::unique_ptr<FiffStream> stream = FiffStream::start_file(fileIn.toStdU16String());
        QVERIFY(stream);
        stream->start_block(FIFFB_MEAS);
        stream->start_block(FIFFB_SUBJECT);
        stream->write_string(FIFF_SUBJ_HIS_ID, "patient-1");
        stream->end_block(FIFFB_SUBJECT);
        stream->end_block(FIFFB_MEAS);
        stream->end_file();
    }
    const QString hisId = QStringLiteral("Jörg-Müller-ß");
    FiffAnonymizer anonymizer;
    anonymizer.setSubjectHisId(hisId);
    QCOMPARE(anonymizer.setInFile(fileIn), 0);
    QCOMPARE(anonymizer.setOutFile(fileOut), 0);
    QCOMPARE(anonymizer.anonymizeFile(), 0);

    FiffStream stream(fileOut.toStdU16String());
    QVERIFY(stream.open());
    std::optional<FiffTag> tag;
    while ((tag = stream.read_tag()) && tag->kind != FIFF_SUBJ_HIS_ID) {
    }
    QVERIFY(tag);
    QCOMPARE(QString::fromStdString(tag->to_string()), hisId);
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
    const QString inputFile = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    const QString outputFile = temporaryDir.filePath("explicit-output.fif");
    const QStringList arguments{
        "mne_anonymize", "--no-gui", "--in", inputFile, "--out", outputFile,
        "--verbose", "--silent", "--brute", "--delete_input_file_after",
        "--avoid_delete_confirmation", "--measurement_date", "05042001",
        "--subject_birthday", "03021980", "--his", "subject-007", "--mne_environment"};

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
    QVERIFY(sameFileController.getQFiOutFile().absoluteFilePath() != sameFileController.getQFiInFile().absoluteFilePath());
    QVERIFY(randomNamePattern.match(sameFileController.getQFiOutFile().fileName()).hasMatch());
}

//=============================================================================================================

void TestMneAnonymize::testCommandLineOffsetsAndInvalidPaths()
{
    const QString inputFile = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
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
    QTemporaryDir tempDir;
    const QString sFileIn(copyInputTo(tempDir));
    QVERIFY(!sFileIn.isEmpty());
    QString sFileOut(tempDir.filePath("sample_audvis_trunc_raw_anonymized.fif"));

    Log::info("\n\n-------------------------testDefaultOutput-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--no-gui";

    Log::info("arguments {}", arguments.join(", ").toStdString());

    if (QFile::exists(sFileOut)) {
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
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sFileIn(sampleRawFile());
    QString sFileInTest(tempDir.filePath("testing0.fif"));
    QString sFileOutTest(tempDir.filePath("testing0_anonymized.fif"));

    Log::info("\n\n-------------------------testDeleteInputFile-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());

    QFile::copy(sFileIn, sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--no-gui";
    arguments << "--delete_input_file_after";
    arguments << "--avoid_delete_confirmation";

    Log::info("arguments {}", arguments.join(", ").toStdString());

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
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sFileIn(sampleRawFile());
    QString sFileInTest(tempDir.filePath("testing1.fif"));
    QString sFileOutTest(tempDir.filePath("testing1.fif"));

    Log::info("\n\n-------------------------testInPlace-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());

    QFile::copy(sFileIn, sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--no-gui";
    arguments << "--avoid_delete_confirmation";

    Log::info("arguments {}", arguments.join(", ").toStdString());

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();
    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testDefaultAnonymizationOfTags()
{
    QTemporaryDir tempDir;
    const QString sFileIn(copyInputTo(tempDir));
    QVERIFY(!sFileIn.isEmpty());
    QString sFileOut(tempDir.filePath("sample_audvis_trunc_raw_anonymized.fif"));

    Log::info("\n\n-------------------------testDefaultAnonymizationOfTags-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--no-gui";

    Log::info("arguments {}", arguments.join(", ").toStdString());

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    auto outStream = std::make_unique<FiffStream>(sFileOut.toStdU16String());
    if (outStream->open()) {
        Log::info("output file opened correctly {}", sFileIn.toStdString());
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(*outStream);

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::compareBirthdayOffsetOption()
{
    // Init testing arguments
    QTemporaryDir tempDir;
    const QString sFileIn(copyInputTo(tempDir));
    QVERIFY(!sFileIn.isEmpty());
    QString sFileOut(tempDir.filePath("sample_audvis_trunc_raw_anonymized.fif"));

    Log::info("\n\n-------------------------compareBirthdayOffsetOption-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());
    Log::info("sFileOut {}", sFileOut.toStdString());

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--subject_birthday_offset" << QString::number(mDaysToOffsetDates);
    arguments << "--verbose";
    arguments << "--no-gui";

    Log::info("arguments {}", arguments.join(", ").toStdString());

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();

    FiffStream outStream(sFileOut.toStdU16String());
    if (outStream.open()) {
        Log::info("output file opened correctly {}", sFileIn.toStdString());
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
    QTemporaryDir tempDir;
    const QString sFileIn(copyInputTo(tempDir));
    QVERIFY(!sFileIn.isEmpty());
    QString sFileOut(tempDir.filePath("sample_audvis_trunc_raw_anonymized.fif"));

    Log::info("\n\n-------------------------compareMeasureDateOffsetOption-------------------------------------");
    Log::info("sFileIn {}", sFileIn.toStdString());
    Log::info("sFileOut {}", sFileOut.toStdString());

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--measurement_date_offset" << QString::number(mDaysToOffsetDates);
    arguments << "--verbose";
    arguments << "--no-gui";

    Log::info("arguments {}", arguments.join(", ").toStdString());

    MNEANONYMIZE::SettingsControllerCl controller(arguments);
    controller.run();


    FiffStream outStream(sFileOut.toStdU16String());
    if (outStream.open()) {
        Log::info("output file opened correctly {}", sFileIn.toStdString());
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "MeasDateOffset");

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::verifyTags(mne::FiffStream& stream,
                                  QString testArg)
{
    FiffTag pTag;

    stream.byte_stream().seek(0);

    while (pTag.next != -1) {
        auto read = stream.read_tag();
        if (!read)
            break;
        pTag = std::move(*read);

        switch (pTag.kind) {
            //all these 'kinds' of tags contain a fileID struct, which contains info related to
            //measurement date
            case FIFF_FILE_ID:
            case FIFF_BLOCK_ID:
            case FIFF_PARENT_FILE_ID:
            case FIFF_PARENT_BLOCK_ID:
            case FIFF_REF_FILE_ID:
            case FIFF_REF_BLOCK_ID: {
                FiffId inId = pTag.to_fiff_id();

                if (testArg != "MeasDateOffset") {
                    QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(inId.time.secs, QTimeZone::LocalTime));

                    QVERIFY(inMeasDate == mDefaultMeasDate);
                    QVERIFY(inId.time.secs == static_cast<int32_t>(mDefaultMeasDate.toSecsSinceEpoch()));
                    QVERIFY(inId.time.usecs == 0);
                }

                QVERIFY(inId.machid[0] == 0);
                QVERIFY(inId.machid[1] == 0);

                break;
            }
            case FIFF_MEAS_DATE: {
                QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*pTag.to_int(), QTimeZone::LocalTime));
                QDateTime originalDateInFile(QDate(2002, 12, 3), QTime(14, 1, 10));

                Log::info("InMeasDate: {}", inMeasDate.toString().toStdString());
                Log::info("originalDateInFile: {}", originalDateInFile.toString().toStdString());
                Log::info("days in between: {}", inMeasDate.daysTo(originalDateInFile));
                Log::info("days used as offset: {}", mDaysToOffsetDates);

                if (testArg == "MeasDateOffset") {
                    QVERIFY(inMeasDate.daysTo(originalDateInFile) == mDaysToOffsetDates);
                } else {
                    QVERIFY(inMeasDate == mDefaultMeasDate);
                }

                break;
            }
            case FIFF_COMMENT: {
                if (m_pBlockTypeList->first() == FIFFB_MEAS_INFO) {
                    QString defaultComment("mne_anonymize");
                    QString anonFiffInfoComment = QString::fromStdString(pTag.to_string());
                    QVERIFY(anonFiffInfoComment == defaultComment);
                }

                break;
            }
            case FIFF_EXPERIMENTER: {
                QString defaultComment("mne_anonymize");
                QString anonFiffExperimenter = QString::fromStdString(pTag.to_string());
                QVERIFY(anonFiffExperimenter == defaultComment);

                break;
            }
            case FIFF_SUBJ_ID: {
                fiff_int_t intAnnonSubjId(*pTag.to_int());
                QVERIFY(intAnnonSubjId == 0);

                break;
            }
            case FIFF_SUBJ_FIRST_NAME: {
                QString defaultComment("mne_anonymize");
                QString anonSubjFirstName = QString::fromStdString(pTag.to_string());
                QVERIFY(anonSubjFirstName == defaultComment);

                break;
            }
            case FIFF_SUBJ_MIDDLE_NAME: {
                QString defaultComment("mne_anonymize");
                QString anonSubjMiddleName = QString::fromStdString(pTag.to_string());
                QVERIFY(anonSubjMiddleName == defaultComment);

                break;
            }
            case FIFF_SUBJ_LAST_NAME: {
                QString defaultComment("mne_anonymize");
                QString anonSubjLastName = QString::fromStdString(pTag.to_string());
                QVERIFY(anonSubjLastName == defaultComment);

                break;
            }
            case FIFF_SUBJ_BIRTH_DAY: {
                QDate defaultDate(QDate(2000, 1, 1));
                QDate inBirthday(QDate::fromJulianDay(*pTag.to_julian()));
                QDate offSetBirtday(defaultDate.addDays(-35));

                if (testArg == "SubjBirthdayOffset") {
                    QVERIFY(defaultDate == offSetBirtday);
                } else {
                    QVERIFY(defaultDate == inBirthday);
                }

                break;
            }
            case FIFF_SUBJ_SEX: {
                fiff_int_t intAnonSubjSex(*pTag.to_int());
                QVERIFY(intAnonSubjSex == 0);

                break;
            }
            case FIFF_SUBJ_HAND: {
                fiff_int_t intAnonSubjHand(*pTag.to_int());
                QVERIFY(intAnonSubjHand == 0);

                break;
            }
            case FIFF_SUBJ_WEIGHT: {
                if (testArg == "BruteMode") {
                    fiff_int_t intAnonSubjWeight(*pTag.to_int());
                    QVERIFY(intAnonSubjWeight == 0);
                }
                break;
            }
            case FIFF_SUBJ_HEIGHT: {
                if (testArg == "BruteMode") {
                    fiff_int_t intAnonSubjHeight(*pTag.to_int());
                    QVERIFY(intAnonSubjHeight == 0);
                }
                break;
            }
            case FIFF_SUBJ_COMMENT: {
                QString defaultComment("mne_anonymize");
                QString anonSubjComment = QString::fromStdString(pTag.to_string());
                QVERIFY(anonSubjComment == defaultComment);

                break;
            }
            case FIFF_SUBJ_HIS_ID: {
                QString defaultComment("mne_anonymize");
                QString anonSubjHis = QString::fromStdString(pTag.to_string());
                QVERIFY(anonSubjHis == defaultComment);
                break;
            }
            case FIFF_PROJ_ID: {
                if (testArg == "BruteMode") {
                    fiff_int_t intAnonProjId(*pTag.to_int());
                    QVERIFY(intAnonProjId == 0);
                }
                break;
            }
            case FIFF_PROJ_NAME: {
                if (testArg == "BruteMode") {
                    QString defaultComment("mne_anonymize");
                    QString intAnonProjName = QString::fromStdString(pTag.to_string());
                    QVERIFY(intAnonProjName == defaultComment);
                }
                break;
            }
            case FIFF_PROJ_AIM: {
                if (testArg == "BruteMode") {
                    QString defaultComment("mne_anonymize");
                    QString intAnonProjAim = QString::fromStdString(pTag.to_string());
                    QVERIFY(intAnonProjAim == defaultComment);
                }
                break;
            }
            case FIFF_PROJ_PERSONS: {
                QString defaultComment("mne_anonymize");
                QString intAnonProjPersons = QString::fromStdString(pTag.to_string());
                QVERIFY(intAnonProjPersons == defaultComment);

                break;
            }
            case FIFF_PROJ_COMMENT: {
                if (testArg == "BruteMode") {
                    QString defaultComment("mne_anonymize");
                    QString intAnonProjComment(QString::fromStdString(pTag.to_string()));
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
            default: {
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
