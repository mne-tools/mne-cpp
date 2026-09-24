//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     test_signalModel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     December, 2021
 * @brief     Test for the signal model class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>
#include <inv/hpi/inv_signal_model.h>
#include <iostream>
#include <utils/ioutils.h>

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
using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestSignalModel
 *
 * @brief The TestSignalModel class provides read write read fiff verification tests
 *
 */
class TestSignalModel: public QObject
{
    Q_OBJECT

public:
    TestSignalModel();

private slots:
    void initTestCase();
    void testFitData_emptyHpiFres();
    void testFitData_emptySFreq();
    void testFitData_basic_4coils();
    void testFitData_basic_5coils();
    void testFitData_advanced_4coils();
    void testFitData_advanced_5coils();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    Eigen::MatrixXd mFirstInData;
    Eigen::MatrixXd mSecondInData;
    double dErrorTol;
};

//=============================================================================================================

TestSignalModel::TestSignalModel()
{
    dErrorTol = 0.00000001;
}

//=============================================================================================================

void TestSignalModel::initTestCase()
{
    // test your function here
}

//=============================================================================================================

void TestSignalModel::testFitData_emptyHpiFres()
{
    InvHpiModelParameters hpiModelParameters;

    InvSignalModel signalModel = InvSignalModel();
    MatrixXd matSimData = MatrixXd::Identity(10,10);

    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);
    MatrixXd matAmpExpected;

    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_emptySFreq()
{
    QVector<int> vecHpiFreqs = {1,2,3,4};
    InvHpiModelParameters hpiModelParameters(vecHpiFreqs,0,0,true);

    InvSignalModel signalModel = InvSignalModel();
    MatrixXd matSimData = MatrixXd::Identity(10,10);

    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);
    MatrixXd matAmpExpected;

    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_basic_4coils()
{
    // Prepare
    int iSampleFreq = 1000;
    int iLineFreq = 60;
    QVector<int> vecHpiFreqs = {154,158,161,166};
    bool bBasic = true;
    InvHpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);
    InvSignalModel signalModel = InvSignalModel();

    int iNumCoils = hpiModelParameters.iNHpiCoils();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);
    matSimData.fill(0);

    // expected amplitudes
    double dAmpSine = 0.5;
    double dAmpCosine = 0.25;

    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/hpiModelParameters.iSampleFreq();

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array()) + dAmpCosine * cos(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array());
    }

    MatrixXd matAmpExpected = MatrixXd::Zero(2*iNumCoils,iNchan);
    matAmpExpected(0,0) = dAmpSine;
    matAmpExpected(1,1) = dAmpSine;
    matAmpExpected(2,2) = dAmpSine;
    matAmpExpected(3,3) = dAmpSine;
    matAmpExpected(4,0) = dAmpCosine;
    matAmpExpected(5,1) = dAmpCosine;
    matAmpExpected(6,2) = dAmpCosine;
    matAmpExpected(7,3) = dAmpCosine;

    /// Act
    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_basic_5coils()
{
    // Prepare
    int iSampleFreq = 1000;
    int iLineFreq = 60;
    QVector<int> vecHpiFreqs = {154,158,161,166,172};
    bool bBasic = true;
    InvHpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);
    InvSignalModel signalModel = InvSignalModel();

    int iNumCoils = hpiModelParameters.iNHpiCoils();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.5;
    double dAmpCosine = 0.25;

    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/hpiModelParameters.iSampleFreq();

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array()) + dAmpCosine * cos(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array());
    }

    MatrixXd matAmpExpected = MatrixXd::Zero(2*iNumCoils,iNchan);
    matAmpExpected(0,0) = dAmpSine;
    matAmpExpected(1,1) = dAmpSine;
    matAmpExpected(2,2) = dAmpSine;
    matAmpExpected(3,3) = dAmpSine;
    matAmpExpected(4,4) = dAmpSine;

    matAmpExpected(5,0) = dAmpCosine;
    matAmpExpected(6,1) = dAmpCosine;
    matAmpExpected(7,2) = dAmpCosine;
    matAmpExpected(8,3) = dAmpCosine;
    matAmpExpected(9,4) = dAmpCosine;

    /// Act
    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_advanced_4coils()
{
    // Prepare
    int iSampleFreq = 1000;
    int iLineFreq = 60;
    QVector<int> vecHpiFreqs = {154,158,161,166};
    bool bBasic = false;
    InvHpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);

    InvSignalModel signalModel = InvSignalModel();

    int iNumCoils = hpiModelParameters.iNHpiCoils();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.75;
    double dAmpCosine = 0.5;
    double dAmpLine = 0.3;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/hpiModelParameters.iSampleFreq();
    VectorXd vecTime2 = vecTime;
    vecTime2.fill(1);
    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array())
                            + dAmpCosine * cos(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array())
                            + dAmpLine * sin(2*M_PI*60*vecTime.array())
                            + dAmpLine/2 * sin(2*M_PI*60*2*vecTime.array())
                            + dAmpLine/3 * sin(2*M_PI*60*3*vecTime.array())
                            + dAmpLine * VectorXd::LinSpaced(iSamLoc, -0.5, 0.5).array()
                            + dAmpLine * vecTime2.array();
    }

    MatrixXd matAmpExpected = MatrixXd::Zero(2*iNumCoils,iNchan);
    matAmpExpected(0,0) = dAmpSine;
    matAmpExpected(1,1) = dAmpSine;
    matAmpExpected(2,2) = dAmpSine;
    matAmpExpected(3,3) = dAmpSine;
    matAmpExpected(4,0) = dAmpCosine;
    matAmpExpected(5,1) = dAmpCosine;
    matAmpExpected(6,2) = dAmpCosine;
    matAmpExpected(7,3) = dAmpCosine;

    /// Act
    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_advanced_5coils()
{
    // Prepare
    int iSampleFreq = 1000;
    int iLineFreq = 60;
    QVector<int> vecHpiFreqs = {154,158,161,166,172};
    bool bBasic = false;
    InvHpiModelParameters hpiModelParameters(vecHpiFreqs,
                                         iSampleFreq,
                                         iLineFreq,
                                         bBasic);
    InvSignalModel signalModel = InvSignalModel();

    int iNumCoils = hpiModelParameters.iNHpiCoils();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.75;
    double dAmpCosine = 0.5;
    double dAmpLine = 0.3;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/hpiModelParameters.iSampleFreq();

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array())
                            + dAmpCosine * cos(2*M_PI*hpiModelParameters.vecHpiFreqs()[i]*vecTime.array())
                            + dAmpLine * sin(2*M_PI*60*vecTime.array())
                            + dAmpLine/2 * sin(2*M_PI*60*2*vecTime.array())
                            + dAmpLine/3 * sin(2*M_PI*60*3*vecTime.array());
    }

    MatrixXd matAmpExpected = MatrixXd::Zero(2*iNumCoils,iNchan);
    matAmpExpected(0,0) = dAmpSine;
    matAmpExpected(1,1) = dAmpSine;
    matAmpExpected(2,2) = dAmpSine;
    matAmpExpected(3,3) = dAmpSine;
    matAmpExpected(4,4) = dAmpSine;

    matAmpExpected(5,0) = dAmpCosine;
    matAmpExpected(6,1) = dAmpCosine;
    matAmpExpected(7,2) = dAmpCosine;
    matAmpExpected(8,3) = dAmpCosine;
    matAmpExpected(9,4) = dAmpCosine;

    /// Act
    MatrixXd matAmpActual = signalModel.fitData(hpiModelParameters,matSimData);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestSignalModel)
#include "test_signalModel.moc"

