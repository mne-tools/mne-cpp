//=============================================================================================================
/**
 * @file     test_fiff_coord_trans.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     September, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     Testframe for FiffCoordTrans.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QtTest>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// Used Namespaces
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCoordTrans
 *
 * @brief The TestFiffCoordTrans class provides read write read fiff verification tests
 *
 */
class TestFiffCoordTrans: public QObject
{
    Q_OBJECT

public:
    TestFiffCoordTrans();

private slots:
    void initTestCase();
    void compareTrans();
    // add other compareFunctions here
    void cleanupTestCase();

private:
    FiffCoordTrans m_transRef;
    FiffCoordTrans m_transTest;

};

//=============================================================================================================

TestFiffCoordTrans::TestFiffCoordTrans()
{
}

//=============================================================================================================

void TestFiffCoordTrans::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);

    // Reference file (Read) and test file (Write)
    QFile fileTransRef(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif");
    QFile fileTransTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test-trans.fif");

    // read reference
    m_transRef = FiffCoordTrans(fileTransRef);

    // write to test file
    m_transRef.write(fileTransTest);

    // read test again
    m_transTest = FiffCoordTrans(fileTransTest);
}

//=============================================================================================================

void TestFiffCoordTrans::compareTrans()
{
    // compare your data here, think about usefull metrics
    QVERIFY(m_transRef == m_transTest);
}

//=============================================================================================================

void TestFiffCoordTrans::cleanupTestCase()
{
    QFile fileTransTest(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test-trans.fif");
    fileTransTest.remove();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCoordTrans)
#include "test_fiff_coord_trans.moc"

