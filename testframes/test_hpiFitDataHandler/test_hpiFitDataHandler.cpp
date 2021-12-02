//=============================================================================================================
/**
 * @file     test_hpiFitDataHandler.cpp
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
 * @brief     Unit test for the HpiFitDataHandler class..
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <iostream>
#include <inverse/hpiFit/hpifitdatahandler.h>
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
 * DECLARE CLASS TestHpiFitDataHandler
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestHpiFitDataHandler: public QObject
{
    Q_OBJECT

public:
    TestHpiFitDataHandler();

private slots:
    void initTestCase();
    void init();
    void testConstructor_channels_size();           // compare size of channel list
    void testConstructor_bads();                    // compare bad channels
    void testConstructor_channels_bads_size();      // compare expected size when bads included
    void testUpdateChannels_channels();             // compare channel list
    void testUpdateChannels_channels_bads();        // compare channel list when bads are included
    void testUpdateChannels_channels_bads_size();   // compare channel list  size when bads are included
    void testPrepareProj();     // add other compareFunctions here
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    FiffRawData m_raw;
    QSharedPointer<FiffInfo>  m_pFiffInfo;
    MatrixXd m_matData;
    MatrixXd m_matProjectors;

};

//=============================================================================================================

TestHpiFitDataHandler::TestHpiFitDataHandler()
{
}

//=============================================================================================================

void TestHpiFitDataHandler::initTestCase()
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

void TestHpiFitDataHandler::init()
{
    // run at beginning of each test
    m_pFiffInfo =  QSharedPointer<FiffInfo>(new FiffInfo(m_raw.info));
}

//=============================================================================================================

void TestHpiFitDataHandler::testUpdateChannels_channels()
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

    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = hpiData.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFitDataHandler::testUpdateChannels_channels_bads()
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
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = hpiData.getChannels();

    /// assert
    QVERIFY(lChannelsExpected == lChannelsActual);
}

//=============================================================================================================

void TestHpiFitDataHandler::testUpdateChannels_channels_bads_size()
{
    /// Prepare
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    int iNChanExpected = 204 - 2;

    /// Act
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = hpiData.getChannels();
    int iNChanActual = lChannelsActual.size();

    /// Assert
    QVERIFY(iNChanActual == iNChanExpected);
}

//=============================================================================================================

void TestHpiFitDataHandler::testConstructor_channels_size()
{
    /// prepare
    int iNChanExpected = 204; // number of gradiometers

    /// act
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = hpiData.getChannels();
    int iNChanActual = lChannelsActual.size();

    /// assert
    QVERIFY(iNChanExpected == iNChanActual);
}

//=============================================================================================================

void TestHpiFitDataHandler::testConstructor_bads()
{
    /// prepare
    // set some  bad channels
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    QList<QString> lBadsExpected = m_pFiffInfo->bads;

    /// act
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<QString> lBadsActual = hpiData.getBads();

    /// assert
    QVERIFY(lBadsExpected == lBadsActual);
}

//=============================================================================================================

void TestHpiFitDataHandler::testConstructor_channels_bads_size()
{
    /// prepare
    // set some  bad channels
    m_pFiffInfo->bads << "MEG0113" << "MEG0112";
    QList<QString> lBadsExpected = m_pFiffInfo->bads;
    int iNChanExpected = 202; // 204 gradiometers - 2 bads

    /// act
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);
    QList<FIFFLIB::FiffChInfo> lChannelsActual = hpiData.getChannels();
    int iNChanActual = lChannelsActual.size();

    /// assert
    QVERIFY(iNChanActual == iNChanExpected);
}

//=============================================================================================================

void TestHpiFitDataHandler::testPrepareProj()
{
    /// prepare
    HpiFitDataHandler hpiData = HpiFitDataHandler(m_pFiffInfo);

    int iSizeExpected = 204;       // number of channels (204 gradiometers)

    MatrixXd matProj = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    /// act
    hpiData.prepareProjectors(matProj);
    MatrixXd matProjPrepared = hpiData.getProjectors();
    int iSizeActual = matProjPrepared.cols();

    /// assert
    QVERIFY(iSizeExpected == iSizeActual);
}

//=============================================================================================================

void TestHpiFitDataHandler::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestHpiFitDataHandler)
#include "test_hpiFitDataHandler.moc"

