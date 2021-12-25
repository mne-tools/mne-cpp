//=============================================================================================================
/**
 * @file     test_signalModel.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief     Test for the signal model class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <inverse/hpiFit/signalmodel.h>
#include <utils/mnemath.h>
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

using namespace INVERSELIB;
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
    ModelParameters modelParameters;

    SignalModel signalModel = SignalModel();
    MatrixXd matSimData = MatrixXd::Identity(10,10);

    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);
    MatrixXd matAmpExpected;

    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestSignalModel::testFitData_emptySFreq()
{
    ModelParameters modelParameters;
    modelParameters.vecHpiFreqs = {1,2,3,4};

    SignalModel signalModel = SignalModel();
    MatrixXd matSimData = MatrixXd::Identity(10,10);

    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);
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
    ModelParameters modelParameters;
    modelParameters.iSampleFreq = 1000;
    modelParameters.iLineFreq = 60;
    modelParameters.vecHpiFreqs = {154,158,161,166};
    modelParameters.bBasic = true;

    SignalModel signalModel = SignalModel();

    int iNumCoils = modelParameters.vecHpiFreqs.size();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);
    matSimData.fill(0);

    // expected amplitudes
    double dAmpSine = 0.5;
    double dAmpCosine = 0.25;

    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/modelParameters.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array()) + dAmpCosine * cos(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array());
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
    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);

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
    ModelParameters modelParameters;
    modelParameters.iSampleFreq = 1000;
    modelParameters.iLineFreq = 60;
    modelParameters.vecHpiFreqs = {154,158,161,166,172};
    modelParameters.bBasic = true;

    SignalModel signalModel = SignalModel();

    int iNumCoils = modelParameters.vecHpiFreqs.size();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.5;
    double dAmpCosine = 0.25;

    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/modelParameters.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array()) + dAmpCosine * cos(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array());
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
    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);

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
    ModelParameters modelParameters;
    modelParameters.iSampleFreq = 1000;
    modelParameters.iLineFreq = 60;
    modelParameters.vecHpiFreqs = {154,158,161,166};
    modelParameters.bBasic = false;

    SignalModel signalModel = SignalModel();

    int iNumCoils = modelParameters.vecHpiFreqs.size();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.75;
    double dAmpCosine = 0.5;
    double dAmpLine = 0.3;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/modelParameters.iSampleFreq;
    VectorXd vecTime2 = vecTime;
    vecTime2.fill(1);
    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array())
                            + dAmpCosine * cos(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array())
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
    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);

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
    ModelParameters modelParameters;
    modelParameters.iSampleFreq = 1000;
    modelParameters.iLineFreq = 60;
    modelParameters.vecHpiFreqs = {154,158,161,166,172};
    modelParameters.bBasic = false;
    SignalModel signalModel = SignalModel();

    int iNumCoils = modelParameters.vecHpiFreqs.size();
    int iSamLoc = 200;
    int iNchan = 10;

    // create test signal
    MatrixXd matSimData = MatrixXd::Zero(iNchan,iSamLoc);

    // expected amplitudes
    double dAmpSine = 0.75;
    double dAmpCosine = 0.5;
    double dAmpLine = 0.3;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/modelParameters.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSine * sin(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array())
                            + dAmpCosine * cos(2*M_PI*modelParameters.vecHpiFreqs[i]*vecTime.array())
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
    MatrixXd matAmpActual = signalModel.fitData(modelParameters,matSimData);

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

