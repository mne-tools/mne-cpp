//=============================================================================================================
/**
 * @file     test_hpiModelParameter.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     February, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Ruben Dörfel. All rights reserved.
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
 * @brief     Test the HpiModelParameters class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <inverse/hpiFit/hpimodelparameters.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestHpiModelParameters
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestHpiModelParameters: public QObject
{
    Q_OBJECT

public:
    TestHpiModelParameters();

private slots:
    void initTestCase();
    void testDefaultConsturctor();
    void testConstructor();
    void testConstructor_basicModel();
    void testCopyConstructor();
    void testCopyAssignment();
    void testCompare_equal();
    void testCompare_notequal();

    // add other compareFunctions here
    void cleanupTestCase();

private:

};

//=============================================================================================================

TestHpiModelParameters::TestHpiModelParameters()
{
}

//=============================================================================================================

void TestHpiModelParameters::initTestCase()
{
    // test your function here
}

//=============================================================================================================

void TestHpiModelParameters::testDefaultConsturctor()
{
    int iExpectedSampleFreq = 0;
    int iExpectedLineFreq = 0;
    QVector<int> vecExpectedHpiFreqs;
    int iExpectedNHpiCoils = 0;
    bool bExpectedBasic = true;

    HpiModelParameters actualHpiModelParameters;
    QVERIFY2(actualHpiModelParameters.vecHpiFreqs() == vecExpectedHpiFreqs,"HPI frequencies do not match.");
    QVERIFY2(actualHpiModelParameters.iNHpiCoils() == iExpectedNHpiCoils,"Number of coils does not match.");
    QVERIFY2(actualHpiModelParameters.iSampleFreq() == iExpectedSampleFreq,"Sampling frequency does not match.");
    QVERIFY2(actualHpiModelParameters.iLineFreq() == iExpectedLineFreq,"Line frequency does not match.");
    QVERIFY2(actualHpiModelParameters.bBasic() == bExpectedBasic,"Model selection does not match.");
}

//=============================================================================================================

void TestHpiModelParameters::testConstructor()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    int iExpectedNHpiCoils = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = false;

    HpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
                                                iExpectedSampleFreq,
                                                iExpectedLineFreq,
                                                bExpectedBasic);

    QVERIFY2(actualHpiModelParameters.vecHpiFreqs() == vecExpectedHpiFreqs,"HPI frequencies do not match.");
    QVERIFY2(actualHpiModelParameters.iNHpiCoils() == iExpectedNHpiCoils,"Number of coils does not match.");
    QVERIFY2(actualHpiModelParameters.iSampleFreq() == iExpectedSampleFreq,"Sampling frequency does not match.");
    QVERIFY2(actualHpiModelParameters.iLineFreq() == iExpectedLineFreq,"Line frequency does not match.");
    QVERIFY2(actualHpiModelParameters.bBasic() == bExpectedBasic,"Model selection does not match.");
}

//=============================================================================================================

void TestHpiModelParameters::testConstructor_basicModel()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 0;
    int iExpectedNHpiCoils = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    HpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
                                                iExpectedSampleFreq,
                                                iExpectedLineFreq,
                                                false);

    QVERIFY2(actualHpiModelParameters.vecHpiFreqs() == vecExpectedHpiFreqs,"HPI frequencies do not match.");
    QVERIFY2(actualHpiModelParameters.iNHpiCoils() == iExpectedNHpiCoils,"Number of coils does not match.");
    QVERIFY2(actualHpiModelParameters.iSampleFreq() == iExpectedSampleFreq,"Sampling frequency does not match.");
    QVERIFY2(actualHpiModelParameters.iLineFreq() == iExpectedLineFreq,"Line frequency does not match.");
    QVERIFY2(actualHpiModelParameters.bBasic() == bExpectedBasic,"Model selection does not match.");
}

//=============================================================================================================

void TestHpiModelParameters::testCopyConstructor()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    HpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    HpiModelParameters actualHpiModelParameters(expectedHpiModelParameters);

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCopyAssignment()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    HpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    HpiModelParameters actualHpiModelParameters = expectedHpiModelParameters;

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCompare_equal()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    HpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    HpiModelParameters actualHpiModelParameters = expectedHpiModelParameters;

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCompare_notequal()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};

    HpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  true);

    HpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
                                                iExpectedSampleFreq,
                                                iExpectedLineFreq,
                                                false);

    QVERIFY(expectedHpiModelParameters != actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiModelParameters)
#include "test_hpiModelParameter.moc"

