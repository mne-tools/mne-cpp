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
#include <inverse/hpiFit/sensorset.h>

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
    void initTestCase();                            // run once at the very beginning
    void init();                                    // run before each test

    void testConstructor_channels_size();           // compare size of channel list
    void testConstructor_bads();                    // compare bad channels
    void testConstructor_channels_bads_size();      // compare expected size when bads included
    void testConstructor_sensors();                 // check size of sensor struct for sensorset from constructor, acc = 2
    void testUpdateSensors_acc1();                  // check size of updated sensor struct for sensorset, acc = 1
    void testUpdateSensors_empty();                 // check size of updated sensor when HPI object not yet initialized
    void testUpdateChannels_channels();             // compare channel list
    void testUpdateChannels_channels_bads();        // compare channel list when bads are included
    void testUpdateChannels_channels_bads_size();   // compare channel list  size when bads are included
    void testPrepareProj_emptyHPI();                // test function when HPI object not yet initialized
    void testPrepareProj();                         // test projector preparation
    void testUpdateModel_basic_4coils();
    void testUpdateModel_basic_5coils();
    void testUpdateModel_advanced_4coils();
    void testUpdateModel_advanced_5coils();
    void testComputeAmplitudes_basic_sin();         // test with simulated data, only sines
    void testComputeAmplitudes_basic_cos();         // test with simulated data, only cosines
    void testComputeAmplitudes_basic_sincos();      // test with simulated data, both summed
    void testComputeAmplitudes_advanced_sin();      // test with simulated data, only sines
    void testComputeAmplitudes_advanced_cos();      // test with simulated data, only cosines
    void testComputeAmplitudes_advanced_summedCosSine();    // test with simulated data, summed sines/cos + line
    void testComputeCoilLocation_basic_noproj();            // test with basic model, no projectors
    void testComputeCoilLocation_basic_noproj_trafo();      // test with basic model, no projectors and initial trafo
    void testComputeCoilLocation_basic_proj();              // test with basic model and projectors
    void testComputeCoilLocation_advanced_noproj();         // test with advanced model, no projectors
    void testComputeCoilLocation_advanced_noproj_trafo();   // test with advanced model, no projectors and initial trafo
    void testComputeCoilLocation_advanced_proj();           // test with advanced model and projectors
    void testComputeCoilLocation_basic_gof();               // test with advanced model and projectors
    void testComputeCoilLocation_advanced_gof();            // compare gof to specified value
    void testComputeHeadPosition_error();           // compare error to specified value
    void testFindOrder();                           // test with all possible frequency oders
    void cleanupTestCase();                         // clean-up at the end

private:
    FiffRawData m_raw;
    QSharedPointer<FiffInfo>  m_pFiffInfo;
    MatrixXd m_matData;
    double dErrorTol;
    double dErrorEqualTol;
    double dLocalizationErrorTol;
    double dGofTol;
    MatrixXd m_matProjectors;
    QVector<int> vFreqs;
};

//=============================================================================================================

TestHpiFit::TestHpiFit()
{
    dErrorEqualTol = 0.0000001;
    dErrorTol = 0.0001;
    dLocalizationErrorTol = 1.0;
    dGofTol = 0.88;
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
    m_pFiffInfo =  QSharedPointer<FiffInfo>(new FiffInfo(m_raw.info));

    // read data segment (200 samples)
    int iBuffer = 200;
    MatrixXd matTimes;
    if(!m_raw.read_raw_segment(m_matData, matTimes, m_raw.first_samp,  m_raw.first_samp + iBuffer-1)) {
        qCritical("error during read_raw_segment");
    }

    // Use SSP + SGM + calibration
    m_matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    //Do a copy here because we are going to change the activity flags of the SSP's
    FiffInfo infoTemp = *(m_pFiffInfo.data());

    //Turn on all SSP
    for(int i = 0; i < infoTemp.projs.size(); ++i) {
        infoTemp.projs[i].active = true;
    }

    //Create the projector for all SSP's on
    infoTemp.make_projector(m_matProjectors);

    //set columns of matrix to zero depending on bad channels indexes
    for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
        m_matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
    }
}

//=============================================================================================================

void TestHpiFit::init()
{
    // run at beginning of each test
    m_pFiffInfo =  QSharedPointer<FiffInfo>(new FiffInfo(m_raw.info));
}

//=============================================================================================================

void TestHpiFit::testUpdateChannels_channels()
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
    HPIFit HPI = HPIFit();
    HPI.updateChannels(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFit::testUpdateChannels_channels_bads()
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
    HPIFit HPI = HPIFit();
    HPI.updateChannels(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFit::testUpdateChannels_channels_bads_size()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    int iNChanExpected = 204 - 2;

    /// Act
    HPI.updateChannels(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannels = HPI.getChannels();
    int iNChanActual = lChannels.size();

    /// Assert
    QVERIFY(iNChanActual == iNChanExpected);
}

//=============================================================================================================

void TestHpiFit::testConstructor_channels_size()
{
    /// prepare
    int iNChanExpected = 204; // number of gradiometers

    /// act
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = HPI.getChannels();
    int iNChanActual = lChannelsActual.size();
    /// assert
    QVERIFY(iNChanExpected == iNChanActual);
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
    int iNChan = 204;               // number of channels (204 gradiometers)
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

    // create vector with expected sizes of sensor struct data
    int iNChan = 204;               // number of channels (204 gradiometers)
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

void TestHpiFit::testUpdateSensors_empty()
{
    /// prepare
    HPIFit HPI = HPIFit();
    SensorSet sensorExpected = SensorSet();

    /// act
    int iAcc = 2;
    HPI.updateSensor(iAcc);
    SensorSet sensorsActual = HPI.getSensors();

    /// assert
    QVERIFY2(sensorsActual.np == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(sensorsActual.ncoils == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(sensorsActual.rmag.rows() == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(sensorsActual.cosmag.rows() == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(sensorsActual.tra.size() == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(sensorsActual.w.size() == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestHpiFit::testPrepareProj_emptyHPI()
{
    /// prepare
    // use already tested functions to get channels
    HPIFit HPI = HPIFit();

    // create vector with expected sizes of sensor struct data
    int iSizeExpected = 0;               // 0 expected, not initialized

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// act
    HPI.updateProjectors(matProj);
    MatrixXd matProjPrepared = HPI.getProjectors();
    int iSizeActual = matProjPrepared.cols();

    /// assert
    QVERIFY(iSizeExpected == iSizeActual);
}

//=============================================================================================================

void TestHpiFit::testPrepareProj()
{
    /// prepare
    // use already tested functions to get channels
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // create vector with expected sizes of sensor struct data
    int iSizeExpected = 204;       // number of channels (204 gradiometers)

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// act
    HPI.updateProjectors(matProj);
    MatrixXd matProjPrepared = HPI.getProjectors();
    int iSizeActual = matProjPrepared.cols();

    /// assert
    QVERIFY(iSizeExpected == iSizeActual);
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

//    // reorder for faster computation
//    MatrixXd matTemp = matModelExpected;
//    RowVectorXi vecIndex(2*iNumCoils);

//    vecIndex << 0,4,1,5,2,6,3,7;
//    for(int i = 0; i < vecIndex.size(); ++i) {
//        matTemp.row(i) = matModelExpected.row(vecIndex(i));
//    }
//    matModelExpected = matTemp;

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
    int iSamLoc = 200;
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

    matModelExpected = UTILSLIB::MNEMath::pinv(matSimsig);

//    // reorder for faster computation
//    MatrixXd matTemp = matModelExpected;
//    RowVectorXi vecIndex(2*iNumCoils);

//    vecIndex << 0,5,1,6,2,7,3,8,4,9;
//    for(int i = 0; i < vecIndex.size(); ++i) {
//        matTemp.row(i) = matModelExpected.row(vecIndex(i));
//    }
//    matModelExpected = matTemp;

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

void TestHpiFit::testComputeAmplitudes_basic_sin()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();

    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = true;
    double dAmplitude = 0.5;        // expected amplitudes

    // simulate data - zeros and sines with specified freqs and amplitudes in first 4 channels
    // meaning that computed amplitudes should match in the first 4 cases
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    MatrixXd matModel;
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmplitude * sin(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents to model, first index for channel,
    // second for sine within the model
    matAmpExpected(0,0) = dAmplitude;
    matAmpExpected(1,1) = dAmplitude;
    matAmpExpected(2,2) = dAmplitude;
    matAmpExpected(3,3) = dAmplitude;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,matProj,vecFreqs,m_pFiffInfo,matAmpActual,bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_basic_cos()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();

    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = true;
    double dAmplitude = 0.5;        // expected amplitudes

    // simulate data - zeros and cosines with specified freqs and amplitudes in first 4 channels
    // meaning that computed amplitudes should match in the first 4 cases
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    MatrixXd matModel;
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmplitude * cos(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents to model, first index for channel,
    // second for sine within the model
    matAmpExpected(0,0) = dAmplitude;
    matAmpExpected(1,1) = dAmplitude;
    matAmpExpected(2,2) = dAmplitude;
    matAmpExpected(3,3) = dAmplitude;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,matProj,vecFreqs,m_pFiffInfo,matAmpActual,bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_basic_sincos()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();

    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = true;
    double dAmpSin = 0.5;        // expected amplitudes
    double dAmpCos = 0.25;        // expected amplitudes

    // simulate data - zeros and summed sines and cosines, but different amplitudes
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    MatrixXd matModel;
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSin * sin(2*M_PI*vecFreqs[i]*vecTime.array()) + dAmpCos * cos(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents of fitted model - first index for channel, second for sine/coseine
    // we expect amplitude of sine, since it is higher
    matAmpExpected(0,0) = dAmpSin;
    matAmpExpected(1,1) = dAmpSin;
    matAmpExpected(2,2) = dAmpSin;
    matAmpExpected(3,3) = dAmpSin;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,matProj,vecFreqs,m_pFiffInfo,matAmpActual,bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_sin()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();
    m_pFiffInfo->sfreq = 1000;
    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = false;
    double dAmplitude = 0.5;        // expected amplitudes

    // simulate data - zeros and sines with specified freqs and amplitudes in first 4 channels
    // meaning that computed amplitudes should match in the first 4 cases
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmplitude * sin(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents to model, first index for channel,
    // second for sine within the model
    matAmpExpected(0,0) = dAmplitude;
    matAmpExpected(1,1) = dAmplitude;
    matAmpExpected(2,2) = dAmplitude;
    matAmpExpected(3,3) = dAmplitude;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,matProj,vecFreqs,m_pFiffInfo,matAmpActual,bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_cos()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();

    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = false;
    double dAmplitude = 0.5;        // expected amplitudes

    // simulate data - zeros and cosines with specified freqs and amplitudes in first 4 channels
    // meaning that computed amplitudes should match in the first 4 cases
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmplitude * cos(2*M_PI*vecFreqs[i]*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents to model, first index for channel,
    // second for sine within the model
    matAmpExpected(0,0) = dAmplitude;
    matAmpExpected(1,1) = dAmplitude;
    matAmpExpected(2,2) = dAmplitude;
    matAmpExpected(3,3) = dAmplitude;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    MatrixXd matModel = HPI.getModel();
//    IOUtils::write_eigen_matrix(matSimData, QCoreApplication::applicationDirPath() + "/MNE-sample-data/" + "testData.txt");
//    IOUtils::write_eigen_matrix(matModel, QCoreApplication::applicationDirPath() + "/MNE-sample-data/" + "testModel.txt");
    HPI.computeAmplitudes(matSimData,matProj,vecFreqs,m_pFiffInfo,matAmpActual,bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_summedCosSine()
{
    /// Prepare
    // simulate fiff info with only grads
    int iNumCh = m_pFiffInfo->nchan;
    QList<FIFFLIB::FiffChInfo> lChannels;
    QStringList lChannelsNames;

    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                lChannels.append(m_pFiffInfo->chs[i]);
                lChannelsNames.append(m_pFiffInfo->ch_names[i]);
            }
        }
    }

    m_pFiffInfo->chs = lChannels;
    m_pFiffInfo->ch_names = lChannelsNames;
    m_pFiffInfo->nchan = lChannels.size();

    HPIFit HPI = HPIFit(m_pFiffInfo);

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = 60;
    int iSamLoc = m_matData.cols();
    bool bBasic = false;
    double dAmpSin = 0.5;        // expected amplitudes
    double dAmpCos = 0.25;       // amplitudes of cosine to add
    double dAmpLine = 0.25;      // amplitudes of line freq.

    // simulate data - zeros and summed sines and cosines, but different amplitudes
    MatrixXd matSimData = MatrixXd::Zero(m_pFiffInfo->nchan,200);
    matSimData.fill(0);
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        matSimData.row(i) = dAmpSin * sin(2*M_PI*vecFreqs[i]*vecTime.array())
                            + dAmpCos * cos(2*M_PI*vecFreqs[i]*vecTime.array())
                            + dAmpLine * sin(2*M_PI*60*vecTime.array())
                            + dAmpLine/2 * sin(2*M_PI*60*2*vecTime.array())
                            + dAmpLine/3 * sin(2*M_PI*60*3*vecTime.array());
    }
    MatrixXd matAmpExpected = MatrixXd::Zero(204,4);

    // coefficents of fitted model - first index for channel, second for sine/coseine
    // we expect amplitude of sine, since it is higher
    matAmpExpected(0,0) = dAmpSin;
    matAmpExpected(1,1) = dAmpSin;
    matAmpExpected(2,2) = dAmpSin;
    matAmpExpected(3,3) = dAmpSin;

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// Act
    MatrixXd matAmpActual;
    HPI.updateModel(iSamF,iSamLoc,iLineF,vecFreqs,bBasic);
    HPI.computeAmplitudes(matSimData,
                          matProj,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmpActual,
                          bBasic);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_basic_noproj()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError = {1.0, 1.0, 1.0, 1.0};
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            matProjectors,
                            m_pFiffInfo->dev_head_t,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_basic_noproj_trafo()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            matProjectors,
                            m_pFiffInfo->dev_head_t,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_basic_proj()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          m_matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            m_matProjectors,
                            m_pFiffInfo->dev_head_t,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_advanced_noproj()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError = {1.0, 1.0, 1.0, 1.0};
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = false;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            matProjectors,
                            transDevHead,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_advanced_noproj_trafo()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = false;

    HPI.computeAmplitudes(m_matData,
                          matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            matProjectors,
                            m_pFiffInfo->dev_head_t,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_advanced_proj()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = false;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          m_matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            m_matProjectors,
                            transDevHead,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_basic_gof()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          m_matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            m_matProjectors,
                            transDevHead,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    double meanGoF = vecGoF.mean();
    QVERIFY(meanGoF > dGofTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_advanced_gof()
{
    /// Prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = HPI.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    MatrixXd matAmplitudes;
    bool bBasic = false;
    FiffCoordTrans transDevHead;

    HPI.computeAmplitudes(m_matData,
                          m_matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    /// Act
    MatrixXd matCoilLocActual(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            m_matProjectors,
                            transDevHead,
                            m_pFiffInfo,
                            vecError,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    double meanGoF = vecGoF.mean();
    QVERIFY(meanGoF > dGofTol);
}

//=============================================================================================================

void TestHpiFit::testComputeHeadPosition_error()
{
    /// prepare
    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiPosExpected = HPI.getHpiDigitizer();
    MatrixXd matAmplitudes;
    bool bBasic = true;
    FiffDigPointSet fittedPointSet;

    HPI.computeAmplitudes(m_matData,
                          m_matProjectors,
                          vecFreqs,
                          m_pFiffInfo,
                          matAmplitudes,
                          bBasic);

    MatrixXd matCoilLoc(4,3);

    HPI.computeCoilLocation(matAmplitudes,
                            m_matProjectors,
                            m_pFiffInfo->dev_head_t,
                            m_pFiffInfo,
                            vecError,
                            matCoilLoc,
                            vecGoF);

    /// act
    FiffCoordTrans transDevHeadActual;
    HPI.computeHeadPosition(matCoilLoc,
                            transDevHeadActual,
                            vecError,
                            fittedPointSet);

    /// assert
    double dLocalizationErrorMean = 100.0f * std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();
    QVERIFY(dLocalizationErrorMean < dLocalizationErrorTol);
}

//=============================================================================================================

void TestHpiFit::testFindOrder()
{
    /// prepare
    QVector<int> vecFreqsActual01 = {154,158,161,166};
    QVector<int> vecFreqsActual02 = {158,154,161,166};
    QVector<int> vecFreqsActual03 = {161,154,158,166};
    QVector<int> vecFreqsActual04 = {154,161,158,166};
    QVector<int> vecFreqsActual05 = {158,161,154,166};
    QVector<int> vecFreqsActual06 = {161,158,154,166};
    QVector<int> vecFreqsActual07 = {161,158,166,154};
    QVector<int> vecFreqsActual08 = {158,161,166,154};
    QVector<int> vecFreqsActual09 = {166,161,158,154};
    QVector<int> vecFreqsActual10 = {161,166,158,154};
    QVector<int> vecFreqsActual11 = {158,166,161,154};
    QVector<int> vecFreqsActual12 = {166,158,161,154};
    QVector<int> vecFreqsActual13 = {166,154,161,158};
    QVector<int> vecFreqsActual14 = {154,166,161,158};
    QVector<int> vecFreqsActual15 = {161,166,154,158};
    QVector<int> vecFreqsActual16 = {166,161,154,158};
    QVector<int> vecFreqsActual17 = {154,161,166,158};
    QVector<int> vecFreqsActual18 = {161,154,166,158};
    QVector<int> vecFreqsActual19 = {158,154,166,161};
    QVector<int> vecFreqsActual20 = {154,158,166,161};
    QVector<int> vecFreqsActual21 = {166,158,154,161};
    QVector<int> vecFreqsActual22 = {158,166,154,161};
    QVector<int> vecFreqsActual23 = {154,166,158,161};
    QVector<int> vecFreqsActual24 = {166,154,158,161};

    HPIFit HPI = HPIFit(m_pFiffInfo);
    QVector<int> vecFreqsExpected = {166, 154, 161, 158};
    QVector<bool> vecResultExpected(24);
    vecResultExpected.fill(true);

    /// act
    QVector<bool> vecResultActual;
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual01,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual01);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual02,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual02);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual03,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual03);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual04,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual04);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual05,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual05);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual06,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual06);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual07,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual07);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual08,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual08);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual09,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual09);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual10,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual10);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual11,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual11);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual12,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual12);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual13,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual13);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual14,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual14);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual15,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual15);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual16,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual16);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual17,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual17);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual18,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual18);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual19,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual19);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual20,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual20);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual21,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual21);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual22,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual22);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual23,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual23);
    HPI.findOrder(m_matData,m_matProjectors,vecFreqsActual24,m_pFiffInfo);
    vecResultActual << (vecFreqsExpected == vecFreqsActual24);

    /// assert
    QVERIFY(vecResultExpected == vecResultActual);

}
//=============================================================================================================

void TestHpiFit::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN

//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiFit)
#include "test_hpiFit.moc"
