//=============================================================================================================
/**
 * @file     test_hpiDataUpdater.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
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
 * @brief     Unit test for the HpiDataUpdater class..
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <iostream>
#include <inverse/hpiFit/hpidataupdater.h>
#include <inverse/hpiFit/sensorset.h>

#include <fiff/fiff.h>

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

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestHpiDataUpdater
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestHpiDataUpdater: public QObject
{
    Q_OBJECT

public:
    TestHpiDataUpdater();

private slots:
    void initTestCase();
    void init();
    void testPrepareProj_size();                 // add other compareFunctions here
    void testprepareDataAndProjectors();
    void testprepareDataAndProjectors_bads();
    void testPrepareData_bads();
    void testPrepareData();
    void testGetSensors();
    void testGetSensors_bads();
    void testCheckForUpdates_sensors();
    void testCheckForUpdates_data();
    void testCheckForUpdates_projectors();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    FiffRawData m_raw;
    QSharedPointer<FiffInfo>  m_pFiffInfo;
    MatrixXd m_matData;
    MatrixXd m_matProjectors;
    QList<FIFFLIB::FiffChInfo> m_lChannelsWithoutBads;
    QList<FIFFLIB::FiffChInfo> m_lChannelsWithBads;
    QVector<int> m_vecInnerindWithBads;             /**< index of inner channels . */
    QVector<int> m_vecInnerindWithoutBads;             /**< index of inner channels . */

};

//=============================================================================================================

TestHpiDataUpdater::TestHpiDataUpdater()
{
}

//=============================================================================================================

void TestHpiDataUpdater::initTestCase()
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

    // Prepare channel lists without bads
    int iNumCh = m_pFiffInfo->nchan;
    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                m_lChannelsWithoutBads.append(m_pFiffInfo->chs[i]);
                m_vecInnerindWithoutBads.append(i);
            }
        }
    }

    // Prepare channel lists with bads
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    for (int i = 0; i < iNumCh; ++i) {
        if(m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            m_pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(i)))) {
                m_lChannelsWithBads.append(m_pFiffInfo->chs[i]);
                m_vecInnerindWithBads.append(i);
            }
        }
    }
    m_matProjectors = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

}

//=============================================================================================================

void TestHpiDataUpdater::init()
{
    // run at beginning of each test
    m_pFiffInfo =  QSharedPointer<FiffInfo>(new FiffInfo(m_raw.info));
}

//=============================================================================================================

void TestHpiDataUpdater::testPrepareProj_size()
{
    // prepare
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);

    int iSizeExpected = 204;       // number of channels (204 gradiometers)

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    // act
    hpiData.prepareDataAndProjectors(m_matData,matProj);
    MatrixXd matProjPrepared = hpiData.getProjectors();
    int iSizeActual = matProjPrepared.cols();

    // assert
    QVERIFY(iSizeExpected == iSizeActual);
}

//=============================================================================================================

void TestHpiDataUpdater::testprepareDataAndProjectors()
{
    // prepare
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);
    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vecInnerindWithoutBads.size(),matProj.cols());
    MatrixXd matProjectorsExpected(m_vecInnerindWithoutBads.size(),m_vecInnerindWithoutBads.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = matProj.row(m_vecInnerindWithoutBads.at(i));
    }

    for (int i = 0; i < matProjectorsExpected.cols(); ++i) {
        matProjectorsExpected.col(i) = matProjectorsRows.col(m_vecInnerindWithoutBads.at(i));
    }

    // act
    hpiData.prepareDataAndProjectors(m_matData,matProj);
    MatrixXd matProjPrepared = hpiData.getProjectors();

    // assert
    QVERIFY(matProjectorsExpected == matProjPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::testprepareDataAndProjectors_bads()
{
    // prepare
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";

    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);
    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vecInnerindWithBads.size(),matProj.cols());
    MatrixXd matProjectorsExpected(m_vecInnerindWithBads.size(),m_vecInnerindWithBads.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = matProj.row(m_vecInnerindWithBads.at(i));
    }

    for (int i = 0; i < matProjectorsExpected.cols(); ++i) {
        matProjectorsExpected.col(i) = matProjectorsRows.col(m_vecInnerindWithBads.at(i));
    }

    // act
    hpiData.prepareDataAndProjectors(m_matData,matProj);
    MatrixXd matProjPrepared = hpiData.getProjectors();

    // assert
    QVERIFY(matProjectorsExpected == matProjPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::testPrepareData()
{
    // prepare
    // extract data for channels to use
    MatrixXd matDataExpected = MatrixXd(m_vecInnerindWithoutBads.size(), m_matData.cols());

    for(int j = 0; j < m_vecInnerindWithoutBads.size(); ++j) {
        matDataExpected.row(j) << m_matData.row(m_vecInnerindWithoutBads[j]);
    }

    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);

    // act
    hpiData.prepareDataAndProjectors(m_matData,m_matProjectors);
    MatrixXd matDataPrepared = hpiData.getData();

    // assert
    QVERIFY(matDataExpected == matDataPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::testPrepareData_bads()
{
    // extract data for channels to use
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    MatrixXd matDataExpected = MatrixXd(m_vecInnerindWithBads.size(), m_matData.cols());

    for(int j = 0; j < m_vecInnerindWithBads.size(); ++j) {
        matDataExpected.row(j) << m_matData.row(m_vecInnerindWithBads[j]);
    }

    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);

    /// act
    hpiData.prepareDataAndProjectors(m_matData,m_matProjectors);
    MatrixXd matDataPrepared = hpiData.getData();

    /// assert
    QVERIFY(matDataExpected == matDataPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::testGetSensors()
{
    // extract data for channels to use
    SensorSetCreator sensorCreator;
    SensorSet sensorsExpected = sensorCreator.updateSensorSet(m_lChannelsWithoutBads,Accuracy::high);

    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);

    /// act
    SensorSet sensorsActual = hpiData.getSensors();

    /// assert
    QVERIFY(sensorsExpected==sensorsActual);
}

//=============================================================================================================

void TestHpiDataUpdater::testGetSensors_bads()
{
    // extract data for channels to use
    int iAccuracy = 2;

    SensorSetCreator sensorCreator;
    SensorSet sensorsExpected = sensorCreator.updateSensorSet(m_lChannelsWithBads,Accuracy::high);

    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);

    /// act
    SensorSet sensorsActual = hpiData.getSensors();

    /// assert
    QVERIFY(sensorsExpected==sensorsActual);
}

//=============================================================================================================

void TestHpiDataUpdater::testCheckForUpdates_sensors()
{
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    SensorSetCreator sensorCreator;
    SensorSet sensorsExpected = sensorCreator.updateSensorSet(m_lChannelsWithBads,Accuracy::high);

    /// act
    hpiData.checkForUpdate(m_pFiffInfo);
    SensorSet sensorsActual = hpiData.getSensors();

    /// assert
    QVERIFY(sensorsExpected==sensorsActual);
}

//=============================================================================================================

void TestHpiDataUpdater::testCheckForUpdates_data()
{
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    MatrixXd matDataExpected = MatrixXd(m_vecInnerindWithBads.size(), m_matData.cols());

    for(int j = 0; j < m_vecInnerindWithBads.size(); ++j) {
        matDataExpected.row(j) << m_matData.row(m_vecInnerindWithBads[j]);
    }

    /// act
    hpiData.checkForUpdate(m_pFiffInfo);
    hpiData.prepareDataAndProjectors(m_matData,m_matProjectors);
    MatrixXd matDataPrepared = hpiData.getData();

    /// assert
    QVERIFY(matDataExpected == matDataPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::testCheckForUpdates_projectors()
{
    HpiDataUpdater hpiData = HpiDataUpdater(m_pFiffInfo);
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vecInnerindWithBads.size(),matProj.cols());
    MatrixXd matProjectorsExpected(m_vecInnerindWithBads.size(),m_vecInnerindWithBads.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = matProj.row(m_vecInnerindWithBads.at(i));
    }

    for (int i = 0; i < matProjectorsExpected.cols(); ++i) {
        matProjectorsExpected.col(i) = matProjectorsRows.col(m_vecInnerindWithBads.at(i));
    }

    // act
    hpiData.checkForUpdate(m_pFiffInfo);
    hpiData.prepareDataAndProjectors(m_matData,matProj);
    MatrixXd matProjPrepared = hpiData.getProjectors();

    // assert
    QVERIFY(matProjectorsExpected == matProjPrepared);
}

//=============================================================================================================

void TestHpiDataUpdater::cleanupTestCase()
{

}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiDataUpdater)
#include "test_hpiDataUpdater.moc"

