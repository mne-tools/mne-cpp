
//=============================================================================================================
/**
* @file     test_mne_anonymize.cpp
* @author   Lorenz Esch <lorenzesch@hotmail.com>;
* @version  1.0
* @date     September, 2019
*
* @section  LICENSE
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

#include "../applications/mne_anonymize/fiffanonymizer.h"
#include "../applications/mne_anonymize/settingscontroller.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNEANONYMIZE;

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
    void testDefaultOutput();
    void testMultipleInputs();
    void testDeleteInputFile();
    void testInplace();
    void testInplaceAndDeleteInFile();

    //test anonymization
    void testDefaultAnonymizationOfTags();
    void compareBirthdayOffsetOption();
    void compareMeasureDateOffsetOption();
    void cleanupTestCase();

private:
    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;

    void verifyTags(FIFFLIB::FiffStream::SPtr &outStream,
                    QString testArg="blank");
};

//=============================================================================================================

TestMneAnonymize::TestMneAnonymize()
{
}

//=============================================================================================================

void TestMneAnonymize::initTestCase()
{
}

//=============================================================================================================

void TestMneAnonymize::testDefaultOutput()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------testDefaultOutput-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;

    qInfo() << "arguments" << arguments;

    if(QFile::exists(sFileOut)) {
        QFile::remove(sFileOut);
    }

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");
    QVERIFY(QFile::exists(sFileOut));

    QFile::remove(sFileOut);
}

//=============================================================================================================

void TestMneAnonymize::testMultipleInputs()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInA(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_A_raw.fif");
    QString sFileInB(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_B_raw.fif");

    qInfo() << "\n\n-------------------------testMultipleInputs-------------------------------------";

    QFile::copy(sFileIn,sFileInA);
    QFile::copy(sFileIn,sFileInB);

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--in" << sFileInA;
    arguments << "--in" << sFileInB;

    qInfo() << "arguments" << arguments;

    // Delete anoymized file from other test runs
    if(QFile::exists(QCoreApplication::applicationDirPath() + QString("/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif"))) {
        QFile::remove(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");
    }

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");

    QVERIFY(QFile::exists(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif"));
    QVERIFY(QFile::exists(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_A_raw_anonymized.fif"));
    QVERIFY(QFile::exists(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_B_raw_anonymized.fif"));

    QFile::remove(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");
    QFile::remove(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_A_raw_anonymized.fif");
    QFile::remove(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_B_raw_anonymized.fif");
}

//=============================================================================================================

void TestMneAnonymize::testDeleteInputFile()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing0.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing0_anonymized.fif");

    qInfo() << "\n\n-------------------------testDeleteInputFile-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--delete_input_file_after";
    arguments << "--avoid_delete_confirmation";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "dev");

    QVERIFY(!QFile::exists(sFileInTest));
    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testInplace()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing1.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing1.fif");

    qInfo() << "\n\n-------------------------testInOutSameName-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--avoid_delete_confirmation";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "dev");

    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testInplaceAndDeleteInFile()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing2.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing2.fif");

    qInfo() << "\n\n-------------------------testInOutSameNameAndDeleteInFile-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "--avoid_delete_confirmation";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");

    QVERIFY(QFile::exists(sFileOutTest));

    QFile::remove(sFileOutTest);
}

//=============================================================================================================

void TestMneAnonymize::testDefaultAnonymizationOfTags()
{
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------testDefaultAnonymizationOfTags-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream);
}

//=============================================================================================================

void TestMneAnonymize::compareBirthdayOffsetOption()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------compareBirthdayOffsetOption-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;
    qInfo() << "sFileOut" << sFileOut;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--subject_birthday_offset" << "35";
    arguments << "--verbose";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "SubjBirthdayOffset");
}

//=============================================================================================================

void TestMneAnonymize::compareMeasureDateOffsetOption()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

    qInfo() << "\n\n-------------------------compareMeasureDateOffsetOption-------------------------------------";
    qInfo() << "sFileIn" << sFileIn;
    qInfo() << "sFileOut" << sFileOut;

    QStringList arguments;
    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--measurement_date_offset" << "35";
    arguments << "--verbose";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "dev");

    QFile fFileOut(sFileOut);

    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "MeasDateOffset");
}

//=============================================================================================================

void TestMneAnonymize::verifyTags(FIFFLIB::FiffStream::SPtr &stream,
                                  QString testArg)
{

    FiffTag::SPtr pTag = FiffTag::SPtr::create();

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
                QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(inId.time.secs, Qt::UTC));
                QDateTime defaultMeasDate(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC);

                QVERIFY(inMeasDate == defaultMeasDate);
                QVERIFY(inId.time.secs == static_cast<int32_t>(defaultMeasDate.toSecsSinceEpoch()));
                QVERIFY(inId.time.usecs == 0);
            }

            QVERIFY(inId.machid[0] == 0);
            QVERIFY(inId.machid[1] == 0);

            break;
        }
        case FIFF_MEAS_DATE:
        {
            QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*pTag->toInt(), Qt::UTC));
            QDateTime defaultMeasDate(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC);
            QDateTime actualDate(QDate(2002,12,3), QTime(19, 1, 10), Qt::UTC);
            QDateTime offSetMeasDate(actualDate.addDays(-35));

            if(testArg == "MeasDateOffset"){
                QVERIFY(inMeasDate == offSetMeasDate);
            } else {
                QVERIFY(inMeasDate == defaultMeasDate);
            }

            break;
        }
        case FIFF_COMMENT:
        {
            if(m_pBlockTypeList->first()==FIFFB_MEAS_INFO) {
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
            QDateTime defaultDate(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC);
            QDateTime inBirthday(QDate::fromJulianDay(*pTag->toJulian()));
            QDateTime offSetBirtday(defaultDate.date().addDays(-35));

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
                fiff_int_t intAnonSubjWeight(*pTag.data()->toInt());
                QVERIFY(intAnonSubjWeight == 0);
            }
            break;
        }
        case FIFF_SUBJ_HEIGHT:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonSubjHeight(*pTag.data()->toInt());
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
                fiff_int_t intAnonProjId(*pTag.data()->toInt());
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
                QString intAnonProjComment(pTag.data()->toString());
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
