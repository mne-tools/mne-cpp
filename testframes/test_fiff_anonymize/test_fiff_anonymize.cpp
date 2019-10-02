//=============================================================================================================
/**
* @file     test_fiff_anonymize.cpp
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

#include <fiff/fiff.h>

#include <iostream>


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

//=============================================================================================================
/**
* DECLARE CLASS TestFiffAnonymize
*
* @brief The TestFiffAnonymize class provides fiff anonymizing verification tests
*
*/
class TestFiffAnonymize: public QObject
{
    Q_OBJECT

public:
    TestFiffAnonymize();

private slots:
    void initTestCase();
    void compareData();
    void cleanupTestCase();

private:
    double epsilon;
};


//*************************************************************************************************************

TestFiffAnonymize::TestFiffAnonymize()
: epsilon(0.000001)
{
}



//*************************************************************************************************************

void TestFiffAnonymize::initTestCase()
{
    qInfo() << "TestFiffAnonymize::initTestCase - Epsilon" << epsilon;

    // Init testing arguments
    QString sFileIn("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif");
    QString sFileOut("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short_anonymized.fif");

    qInfo() << "TestFiffAnonymize::initTestCase - sFileIn" << sFileIn;
    qInfo() << "TestFiffAnonymize::initTestCase - sFileOut" << sFileOut;

    QString program = "./mne_anonymize";
    QStringList arguments;
    arguments << "--in" << sFileIn;
    arguments << "--out" << sFileOut;

    // Pass arguments to application and anaonyimze the fiff file
    QScopedPointer<QProcess> myProcess (new QProcess);
    myProcess->start(program, arguments);
    myProcess->waitForFinished();
}


//*************************************************************************************************************

void TestFiffAnonymize::compareData()
{
    // Open ./mne-cpp-test-data/MEG/sample/sample_audvis_raw_anonymized.fif
    QString inFileName("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_anonymized.fif");
    QFile inFile(inFileName);
    QByteArray inData(inFile.readAll());
    quint16 crc = qChecksum(inData.data(),static_cast<uint>(inData.size()));
    qDebug() << crc;

    // ToDo: Implement function which reads sensitive tags and checks if they were anaonymized. Use Q_VERIFY().
}

//*************************************************************************************************************

void TestFiffAnonymize::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestFiffAnonymize)
#include "test_fiff_anonymize.moc"
