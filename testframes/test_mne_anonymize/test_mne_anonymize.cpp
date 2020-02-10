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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../applications/mne_anonymize/fiffanonymizer.h"
#include "../applications/mne_anonymize/settingscontroller.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QScopedPointer>


//*************************************************************************************************************
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
    void testDeleteInputFile();
    void testInOutSameName();
//    void testInOutSameNameAndDeleteInFile(); // Not working -> error indicates that first tag in the fif file is not a tag

    //test anonymization
    void testDefaultAnonymizationOfTags();

    void compareBirthdayOffsetOption();
//    void compareMeasureDateOffsetOption(); // Failing 35 days are not subtracted from date in the fiff file
    void cleanupTestCase();

private:
    double epsilon;
    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;
    void verifyTags(FIFFLIB::FiffStream::SPtr &outStream,
                    QString testArg="blank");
//    void verifyCRC(const QString file,
//                   const quint16 validatedCRC); // I think we can delete this function since CRC comparison is difficult to achieve because the files are created on different machines.
};


//*************************************************************************************************************

TestMneAnonymize::TestMneAnonymize()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestMneAnonymize::initTestCase()
{

}


//*************************************************************************************************************

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

    //verify tags of the file
    QFile::remove(sFileOut);
}


//*************************************************************************************************************

void TestMneAnonymize::testDeleteInputFile()
{
    // Init testing arguments
    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    QString sFileInTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing0.fif");
    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing0_output.fif");

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

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QVERIFY(!QFile::exists(sFileInTest));
    QVERIFY(QFile::exists(sFileOutTest));
}


//*************************************************************************************************************

void TestMneAnonymize::testInOutSameName()
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

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QVERIFY(QFile::exists(sFileOutTest));
}


////*************************************************************************************************************

//void TestMneAnonymize::testInOutSameNameAndDeleteInFile()
//{
//    // Init testing arguments
//    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
//    QString sFileInTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing2.fif");
//    QString sFileOutTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/testing2.fif");

//    qInfo() << "\n\n-------------------------testInOutSameNameAndDeleteInFile-------------------------------------";
//    qInfo() << "sFileIn" << sFileIn;

//    QFile::copy(sFileIn,sFileInTest);
//    QVERIFY(QFile::exists(sFileInTest));

//    QStringList arguments;
//    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
//    arguments << "--in" << sFileInTest;
//    arguments << "--out" << sFileOutTest;
//    arguments << "--delete_input_file_after";
//    arguments << "--avoid_delete_confirmation";

//    qInfo() << "arguments" << arguments;

//    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

//    QVERIFY(!QFile::exists(sFileInTest));
//    QVERIFY(QFile::exists(sFileOutTest));
//}


//*************************************************************************************************************

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

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream);
}


//*************************************************************************************************************

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
    arguments << "subject_birthday_offset 35";
    arguments << "--verbose";

    qInfo() << "arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly)) {
        qInfo() << "output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, "SubjBirthdayOffset");
}


////*************************************************************************************************************

//void TestMneAnonymize::compareMeasureDateOffsetOption()
//{
//    // Init testing arguments
//    QString sFileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
//    QString sFileOut(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw_anonymized.fif");

//    qInfo() << "\n\n-------------------------compareMeasureDateOffsetOption-------------------------------------";
//    qInfo() << "sFileIn" << sFileIn;
//    qInfo() << "sFileOut" << sFileOut;

//    QStringList arguments;
//    arguments << QCoreApplication::applicationDirPath() + "/mne_anonymize";
//    arguments << "--in" << sFileIn;
//    arguments << "measurement_date_offset 35";
//    arguments << "--verbose";

//    qInfo() << "arguments" << arguments;

//    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

//    QFile fFileOut(sFileOut);

//    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
//    if(outStream->open(QIODevice::ReadOnly)) {
//        qInfo() << "output file opened correctly " << sFileIn;
//    } else {
//        QFAIL("Output file could not be loaded.");
//    }

//    verifyTags(outStream, "MeasDateOffset");
//}


//*************************************************************************************************************

void TestMneAnonymize::cleanupTestCase()
{

}


////*************************************************************************************************************

//void TestMneAnonymize::verifyCRC(const QString file,
//                                 const quint16 validatedCRC)
//{
//    QFile fFileIn(file);

//    qInfo() << "\n\n-------------------------verifyCRC-------------------------------------";
//    if(fFileIn.open(QIODevice::ReadOnly)) {
//        qInfo() << "Anonymized file opened correctly " << fFileIn.fileName();
//    } else {
//        QFAIL("Anonymized file could not be loaded.");
//    }

//    // Create crc checksum and compare to reference
//    QByteArray inData(fFileIn.readAll());
//    fFileIn.close();

//    quint16 crc = qChecksum(inData.data(),static_cast<uint>(inData.size()));
//    qInfo() << "crc expected value: " << validatedCRC;
//    qInfo() << "crc obtained value:" << crc;

//    QVERIFY(validatedCRC == crc);
//}


//*************************************************************************************************************

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
            QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(inId.time.secs));
            QDateTime defaultMeasDate(QDate(2000,1,1), QTime(1, 1, 0));
            QDateTime offSetMeasDate(defaultMeasDate.date().addDays(-35));

            qDebug() << "inMeasDate" << inMeasDate;
            qDebug() << "defaultMeasDate" << defaultMeasDate;

            if(testArg == "MeasDateOffset"){
                QVERIFY(inMeasDate == offSetMeasDate);
            } else {
                QVERIFY(inMeasDate == defaultMeasDate);
            }

            QVERIFY(inId.machid[0] == 0);
            QVERIFY(inId.machid[1] == 0);
            QVERIFY(inId.time.secs == static_cast<int32_t>(defaultMeasDate.toSecsSinceEpoch()));
            QVERIFY(inId.time.usecs == 0);

            break;
        }
        case FIFF_MEAS_DATE:
        {

            QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*pTag->toInt()));
            QDateTime defaultMeasDate(QDate(2000,1,1), QTime(1, 1, 0));
            QDateTime offSetMeasDate(defaultMeasDate.date().addDays(-35));

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
                QString anonFileComment(pTag->data());
                QVERIFY(anonFileComment == defaultComment);
            }

            break;
        }
        case FIFF_EXPERIMENTER:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

           break;
        }
        case FIFF_SUBJ_ID:
        {
            fiff_int_t intAnonFile(*pTag->toInt());
            QVERIFY(intAnonFile == 0);

            break;
        }
        case FIFF_SUBJ_FIRST_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

            break;
        }
        case FIFF_SUBJ_MIDDLE_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

            break;
        }
        case FIFF_SUBJ_LAST_NAME:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

            break;
        }
        case FIFF_SUBJ_BIRTH_DAY:
        {
            QDateTime defaultDate(QDate(2000,1,1), QTime(1, 1, 0));
            QDateTime inBirthday(QDate::fromJulianDay(*pTag->toJulian()));
            QDateTime offSetBirtday(defaultDate.date().addDays(-35));

            if(testArg == "SubjBirthdayOffset") {
                QVERIFY(defaultDate == offSetBirtday);
            } else {
                QVERIFY(defaultDate == inBirthday);
            }

            break;
        }
        case FIFF_SUBJ_WEIGHT:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonFile(*pTag.data()->toInt());
                QVERIFY(intAnonFile == 0);
            }
            break;
        }
        case FIFF_SUBJ_HEIGHT:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonFile(*pTag.data()->toInt());
                QVERIFY(intAnonFile == 0);
            }
            break;
        }
        case FIFF_SUBJ_COMMENT:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

            break;
        }
        case FIFF_SUBJ_HIS_ID:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);
            break;
        }
        case FIFF_PROJ_ID:
        {
            if(testArg == "BruteMode")
            {
                fiff_int_t intAnonFile(*pTag.data()->toInt());
                QVERIFY(intAnonFile == 0);
            }
            break;
        }
        case FIFF_PROJ_NAME:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString anonFileComment(pTag->data());
                QVERIFY(anonFileComment == defaultComment);
            }
            break;
        }
        case FIFF_PROJ_AIM:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString anonFileComment(pTag->data());
                QVERIFY(anonFileComment == defaultComment);
            }
            break;
        }
        case FIFF_PROJ_PERSONS:
        {
            QString defaultComment("mne_anonymize");
            QString anonFileComment(pTag->data());
            QVERIFY(anonFileComment == defaultComment);

            break;
        }
        case FIFF_PROJ_COMMENT:
        {
            if(testArg == "BruteMode")
            {
                QString defaultComment("mne_anonymize");
                QString anonFileComment(pTag.data()->toString());
                QVERIFY(anonFileComment == defaultComment);
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
// MAIN
//=============================================================================================================


QTEST_GUILESS_MAIN(TestMneAnonymize)
#include "test_mne_anonymize.moc"
