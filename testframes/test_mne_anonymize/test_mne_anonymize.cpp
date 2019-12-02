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
    void testDefaultWildcard();
    void testDeleteInputFile();
    void testInOutSameName();
    void testInOutSameNameAndDeleteInFile();

    //test anonymization
    void testDefaultAnonymizationOfTags();

    void compareBirthdayOffsetOption();
    void compareMeasureDateOffsetOption();
    void cleanupTestCase();

private:
    double epsilon;
    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;
    void verifyTags(FIFFLIB::FiffStream::SPtr &outStream,
                    bool SubjBirthdayOffset=false,
                    bool MeasDateOffset=false,
                    bool BruteMode=false);
    void verifyCRC(const QString file,
                   const quint16 validatedCRC);
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
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestMneAnonymize::initTestCase - sFileIn" << sFileIn;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    if(QFile::exists(sFileOut))
    {
        QFile::remove(sFileOut);
    }

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");
    QVERIFY(QFile::exists(sFileOut));
    //verify tags of the file
    QFile::remove(sFileOut);
}


//*************************************************************************************************************

void TestMneAnonymize::testDefaultWildcard()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/*.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestMneAnonymize::initTestCase - sFileIn" << sFileIn;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QStringList listOfFiles = MNEANONYMIZE::listFilesMatchingPatternName(sFileIn);
    for(QString fin: listOfFiles)
    {
        QString fout(fin.replace(fin.size()-4,4,"_anonymized.fif"));
        QVERIFY(QFile::exists(fout));
        //for each file verify tags.
        QFile::remove(fout);
    }
}



//*************************************************************************************************************

void TestMneAnonymize::testDeleteInputFile()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileInTest("./mne-cpp-test-data/MEG/sample/testing.fif");
    QString sFileOutTest("./mne-cpp-test-data/MEG/sample/testing_filename_output.fif");

    qInfo() << "TestMneAnonymize::testDeleteInputFile - sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "delete_input_file_after";
    arguments << "avoid_delete_confirmation";

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QVERIFY(!QFile::exists(sFileInTest )); //delete input file, remember?
    QVERIFY( QFile::exists(sFileOutTest));

    verifyCRC(sFileOutTest,17542);
}


//*************************************************************************************************************

void TestMneAnonymize::testInOutSameName()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileInTest("./mne-cpp-test-data/MEG/sample/testing.fif");
    QString sFileOutTest("./mne-cpp-test-data/MEG/sample/testing.fif");

    qInfo() << "TestMneAnonymize::testInOutSameName - sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;

    qInfo() << "TestMneAnonymize::testInOutSameName - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QVERIFY( QFile::exists(sFileOutTest));

//    verifyCRC(sFileOutTest,17542);
}


//*************************************************************************************************************

void TestMneAnonymize::testInOutSameNameAndDeleteInFile()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileInTest("./mne-cpp-test-data/MEG/sample/testing.fif");
    QString sFileOutTest("./mne-cpp-test-data/MEG/sample/testing.fif");

    qInfo() << "TestMneAnonymize::testInOutSameNameAndDelete - sFileIn" << sFileIn;

    QFile::copy(sFileIn,sFileInTest);
    QVERIFY(QFile::exists(sFileInTest));

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileInTest;
    arguments << "--out" << sFileOutTest;
    arguments << "delete_input_file_after";
    arguments << "avoid_delete_confirmation";

    qInfo() << "TestMneAnonymize::testInOutSameNameAndDelete - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    QVERIFY( QFile::exists(sFileOutTest));

//    verifyCRC(sFileOutTest,17542);
}


//*************************************************************************************************************

void TestMneAnonymize::testDefaultAnonymizationOfTags()
{
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");
    qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - sFileIn" << sFileIn;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;

    qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly))
    {
        qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream);



}


//*************************************************************************************************************

void TestMneAnonymize::compareBirthdayOffsetOption()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestMneAnonymize::initTestCase - sFileIn" << sFileIn;
    qInfo() << "TestMneAnonymize::initTestCase - sFileOut" << sFileOut;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "subject_birthday_offset 35";
    arguments << "--verbose";

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly))
    {
        qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, true);

}

//*************************************************************************************************************

void TestMneAnonymize::compareMeasureDateOffsetOption()
{
    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestMneAnonymize::initTestCase - sFileIn" << sFileIn;
    qInfo() << "TestMneAnonymize::initTestCase - sFileOut" << sFileOut;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--measurement_date_offset 35";
    arguments << "--verbose";

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

    QFile fFileOut(sFileOut);
    FiffStream::SPtr outStream(new FiffStream(&fFileOut));
    if(outStream->open(QIODevice::ReadOnly))
    {
        qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    verifyTags(outStream, false, true);

}

//*************************************************************************************************************

void TestMneAnonymize::cleanupTestCase()
{

}


//*************************************************************************************************************

void TestMneAnonymize::verifyCRC(const QString file,
                                 const quint16 validatedCRC)
{
    QFile fFileIn(file);

    if(fFileIn.open(QIODevice::ReadOnly)) {
        qInfo() << "TestMneAnonymize::verifyCRC - Anonymized file opened correctly " << fFileIn.fileName();
    } else {
        QFAIL("Anonymized file could not be loaded.");
    }

    // Create crc checksum and compare to reference
    QByteArray inData(fFileIn.readAll());
    fFileIn.close();

    quint16 crc = qChecksum(inData.data(),static_cast<uint>(inData.size()));
    qInfo() << "TestMneAnonymize::compareData - crc expected value: " << validatedCRC;
    qInfo() << "TestMneAnonymize::compareData - crc obtained value:" << crc;

    QVERIFY(validatedCRC == crc);
}


//*************************************************************************************************************

void TestMneAnonymize::verifyTags(FiffStream::SPtr &stream,
                                  bool SubjBirthdayOffset,
                                  bool MeasDateOffset,
                                  bool BruteMode)
{

    FiffTag::SPtr pTag = FiffTag::SPtr::create();

    stream->device()->seek(0);

    do
    {
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
            if(MeasDateOffset){
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
            if(MeasDateOffset){
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
            if(SubjBirthdayOffset) {
                QVERIFY(defaultDate == offSetBirtday);
            } else {
                QVERIFY(defaultDate == inBirthday);
            }


            break;
        }
        case FIFF_SUBJ_WEIGHT:
        {

            if(BruteMode)
            {
                fiff_int_t intAnonFile(*pTag.data()->toInt());
                QVERIFY(intAnonFile == 0);
            }
            break;
        }
        case FIFF_SUBJ_HEIGHT:
        {
            if(BruteMode)
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

            if(BruteMode)
            {
                fiff_int_t intAnonFile(*pTag.data()->toInt());
                QVERIFY(intAnonFile == 0);
            }
            break;
        }
        case FIFF_PROJ_NAME:
        {

            if(BruteMode)
            {
                QString defaultComment("mne_anonymize");
                QString anonFileComment(pTag->data());
                QVERIFY(anonFileComment == defaultComment);
            }
            break;
        }
        case FIFF_PROJ_AIM:
        {

            if(BruteMode)
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
            if(BruteMode)
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
        default:
        {
        }

        }

    }while(pTag->next != -1);
}


//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestMneAnonymize)
#include "test_mne_anonymize.moc"

