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
    void testInOutSameNameAndDeleteInFile();

    //test anonymization
    void testDefaultAnonymizationOfTags();

    void compareBirthdayOffsetOption();
    void cleanupTestCase();

private:
    double epsilon;
    void searchForTag(FIFFLIB::FiffStream &outStream,
                     FIFFLIB::fiff_int_t ThisKind,
                     FiffTag::SPtr inTag);
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

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize - Testing", "1.0");

    verifyCRC(sFileOut,17542);


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

    verifyCRC(sFileOutTest,17542);
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

    verifyCRC(sFileOutTest,17542);
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

    QFile fFileIn(sFileIn);
    FIFFLIB::FiffStream inStream(&fFileIn);
    if(inStream.open(QIODevice::ReadOnly))
    {
        qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - Input file opened correctly " << sFileIn;
    } else {
        QFAIL("Input file could not be loaded.");
    }

    QFile fFileOut(sFileOut);
    FIFFLIB::FiffStream outStream(&fFileOut);
    if(outStream.open(QIODevice::ReadOnly))
    {
        qInfo() << "TestMneAnonymize::testDefaultAnonymizationOfTags - output file opened correctly " << sFileIn;
    } else {
        QFAIL("Output file could not be loaded.");
    }

    FiffTag::SPtr pTag = FiffTag::SPtr::create();
    searchForTag(inStream,FIFF_FILE_ID,pTag);



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
    arguments << "--verbose";

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments, "MNE Anonymize", "1.0");

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

void TestMneAnonymize::searchForTag(FIFFLIB::FiffStream &stream,
                                   FIFFLIB::fiff_int_t thisKind,
                                   FiffTag::SPtr pTag)
{
    stream.device()->seek(0);
    do
    {
        stream.read_tag(pTag);
    }while(pTag->kind != thisKind);
}


//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestMneAnonymize)
#include "test_mne_anonymize.moc"

