//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>;
 * @since    0.1.9
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
    void testFit_dimensions_dataAndProjectors();
    void testFit_dimensions_nCoils();
    void testFit_dimensions_emptyData();
    void testFit_dimensions_dataAndSensors();
    void testFit_basic_gof();  // test with advanced model and projectors
    void testFit_advanced_gof();  // compare gof to specified value
    void testFit_basic_error();  // compare error to specified value
    void testFit_advanced_error();  // compare error to specified value
    void testCheckForUpdate();
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

void TestHpiFit::testFit_dimensions_dataAndProjectors()
{
    HpiModelParameters hpiModelParameters;
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto matPreparedProjectors = MatrixXd::Zero(306,306);
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            hpiModelParameters,
            matCoilsHead,
            hpiFitResult);

    QVERIFY(hpiFitResult.errorDistances.isEmpty());
}

//=============================================================================================================

void TestHpiFit::testFit_dimensions_nCoils()
{
    HpiModelParameters hpiModelParameters;
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            hpiModelParameters,
            matCoilsHead,
            hpiFitResult);

    QVERIFY(hpiFitResult.errorDistances.isEmpty());
}

//=============================================================================================================

void TestHpiFit::testFit_dimensions_emptyData()
{
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = true;
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);
    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(MatrixXd(),
            MatrixXd(),
            hpiModelParameters,
            matCoilsHead,
            hpiFitResult);

    QVERIFY(hpiFitResult.errorDistances.isEmpty());
}

//=============================================================================================================

void TestHpiFit::testFit_dimensions_dataAndSensors()
{
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = true;
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    hpiDataUpdater.checkForUpdate(m_pFiffInfo);

    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            hpiModelParameters,
            matCoilsHead,
            hpiFitResult);

    QVERIFY(hpiFitResult.errorDistances.isEmpty());
}

//=============================================================================================================

void TestHpiFit::testFit_basic_gof()
{
    /// prepare    
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = true;
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
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
            hpiModelParameters,
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
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
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
            hpiModelParameters,
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
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
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
            hpiModelParameters,
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
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
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
            hpiModelParameters,
            matCoilsHead,
            hpiFitResult);

    QVector<double> vecError = hpiFitResult.errorDistances;

    /// assert
    double dLocalizationErrorMean = 1000.0 * std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();
    QVERIFY(dLocalizationErrorMean < dLocalizationErrorTol);
}

//=============================================================================================================

void TestHpiFit::testCheckForUpdate()
{
    /// prepare
    int iSampleFreq = m_pFiffInfo->sfreq;
    int iLineFreq = m_pFiffInfo->linefreq;
    QVector<int> vecHpiFreqs = {166, 154, 161, 158};
    bool bBasic = false;
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    hpiDataUpdater.checkForUpdate(m_pFiffInfo);

    HPI.checkForUpdate(hpiDataUpdater.getSensors());

    hpiDataUpdater.prepareDataAndProjectors(m_matData,m_matProjectors);
    const auto& matProjectedData = hpiDataUpdater.getProjectedData();
    const auto& matPreparedProjectors = hpiDataUpdater.getProjectors();
    const auto& matCoilsHead = hpiDataUpdater.getHpiDigitizer();

    HpiFitResult hpiFitResult;

    HPI.fit(matProjectedData,
            matPreparedProjectors,
            hpiModelParameters,
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
    HpiModelParameters hpiModelParameters(vecHpiFreqs,
                                          iSampleFreq,
                                          iLineFreq,
                                          bBasic);

    hpiModelParameters = HpiModelParameters(vecFreqsActual01,iSampleFreq,iLineFreq,bBasic);
    QVector<bool> vecResultActual;
    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual01,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual02,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual03,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual04,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual05,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual06,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual07,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual08,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual09,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual10,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual11,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual12,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual13,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual14,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual15,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual16,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual17,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual18,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual19,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual20,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual21,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual22,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual23,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
    vecResultActual << (vecFreqsExpected == hpiFitResult.hpiFreqs);

    hpiFitResult = HpiFitResult();
    hpiModelParameters = HpiModelParameters(vecFreqsActual24,iSampleFreq,iLineFreq,bBasic);
    HPI.fit(matProjectedData,matPreparedProjectors,hpiModelParameters,matCoilsHead,true,hpiFitResult);
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
