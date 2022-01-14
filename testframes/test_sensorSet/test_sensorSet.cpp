//=============================================================================================================
/**
 * @file     test_sensorSet.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     November, 2021
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
 * @brief     Test for the sensor set class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <inverse/hpiFit/sensorset.h>
#include <fiff/fiff_raw_data.h>

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
 * DECLARE CLASS TestSensorSet
 *
 * @brief The TestSensorSet class provides tests for SensorSet
 *
 */
class TestSensorSet: public QObject
{
    Q_OBJECT

public:
    TestSensorSet();

private slots:
    void initTestCase();
    void testConstructor();
    void testChannelList_empty();
    void testChannelList_acc1();
    void testChannelList_acc2();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    QList<FIFFLIB::FiffChInfo> m_lChannels;

};

//=============================================================================================================

TestSensorSet::TestSensorSet()
{
}

//=============================================================================================================

void TestSensorSet::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    // Setup for reading the raw data
    FiffRawData raw = FiffRawData(t_fileIn);
    FiffInfo::SPtr pFiffInfo =  FiffInfo::SPtr(new FiffInfo(raw.info));

    // create meg channel list
    int iNumCh = pFiffInfo->nchan;
    for (int i = 0; i < iNumCh; ++i) {
        if(pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(pFiffInfo->bads.contains(pFiffInfo->ch_names.at(i)))) {
                m_lChannels.append(pFiffInfo->chs[i]);
            }
        }
    }
}

//=============================================================================================================

void TestSensorSet::testConstructor()
{
    int iNChan = 0;
    int iNp = 0;
    int iNRmag = 0;
    int iNCosmag = 0;
    int iNTra = 0;
    int iNW = 0;

    /// act
    SensorSet sensorsActual = SensorSet();

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testChannelList_empty()
{
    int iNChan = 0;
    int iNp = 0;
    int iNRmag = 0;
    int iNCosmag = 0;
    int iNTra = 0;
    int iNW = 0;

    /// act
    int iAccuracy = 1;

    SensorSetCreator sensorSetCreator;
    QList<FIFFLIB::FiffChInfo> lChannels;

    SensorSet sensorsActual = sensorSetCreator.updateSensorSet(lChannels,iAccuracy);

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testChannelList_acc1()
{
    // create vector with expected sizes of sensor struct data
    int iNChan = 204;               // number of channels (204 gradiometers)
    int iNp = 4;                    // 8 integration points for acc 2
    int iNRmag = iNp * iNChan;      // expected number of points for computation, 8 for each sensor -> 8*204
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point

    /// act
    int iAccuracy = 1;
    SensorSetCreator sensorSetCreator;
    SensorSet sensorsActual = sensorSetCreator.updateSensorSet(m_lChannels,iAccuracy);

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testChannelList_acc2()
{
    // create vector with expected sizes of sensor struct data
    int iNChan = 204;               // number of channels (204 gradiometers)
    int iNp = 8;                    // 8 integration points for acc 2
    int iNRmag = iNp * iNChan;      // expected number of points for computation, 8 for each sensor -> 8*204
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point

    /// act
    int iAccuracy = 2;
    SensorSetCreator sensorSetCreator;
    SensorSet sensorsActual = sensorSetCreator.updateSensorSet(m_lChannels,iAccuracy);

    /// assert
    QVERIFY2(iNp == sensorsActual.np,"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils,"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag.rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra.size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w.size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestSensorSet)
#include "test_sensorSet.moc"

