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
 * @brief     Test for the signal model class..
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <inverse/hpiFit/signalmodel.h>
#include <utils/mnemath.h>

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
    void testUpdateModel_basic_4coils();
    void testUpdateModel_basic_5coils();
    void testUpdateModel_advanced_4coils();
    void testUpdateModel_advanced_5coils();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    Eigen::MatrixXd mFirstInData;
    Eigen::MatrixXd mSecondInData;

};

//=============================================================================================================

TestSignalModel::TestSignalModel()
{
}

//=============================================================================================================

void TestSignalModel::initTestCase()
{
    // test your function here
}

//=============================================================================================================

void TestSignalModel::testUpdateModel_basic_4coils()
{
    // Prepare
    Frequencies frequencies;
    frequencies.iSampleFreq = 1000;
    frequencies.iLineFreq = 60;
    frequencies.vecHpiFreqs = {154,158,161,166};

    bool bBasic = true;
    SignalModel signalModel = SignalModel(frequencies,bBasic);

    int iNumCoils = frequencies.vecHpiFreqs.size();
    int iSamLoc = 200;
    MatrixXd matData = MatrixXd::Identity(iSamLoc,iSamLoc);

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/frequencies.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
    }
    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);


    /// Act
    signalModel.setData(matData);
    MatrixXd matModelActual = signalModel.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestSignalModel::testUpdateModel_basic_5coils()
{
    // Prepare
    Frequencies frequencies;
    frequencies.iSampleFreq = 1000;
    frequencies.iLineFreq = 60;
    frequencies.vecHpiFreqs = {154,158,161,166,172};

    bool bBasic = true;
    SignalModel signalModel = SignalModel(frequencies,bBasic);

    int iNumCoils = frequencies.vecHpiFreqs.size();
    int iSamLoc = 200;
    MatrixXd matData = MatrixXd::Identity(iSamLoc,iSamLoc);

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/frequencies.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
    }
    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);


    /// Act
    signalModel.setData(matData);
    MatrixXd matModelActual = signalModel.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}


//=============================================================================================================

void TestSignalModel::testUpdateModel_advanced_4coils()
{
    // Prepare
    Frequencies frequencies;
    frequencies.iSampleFreq = 1000;
    frequencies.iLineFreq = 60;
    frequencies.vecHpiFreqs = {154,158,161,166};

    bool bBasic = false;
    SignalModel signalModel = SignalModel(frequencies,bBasic);

    int iNumCoils = frequencies.vecHpiFreqs.size();
    int iSamLoc = 200;
    MatrixXd matData = MatrixXd::Identity(iSamLoc,iSamLoc);

    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*4+2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/frequencies.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*frequencies.iLineFreq*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*frequencies.iLineFreq*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);

    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    //    // reorder for faster computation
    //    MatrixXd matTemp = matModelExpected;
    //    RowVectorXi vecIndex(2*iNumCoils);

    //    vecIndex << 0,4,1,5,2,6,3,7;
    //    for(int i = 0; i < vecIndex.size(); ++i) {
    //        matTemp.row(i) = matModelExpected.row(vecIndex(i));
    //    }
    //    matModelExpected = matTemp;

    /// Act
    signalModel.setData(matData);
    MatrixXd matModelActual = signalModel.getModel();


    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestSignalModel::testUpdateModel_advanced_5coils()
{
    // Prepare
    Frequencies frequencies;
    frequencies.iSampleFreq = 1000;
    frequencies.iLineFreq = 60;
    frequencies.vecHpiFreqs = {154,158,161,166,172};

    bool bBasic = false;
    SignalModel signalModel = SignalModel(frequencies,bBasic);

    int iNumCoils = frequencies.vecHpiFreqs.size();
    int iSamLoc = 200;
    MatrixXd matData = MatrixXd::Identity(iSamLoc,iSamLoc);

    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*4+2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/frequencies.iSampleFreq;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*frequencies.vecHpiFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*frequencies.iLineFreq*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*frequencies.iLineFreq*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);

    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    //    // reorder for faster computation
    //    MatrixXd matTemp = matModelExpected;
    //    RowVectorXi vecIndex(2*iNumCoils);

    //    vecIndex << 0,4,1,5,2,6,3,7;
    //    for(int i = 0; i < vecIndex.size(); ++i) {
    //        matTemp.row(i) = matModelExpected.row(vecIndex(i));
    //    }
    //    matModelExpected = matTemp;

    /// Act
    signalModel.setData(matData);
    MatrixXd matModelActual = signalModel.getModel();


    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
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

