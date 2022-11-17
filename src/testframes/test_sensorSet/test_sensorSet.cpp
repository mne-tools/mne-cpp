//=============================================================================================================
/**
 * @file     test_sensorSet.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
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
#include <fwd/fwd_coil_set.h>

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
using namespace FWDLIB;
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
    void testSensorSet_defaultConstructor();
    void testSensorSet_constructor_nullptr();
    void testSensorSet_constructor_accuracyLow();
    void testSensorSet_constructor_accuracyMedium();
    void testSensorSet_constructor_accuracyHigh();
    void testSensorSet_equal();
    void testSensorSet_notequal();
    void testSensorSetCreator_channelList_empty();
    void testSensorSetCreator_channelList_medium();
    void cleanupTestCase();

private:
    // declare your thresholds, variables and error values here
    QList<FIFFLIB::FiffChInfo> m_lChannels;
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilDefinitions{nullptr};
};

//=============================================================================================================

TestSensorSet::TestSensorSet()
{
}

//=============================================================================================================

void TestSensorSet::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    m_pCoilDefinitions = FwdCoilSet::SPtr(FwdCoilSet::read_coil_defs(qPath));

    // Setup for reading the raw data
    QFile t_fileIn(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");
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

void TestSensorSet::testSensorSet_defaultConstructor()
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
    QVERIFY2(iNp == sensorsActual.np(),"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils(),"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra().size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w().size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testSensorSet_constructor_nullptr()
{
    int iNChan = 0;
    int iNp = 0;
    int iNRmag = 0;
    int iNCosmag = 0;
    int iNTra = 0;
    int iNW = 0;

    /// act
    SensorSet sensorsActual = SensorSet(nullptr);

    /// assert
    QVERIFY2(iNp == sensorsActual.np(),"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils(),"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra().size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w().size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testSensorSet_constructor_accuracyLow()
{
    int iNChan = 204;               // number of channels (204 gradiometers)
    int iNp = 2;                    // 1 integration points for each coil (gradiometers -> x2) accuracy low
    int iNRmag = iNp * iNChan;      // expected number of points for computation
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point
    Accuracy accuracy = Accuracy::low;
    auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy), nullptr));

    /// act
    SensorSet sensorsActual = SensorSet(pCoilMeg);

    /// assert
    QVERIFY2(iNp == sensorsActual.np(),"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils(),"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra().size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w().size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testSensorSet_constructor_accuracyMedium()
{
    int iNChan = 204;               // number of channels (204 gradiometers)
    int iNp = 4;                    // 4 integration points for accuracy medium
    int iNRmag = iNp * iNChan;      // expected number of points for computation
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point
    int iAcc = 1;

    auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), iAcc, nullptr));

    /// act
    SensorSet sensorsActual = SensorSet(pCoilMeg);

    /// assert
    QVERIFY2(iNp == sensorsActual.np(),"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils(),"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra().size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w().size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testSensorSet_constructor_accuracyHigh()
{
    int iNChan = 204;               // number of channels (204 gradiometers)
    int iNp = 8;                    // 8 integration points for accuracy high
    int iNRmag = iNp * iNChan;      // expected number of points for computation, 8 for each sensor -> 8*204
    int iNCosmag = iNp * iNChan;    // same as rmag
    int iNTra = iNChan*iNChan;      // size square matrix 204*204
    int iNW = iNp * iNChan;         // one weight for each point
    Accuracy accuracy = Accuracy::high;
    auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy), nullptr));

    /// act
    SensorSet sensorsActual = SensorSet(pCoilMeg);

    /// assert
    QVERIFY2(iNp == sensorsActual.np(),"Number of integration points does not match.");
    QVERIFY2(iNChan == sensorsActual.ncoils(),"Number of channels does not match.");
    QVERIFY2(iNRmag == sensorsActual.rmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNCosmag == sensorsActual.cosmag().rows(),"Number of points for computation does not match.");
    QVERIFY2(iNTra == sensorsActual.tra().size(),"Size of square matrix does not match.");
    QVERIFY2(iNW == sensorsActual.w().size(),"Number of iweights does not match");
}

//=============================================================================================================

void TestSensorSet::testSensorSet_equal()
{
    Accuracy accuracy = Accuracy::medium;
    auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy), nullptr));
    SensorSet sensorsA = SensorSet(pCoilMeg);
    SensorSet sensorsB = SensorSet(pCoilMeg);
    QVERIFY(sensorsA == sensorsB);
}
//=============================================================================================================

void TestSensorSet::testSensorSet_notequal()
{
    Accuracy accuracy1 = Accuracy::medium;
    Accuracy accuracy2 = Accuracy::high;

    auto pCoilMeg1 = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy1), nullptr));
    auto pCoilMeg2 = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy2), nullptr));

    SensorSet sensorsA = SensorSet(pCoilMeg1);
    SensorSet sensorsB = SensorSet(pCoilMeg2);
    QVERIFY(sensorsA != sensorsB);
}

//=============================================================================================================

void TestSensorSet::testSensorSetCreator_channelList_empty()
{
    // act
    SensorSetCreator sensorSetCreator;
    QList<FIFFLIB::FiffChInfo> lChannels;
    Accuracy accuracy = Accuracy::medium;

    SensorSet sensorsActual = sensorSetCreator.updateSensorSet(lChannels,accuracy);
    SensorSet sensorsExpected = SensorSet();

    // assert
    QVERIFY(sensorsActual==sensorsExpected);
}

//=============================================================================================================

void TestSensorSet::testSensorSetCreator_channelList_medium()
{
    Accuracy accuracy = Accuracy::medium;
    auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(m_lChannels, m_lChannels.size(), static_cast<int>(accuracy), nullptr));
    SensorSet sensorsExpected(pCoilMeg);

    SensorSetCreator sensorSetCreator;
    SensorSet sensorsActual = sensorSetCreator.updateSensorSet(m_lChannels,accuracy);

    /// assert
    QVERIFY(sensorsActual==sensorsExpected);
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

