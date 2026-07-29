//=============================================================================================================
/**
 * @file     test_scmeas_measurements.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     February, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MNE-CPP AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Checks the mne_scan real time measurement types.
 *
 * The scMeas types are the containers that carry acquired data between
 * mne_scan plugins. None of them had a test. Unlike a widget, these are not
 * only worth constructing: they have accessor pairs that can be asserted
 * properly, so this checks that what is stored is what comes back rather than
 * merely that the object can be built.
 *
 * The emphasis is on the paths a live acquisition actually takes. A sampling
 * rate or channel count that fails to round trip silently mislabels every
 * sample that follows, and a value set before any channel info is attached is
 * exactly the state a plugin is in during startup.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimespectrum.h>
#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimefwdsolution.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/numeric.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QScopedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestScMeasMeasurements
 *
 * @brief Checks the mne_scan real time measurement containers.
 */
class TestScMeasMeasurements: public QObject
{
    Q_OBJECT

public:
    TestScMeasMeasurements() = default;

private slots:
    void rtmsa_scalarsRoundTrip_data();
    void rtmsa_scalarsRoundTrip();
    void rtmsa_nameAndLayoutRoundTrip();
    void rtmsa_valueBeforeChannelInfoIsSafe();
    void evokedSet_layoutRoundTrip();
    void numeric_valueRoundTrips();
    void construct_remainingMeasurementTypes();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestScMeasMeasurements::rtmsa_scalarsRoundTrip_data()
{
    QTest::addColumn<double>("samplingRate");
    QTest::addColumn<int>("multiArraySize");

    // A sampling rate that does not survive a round trip silently mislabels
    // the time axis of everything acquired afterwards, so the boundary values
    // are checked as well as an ordinary one.
    QTest::newRow("typical meg") << 1000.0 << 10;
    QTest::newRow("low rate")    <<    1.0 <<  1;
    QTest::newRow("high rate")   << 5000.0 << 64;
    QTest::newRow("fractional")  <<  600.615 << 5;
}

//=============================================================================================================

void TestScMeasMeasurements::rtmsa_scalarsRoundTrip()
{
    QFETCH(double, samplingRate);
    QFETCH(int, multiArraySize);

    RealTimeMultiSampleArray rtmsa;

    rtmsa.setSamplingRate(static_cast<float>(samplingRate));
    rtmsa.setMultiArraySize(multiArraySize);

    QCOMPARE(static_cast<double>(rtmsa.getSamplingRate()), static_cast<double>(static_cast<float>(samplingRate)));
    QCOMPARE(static_cast<int>(rtmsa.getMultiArraySize()), multiArraySize);
}

//=============================================================================================================

void TestScMeasMeasurements::rtmsa_nameAndLayoutRoundTrip()
{
    RealTimeMultiSampleArray rtmsa;

    // The layout file is what the display uses to place channels. An empty
    // string is the startup state and has to be handled, not just a real path.
    QCOMPARE(rtmsa.getXMLLayoutFile(), QString(""));

    rtmsa.setXMLLayoutFile("babymeg-mag-inner-layer.lout");
    QCOMPARE(rtmsa.getXMLLayoutFile(), QString("babymeg-mag-inner-layer.lout"));

    rtmsa.setXMLLayoutFile("");
    QCOMPARE(rtmsa.getXMLLayoutFile(), QString(""));

    // With no channel info attached the array reports no channels rather than
    // an uninitialised count.
    QCOMPARE(rtmsa.getNumChannels(), 0u);
}

//=============================================================================================================

void TestScMeasMeasurements::rtmsa_valueBeforeChannelInfoIsSafe()
{
    // This is the state every plugin passes through at startup: a value
    // arrives before channel info has been attached. It has to be survivable,
    // because the alternative is a crash on the first acquired block.
    RealTimeMultiSampleArray rtmsa;

    MatrixXd mat(4, 8);
    mat.setConstant(1.0);

    rtmsa.setValue(mat);

    // Nothing to assert about stored samples without channel info, but the
    // object must remain usable afterwards rather than being left broken.
    rtmsa.setSamplingRate(500.0f);
    QCOMPARE(static_cast<double>(rtmsa.getSamplingRate()), 500.0);
}

//=============================================================================================================

void TestScMeasMeasurements::evokedSet_layoutRoundTrip()
{
    RealTimeEvokedSet rtes;

    QCOMPARE(rtes.getXMLLayoutFile(), QString(""));

    rtes.setXMLLayoutFile("vectorview-all.lout");
    QCOMPARE(rtes.getXMLLayoutFile(), QString("vectorview-all.lout"));

    QCOMPARE(rtes.getNumChannels(), 0u);
}

//=============================================================================================================

void TestScMeasMeasurements::numeric_valueRoundTrips()
{
    Numeric num;

    num.setValue(42.5);
    QCOMPARE(num.getValue(), 42.5);

    // Zero and a negative value are distinct from "unset" and must be stored
    // as given rather than treated as absent.
    num.setValue(0.0);
    QCOMPARE(num.getValue(), 0.0);

    num.setValue(-17.25);
    QCOMPARE(num.getValue(), -17.25);
}

//=============================================================================================================

void TestScMeasMeasurements::construct_remainingMeasurementTypes()
{
    // The remaining types carry data that needs a full acquisition setup to
    // populate, so this only checks that they construct and destruct cleanly.
    // That is shallow, and is here because these types are instantiated only
    // by mne_scan plugins, where a fault in construction reaches a user first.
    {
        RealTimeCov cov;
        Q_UNUSED(cov)
    }
    {
        RealTimeSpectrum spectrum;
        Q_UNUSED(spectrum)
    }
    {
        RealTimeHpiResult hpi;
        Q_UNUSED(hpi)
    }
    {
        RealTimeFwdSolution fwd;
        Q_UNUSED(fwd)
    }
    {
        RealTimeSourceEstimate srcEst;
        Q_UNUSED(srcEst)
    }
    {
        RealTimeConnectivityEstimate conn;
        Q_UNUSED(conn)
    }

    // Reaching here means every type above constructed and destructed without
    // faulting, which is the whole claim of this case.
    QVERIFY(true);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestScMeasMeasurements)
#include "test_scmeas_measurements.moc"
