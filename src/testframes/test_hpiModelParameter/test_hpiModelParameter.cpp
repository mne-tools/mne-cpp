//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *
 * @file     test_hpiModelParameter.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     February, 2022
 * @brief     Test the InvHpiModelParameters class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>
#include <inv/hpi/inv_hpi_model_parameters.h>

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

using namespace INVLIB;
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

    InvHpiModelParameters actualHpiModelParameters;
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

    InvHpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
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

    InvHpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
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

    InvHpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    InvHpiModelParameters actualHpiModelParameters(expectedHpiModelParameters);

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCopyAssignment()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    InvHpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    InvHpiModelParameters actualHpiModelParameters = expectedHpiModelParameters;

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCompare_equal()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};
    bool bExpectedBasic = true;

    InvHpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  bExpectedBasic);

    InvHpiModelParameters actualHpiModelParameters = expectedHpiModelParameters;

    QVERIFY(expectedHpiModelParameters == actualHpiModelParameters);
}

//=============================================================================================================

void TestHpiModelParameters::testCompare_notequal()
{
    int iExpectedSampleFreq = 1;
    int iExpectedLineFreq = 2;
    QVector<int> vecExpectedHpiFreqs = {1,2};

    InvHpiModelParameters expectedHpiModelParameters(vecExpectedHpiFreqs,
                                                  iExpectedSampleFreq,
                                                  iExpectedLineFreq,
                                                  true);

    InvHpiModelParameters actualHpiModelParameters(vecExpectedHpiFreqs,
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

