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
#include <inverse/hpiFit/hpidataupdater.h>
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
    void initTestCase(); // run once at the very beginning
    void init(); // run before each test
    void testComputeAmplitudes_basic_sin();  // test with simulated data, only sines
    void testComputeAmplitudes_basic_cos();  // test with simulated data, only cosines
    void testComputeAmplitudes_basic_sincos();  // test with simulated data, both summed
    void testComputeAmplitudes_advanced_sin();  // test with simulated data, only sines
    void testComputeAmplitudes_advanced_cos();  // test with simulated data, only cosines
    void testComputeAmplitudes_advanced_summedCosSine();  // test with simulated data, summed sines/cos + line
    void testComputeCoilLocation_basic_noproj();  // test with basic model, no projectors
    void testComputeCoilLocation_basic_noproj_trafo();  // test with basic model, no projectors and initial trafo
    void testComputeCoilLocation_basic_proj();  // test with basic model and projectors
    void testComputeCoilLocation_advanced_noproj();  // test with advanced model, no projectors
    void testComputeCoilLocation_advanced_noproj_trafo();  // test with advanced model, no projectors and initial trafo
    void testComputeCoilLocation_advanced_proj();  // test with advanced model and projectors
    void testFit_basic_gof();  // test with advanced model and projectors
    void testFit_advanced_gof();  // compare gof to specified value
    void testFit_basic_error();  // compare error to specified value
    void testFit_advanced_error();  // compare error to specified value
    void testFindOrder();  // test with all possible frequency oders
    void cleanupTestCase();  // clean-up at the end

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
    dLocalizationErrorTol = 3.0; // mm
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

void TestHpiFit::testComputeAmplitudes_basic_sin()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_basic_cos()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_basic_sincos()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = m_pFiffInfo->linefreq;
    bool bBasic = true;
    int iSamLoc = m_matData.cols();
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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_sin()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_cos()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    // init test data
    QVector<int> vecFreqs = {154,158,161,166};
    int iNumCoils = vecFreqs.size();
    int iSamF = m_pFiffInfo->sfreq;
    int iLineF = m_pFiffInfo->linefreq;
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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeAmplitudes_advanced_summedCosSine()
{
    // Prepare
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

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

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
    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    hpiDataUpdater.prepareDataAndProjectors(matSimData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmpActual);

    /// Assert
    // use summed squared error ssd
    MatrixXd matDiff = matAmpActual - matAmpExpected;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    QVERIFY(dSSD < dErrorEqualTol);
}

//=============================================================================================================

void TestHpiFit::testComputeCoilLocation_basic_noproj()
{
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError = {1.0, 1.0, 1.0, 1.0};
    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
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
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
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
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = true;
    FiffCoordTrans transDevHead;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
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
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError = {1.0, 1.0, 1.0, 1.0};
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = false;
    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;
    FiffCoordTrans transDevHead;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
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
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    MatrixXd matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = false;
    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
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
    // Prepare
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    QVector<int> vecFreqs = {166, 154, 161, 158};
    QVector<double> vecError(4);
    VectorXd vecGoF;
    MatrixXd matHpiDigitizer = hpiDataUpdater.getHpiDigitizer();
    MatrixXd matCoilLocExpected = m_pFiffInfo->dev_head_t.apply_inverse_trans(matHpiDigitizer.cast<float>()).cast<double>();
    bool bBasic = false;
    FiffCoordTrans transDevHead;

    int iLineF = 60;
    int iSamF = m_pFiffInfo->sfreq;

    ModelParameters modelParameters = setModelParameters(vecFreqs,
                                                         iSamF,
                                                         iLineF,
                                                         bBasic);

    MatrixXd matAmplitudes;
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    HPI.computeAmplitudes(matProjectedData,
                          modelParameters,
                          matAmplitudes);

    /// Act
    MatrixXd matCoilLocActual(4,3);
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HPI.computeCoilLocation(matAmplitudes,
                            matPreparedProjectors,
                            m_pFiffInfo->dev_head_t,
                            vecError,
                            matCoilsHead,
                            matCoilLocActual,
                            vecGoF);

    /// Assert
    MatrixXd matDiff = matCoilLocExpected - matCoilLocActual;
    double dSSD = (matDiff*matDiff.transpose()).trace();
    qDebug() << dSSD;

    QVERIFY(dSSD < dErrorTol);
}

//=============================================================================================================

void TestHpiFit::testFit_basic_gof()
{
    /// prepare    
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = true;
    ModelParameters modelParameters = setModelParameters(vecHpiFreqs,
                                                         iSampleFreq,
                                                         iLineFreq,
                                                         bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            modelParameters,
            matCoilsHead,
            hpiFitResult);

    VectorXd vecGoF = hpiFitResult.GoF;

    /// Assert
    double meanGoF = vecGoF.mean();
    QVERIFY(meanGoF > dGofTol);
}

//=============================================================================================================

void TestHpiFit::testFit_advanced_gof()
{
    /// prepare
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = false;
    ModelParameters modelParameters = setModelParameters(vecHpiFreqs,
                                                         iSampleFreq,
                                                         iLineFreq,
                                                         bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            modelParameters,
            matCoilsHead,
            hpiFitResult);

    VectorXd vecGoF = hpiFitResult.GoF;

    /// Assert
    double meanGoF = vecGoF.mean();
    QVERIFY(meanGoF > dGofTol);
}

//=============================================================================================================

void TestHpiFit::testFit_basic_error()
{
    /// prepare
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = true;
    ModelParameters modelParameters = setModelParameters(vecHpiFreqs,
                                                         iSampleFreq,
                                                         iLineFreq,
                                                         bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            modelParameters,
            matCoilsHead,
            hpiFitResult);

    QVector<double> vecError = hpiFitResult.errorDistances;

    /// assert
    double dLocalizationErrorMean = 1000.0 * std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();
    QVERIFY(dLocalizationErrorMean < dLocalizationErrorTol);
}

//=============================================================================================================

void TestHpiFit::testFit_advanced_error()
{
    /// prepare
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = false;
    ModelParameters modelParameters = setModelParameters(vecHpiFreqs,
                                                         iSampleFreq,
                                                         iLineFreq,
                                                         bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            modelParameters,
            matCoilsHead,
            hpiFitResult);

    QVector<double> vecError = hpiFitResult.errorDistances;

    /// assert
    double dLocalizationErrorMean = 1000.0 * std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();
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

    QVector<int> vecFreqsExpected = {166, 154, 161, 158};
    QVector<bool> vecResultExpected(24);
    vecResultExpected.fill(true);

    /// act
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = false;
    ModelParameters modelParameters = setModelParameters(vecHpiFreqs,
                                                         iSampleFreq,
                                                         iLineFreq,
                                                         bBasic);

    QVector<bool> vecResultActual;
    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual01;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual02;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual03;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual04;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual05;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual06;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual07;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual08;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual09;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual10;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual11;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual12;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual13;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual14;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual15;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual16;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual17;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual18;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual19;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual20;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual21;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual22;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual23;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    modelParameters.vecHpiFreqs = vecFreqsActual24;
    HPI.fit(matProjectedData,matPreparedProjectors,modelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

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
