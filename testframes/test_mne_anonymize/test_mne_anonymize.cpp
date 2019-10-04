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
    void initTestCase();
    void compareData();
    void cleanupTestCase();

private:
    double epsilon;
};


//*************************************************************************************************************

TestMneAnonymize::TestMneAnonymize()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestMneAnonymize::initTestCase()
{
    // Init testing arguments
    QString sFileIn(QDir::currentPath()+"/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut(QDir::currentPath()+"/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestMneAnonymize::initTestCase - sFileIn" << sFileIn;
    qInfo() << "TestMneAnonymize::initTestCase - sFileOut" << sFileOut;

    QStringList arguments;
    arguments << "./mne_anonymize";
    arguments << "--in" << sFileIn;
    arguments << "--out" << sFileOut;
    arguments << "--verbose";

    qInfo() << "TestMneAnonymize::initTestCase - arguments" << arguments;

    MNEANONYMIZE::SettingsController controller(arguments);
}


//*************************************************************************************************************

void TestMneAnonymize::compareData()
{
    // Open anonymized file
    QFile inFile(QDir::currentPath()+"/mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    if(inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "TestMneAnonymize::compareData - Anonymized file opened correctly " << inFile.fileName();
    } else {
        QFAIL("Anonymized file could not be loaded.");
    }

    // Create crc checksum and compare to reference
    QByteArray inData(inFile.readAll());

    quint16 crc = qChecksum(inData.data(),static_cast<uint>(inData.size()));
    qInfo() << "TestMneAnonymize::compareData - crc for anonymized file" << crc;

    QVERIFY(17542 == crc);
}


//*************************************************************************************************************

void TestMneAnonymize::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestMneAnonymize)
#include "test_mne_anonymize.moc"
