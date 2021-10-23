//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 * @since    0.1.0
 * @date     February, 2020
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
 * @brief     Test for fitHpi function in inverse library.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <iostream>
#include <vector>
#include <math.h>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_dig_point_set.h>

#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/hpifitdata.h>

#include <utils/ioutils.h>
#include <utils/mnemath.h>

#include <fwd/fwd_coil_set.h>

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestHpiFit
 *
 * @brief The TestHpiFit class provides hpi fit verifivcation tests
 *
 */
class TestHpiFit: public QObject
{
    Q_OBJECT

public:
    TestHpiFit();

private slots:
    void initTestCase();                        // run once at the very beginning
    void init();                                // run before each test
    void testConstructor_channels();            // compare channel lists (no lists)
    void testConstructor_bads();                // compare bad channels
    void testConstructor_channels_bads();       // compare channel list when bads are included
    void testConstructor_channels_bads_size();  // compare expected size when bads included
    void testConstructor_sensors();             // check size of sensor struct for sensorset from constructor, acc = 2
    void testUpdateSensors_acc1();              // check size of updated sensor struct for sensorset, acc = 1

    void testUpdateModel_basic_4coils();
    void testUpdateModel_basic_5coils();
    void testUpdateModel_advanced_4coils();
    void testUpdateModel_advanced_5coils();
    void testComputeAmplitudes_basic();
    void cleanupTestCase();
private:
    FiffRawData m_raw;
    QSharedPointer<FiffInfo>  m_pFiffInfo;
    MatrixXd m_matData;
    MatrixXd mHpiPos;
    MatrixXd mRefResult;
    MatrixXd mHpiResult;
    QVector<int> vFreqs;
};

//=============================================================================================================

TestHpiFit::TestHpiFit()
{
}

//=============================================================================================================

void TestHpiFit::initTestCase()
{

    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    // Make sure test folder exists
    QFileInfo t_fileInInfo(t_fileIn);
    QDir().mkdir(t_fileInInfo.path());

    // Setup for reading the raw data
    m_raw = FiffRawData(t_fileIn);

    // read data segment (200 samples)
    int iBuffer = 200;
    MatrixXd matTimes;
    if(!m_raw.read_raw_segment(m_matData, matTimes, m_raw.first_samp,  m_raw.first_samp + iBuffer-1)) {
        qCritical("error during read_raw_segment");
    }
}

//=============================================================================================================

void TestHpiFit::init()
{
    m_pFiffInfo =  QSharedPointer<FiffInfo>(new FiffInfo(m_raw.info));
}

//=============================================================================================================

void TestHpiFit::testConstructor_channels()
{
    /// prepare
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannelsExpected;
    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannelsExpected.append(m_pFiffInfo->chs[i]);
            }
        }
    }

    /// act
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFit::testConstructor_bads()
{
    /// prepare
    // set some  bad channels
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    QList<QString> lBadsExpected = m_pFiffInfo->bads;

    /// act
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<QString> lBadsActual = HPI.getBads();

    /// assert
    QVERIFY(lBadsExpected == lBadsActual);
}

//=============================================================================================================

void TestHpiFit::testConstructor_channels_bads()
{
    /// prepare
    // set some  bad channels
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    QList<QString> lBadsExpected = m_pFiffInfo->bads;

    /// prepare
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannelsExpected;
    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannelsExpected.append(m_pFiffInfo->chs[i]);
            }
        }
    }

    /// act
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFit::testConstructor_channels_bads_size()
{
    /// prepare
    // set some  bad channels
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    QList<QString> lBadsExpected = m_pFiffInfo->bads;
    int iNChanExpected = 202; // 204 gradiometers - 2 bads

    /// act
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();
    int iNChanActual = lChannelsActual.size();

    /// assert
    QVERIFY(iNChanActual == iNChanExpected);
}

//=============================================================================================================

void TestHpiFit::testConstructor_sensors()
{
    /// prepare
    // use already tested functions to get channels
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannels = HPI.getChannels();

    // create vector with expected sizes of sensor struct data
    int iNChan = lChannels.size();  // number of channels
    int iAcc = 2;                   // accuracy to use
    int iNp = 8;                    // 8 integration points for acc 2
    int iNRmag = iNp * iNChan;      // expected number of points for computation, 8 for each sensor -> 8*204
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point

    /// act
    SensorSet sensorsActual = HPI.getSensors();

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestHpiFit::testUpdateSensors_acc1()
{
    /// prepare
    // use already tested functions to get channels
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannels = HPI.getChannels();

    // create vector with expected sizes of sensor struct data
    int iNChan = lChannels.size();  // number of channels
    int iAcc = 1;                   // accuracy to use
    int iNp = 4;                    // 4 integration points for acc 1
    int iNRmag = iNp * iNChan;      // expected number of points for computation, 4 for each sensor -> 4*204
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point

    /// act
    HPI.updateSensor(iAcc);
    SensorSet sensorsActual = HPI.getSensors();

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestHpiFit::testUpdateModel_basic_4coils()
{
    /// Prepare
    // Create HPI object
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    int iSamF = 1000;
    int iLineF = 60;
    int iSamLoc = 200;
    bool bBasic = true;
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    /// Act
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    MatrixXd matModelActual = HPI.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestHpiFit::testUpdateModel_basic_5coils()
{
    /// Prepare
    // Create HPI object
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    int iSamF = 1000;
    int iLineF = 60;
    int iSamLoc = 200;
    bool bBasic = true;
    QVector<int> vecFreqs = {154,158,161,166,172};
    int iNumCoils = vecFreqs.size();

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    /// Act
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    MatrixXd matModelActual = HPI.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestHpiFit::testUpdateModel_advanced_4coils()
{
    /// Prepare
    // Create HPI object
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    int iSamF = 1000;
    int iLineF = 60;
    int iSamLoc = 200;
    bool bBasic = false;
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*4+2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*iLineF*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*iLineF*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);

    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    // reorder for faster computation
    MatrixXd matTemp = matModelExpected;
    RowVectorXi vecIndex(2*iNumCoils);

    vecIndex << 0,4,1,5,2,6,3,7;
    for(int i = 0; i < vecIndex.size(); ++i) {
        matTemp.row(i) = matModelExpected.row(vecIndex(i));
    }
    matModelExpected = matTemp;

    /// Act
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    MatrixXd matModelActual = HPI.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestHpiFit::testUpdateModel_advanced_5coils()
{
    /// Prepare
    // Create HPI object
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    int iSamF = 1000;
    int iLineF = 60;
    int iSamLoc = 600;
    bool bBasic = false;
    QVector<int> vecFreqs = {154,158,161,166,172};
    int iNumCoils = vecFreqs.size();

    // create basic model
    MatrixXd matModelExpected;
    MatrixXd matSimsig(iSamLoc,iNumCoils*4+2);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*iLineF*(i+1)*vecTime.array());
        matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*iLineF*(i+1)*vecTime.array());
    }
    matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
    matSimsig.col(iNumCoils*4+1).fill(1);

    // IOUtils::write_eigen_matrix(matSimsig, QCoreApplication::applicationDirPath() + "/MNE-sample-data/" + "test.txt");

    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

    // reorder for faster computation
    MatrixXd matTemp = matModelExpected;
    RowVectorXi vecIndex(2*iNumCoils);

    vecIndex << 0,5,1,6,2,7,3,8,4,9;
    for(int i = 0; i < vecIndex.size(); ++i) {
        matTemp.row(i) = matModelExpected.row(vecIndex(i));
    }
    matModelExpected = matTemp;

    /// Act
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    MatrixXd matModelActual = HPI.getModel();

    /// Assert
    // use Summed Difference as measure
    MatrixXd matDiff = (matModelActual - matModelExpected);
    double dSD = matDiff.sum();
    QVERIFY(dSD == 0);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_basic()
{
    /// Prepare
    // simulate data
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = 1000;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = false;
    double dAmplitude = 0.5;

    // simulate data
    MatrixXd matAmpExpected;
    MatrixXd matSimData(m_matData);
    MatrixXd matModel;
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmplitude * sin(2*M_PI*vecFreqs[i]*vecTime.array());
        matSimData.row(i+iNumCoils).fill(0);
    }

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,vecFreqs,m_pFiffInfo,matAmpActual,m_pFiffInfo->linefreq,bBasic);
}

void TestHpiFit::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiFit)
#include "test_hpiFit.moc"
