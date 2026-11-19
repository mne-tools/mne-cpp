//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_timebase.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for sample-exact clock domains: mapSampleExact(), verify() and the host-time clock model.
 *
 * Numbers mirror a Spectrum setup: a 10 GHz domain tick, an 80 MS/s receive card (125 ticks per sample)
 * and a 1.25 GS/s transmit card (8 ticks per sample) that schedules in steps of 32 samples.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_clock_model.h>
#include <acq/acq_timebase.h>

#include <cmath>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{
constexpr std::uint64_t kTicksPerSecond = 10'000'000'000ULL;

AcqTimebase receive(std::uint64_t domain)
{
    AcqTimebase tb;
    tb.domain = domain;
    tb.ticksPerSecond = kTicksPerSecond;
    tb.ticksPerSample = 125;
    tb.originTick = 0;
    tb.lock = AcqLockState::PhaseLocked;
    return tb;
}

AcqTimebase transmit(std::uint64_t domain, AcqLockState lock, std::int64_t origin = 0)
{
    AcqTimebase tb;
    tb.domain = domain;
    tb.ticksPerSecond = kTicksPerSecond;
    tb.ticksPerSample = 8;
    tb.sampleAlignment = 32;
    tb.originTick = origin;
    tb.lock = lock;
    return tb;
}

// Loop-back markers: the transmit card emits an edge every `every` samples; its true tick period is
// 8 * (1 + driftPpm * 1e-6) and its true origin trueOrigin. The receive card records the edge.
std::vector<AcqSyncMarker> markers(std::int64_t trueOrigin, double driftPpm, std::uint64_t count, std::uint64_t every)
{
    std::vector<AcqSyncMarker> out;
    for (std::uint64_t i = 0; i < count; ++i) {
        const std::uint64_t txSample = 1000 + i * every;
        const double tick = static_cast<double>(trueOrigin) + static_cast<double>(txSample) * 8.0 * (1.0 + driftPpm * 1e-6);
        const auto whole = static_cast<std::int64_t>(std::floor(tick));
        AcqSyncMarker m;
        m.targetSample = txSample;
        m.referenceSample = static_cast<std::uint64_t>(whole / 125);
        m.subTicks = whole % 125;
        out.push_back(m);
    }
    return out;
}
} // namespace

//=============================================================================================================

class TestAcqTimebase : public QObject
{
    Q_OBJECT

private slots:
    void testDomainId();
    void testMapRequiresPhaseLock();
    void testMapOnGridAndRounding();
    void testMapBeforeStart();
    void testMapRange();
    void testVerifyPromotesDeclared();
    void testVerifyDetectsDrift();
    void testVerifyRejectsInsufficientMarkers();
    void testVerifyFrequencyLockedSingleMarker();
    void testClockModelRoundTrip();
};

//=============================================================================================================

void TestAcqTimebase::testDomainId()
{
    QCOMPARE(acqClockDomainId("spectrum:12345"), acqClockDomainId("spectrum:12345"));
    QVERIFY(acqClockDomainId("spectrum:12345") != acqClockDomainId("spectrum:12346"));
    QVERIFY(acqClockDomainId("") != 0);
}

//=============================================================================================================

void TestAcqTimebase::testMapRequiresPhaseLock()
{
    const auto dom = acqClockDomainId("spectrum:1");
    QVERIFY(mapSampleExact(receive(dom), transmit(dom, AcqLockState::Declared), 8).error() ==
            AcqErrorCode::NotSynchronized);
    QVERIFY(mapSampleExact(receive(dom), transmit(dom, AcqLockState::FrequencyLocked), 8).error() ==
            AcqErrorCode::NotSynchronized);
    QVERIFY(mapSampleExact(receive(dom), transmit(acqClockDomainId("other"), AcqLockState::PhaseLocked), 8).error() ==
            AcqErrorCode::NotSynchronized);
    QVERIFY(mapSampleExact(receive(0), transmit(0, AcqLockState::PhaseLocked), 8).error() ==
            AcqErrorCode::NotSynchronized);
}

//=============================================================================================================

void TestAcqTimebase::testMapOnGridAndRounding()
{
    const auto dom = acqClockDomainId("spectrum:1");
    const AcqTimebase rx = receive(dom);
    const AcqTimebase tx = transmit(dom, AcqLockState::PhaseLocked, 1000);

    auto onGrid = mapSampleExact(rx, tx, 8); // tick 1000 = tx sample 0
    QVERIFY(onGrid);
    QCOMPARE(*onGrid, std::uint64_t(0));
    auto nextStep = mapSampleExact(rx, tx, 8, 256); // 32 tx samples later
    QVERIFY(nextStep);
    QCOMPARE(*nextStep, std::uint64_t(32));
    QVERIFY(mapSampleExact(rx, tx, 8, 8).error() == AcqErrorCode::NotOnGrid);
    QCOMPARE(*mapSampleExact(rx, tx, 8, 8, AcqRounding::Floor), std::uint64_t(0));
    QCOMPARE(*mapSampleExact(rx, tx, 8, 8, AcqRounding::Ceil), std::uint64_t(32));

    // Exactness over a long run: one second of receive samples maps without accumulated error.
    auto far = mapSampleExact(rx, tx, 8 + 80'000'000ULL);
    QVERIFY(far);
    QCOMPARE(*far, std::uint64_t(1'250'000'000ULL));
}

//=============================================================================================================

void TestAcqTimebase::testMapRange()
{
    // Every window of one stream against every sample of the other: a sample belongs to the mapped range
    // exactly when its instant lies in the window's half-open time span. Rates 3:7 share no factor; origins
    // differ in both directions.
    const auto dom = acqClockDomainId("multi");
    for (const std::int64_t origin : {std::int64_t(-10), std::int64_t(0), std::int64_t(5)}) {
        AcqTimebase a = receive(dom);
        a.ticksPerSample = 3;
        AcqTimebase b = receive(dom);
        b.ticksPerSample = 7;
        b.sampleAlignment = 4; // the scheduling grid does not restrict which samples are read
        b.originTick = origin;
        for (std::uint64_t first = 0; first < 40; ++first) {
            for (std::uint64_t end = first; end < first + 12; ++end) {
                auto range = mapRangeExact(a, b, first, end);
                QVERIFY(range);
                std::uint64_t lo = 0;
                std::uint64_t hi = 0;
                bool any = false;
                for (std::uint64_t k = 0; k < 40; ++k) {
                    const std::int64_t t = b.tickOf(k);
                    if (t >= a.tickOf(first) && t < a.tickOf(end)) {
                        lo = any ? lo : k;
                        hi = k + 1;
                        any = true;
                    }
                }
                QCOMPARE(range->end - range->first, hi - lo);
                if (any) {
                    QCOMPARE(range->first, lo);
                }
            }
        }
    }

    // Exact over long runs, and the result does not depend on how a recording is cut into windows.
    AcqTimebase eeg = receive(dom);
    eeg.ticksPerSample = 2; // 1 kHz on a 2 kHz tick
    AcqTimebase fnirs = receive(dom);
    fnirs.ticksPerSample = 200; // 10 Hz
    auto day = mapRangeExact(eeg, fnirs, 0, 86'400'000ULL);
    QVERIFY(day);
    QCOMPARE(day->end, std::uint64_t(864'000));
    auto left = mapRangeExact(eeg, fnirs, 0, 12'345);
    auto right = mapRangeExact(eeg, fnirs, 12'345, 86'400'000ULL);
    QVERIFY(left && right && left->end == right->first && right->end == day->end);

    QVERIFY(mapRangeExact(eeg, fnirs, 5, 4).error() == AcqErrorCode::InvalidParameter);
    QVERIFY(mapRangeExact(eeg, transmit(dom, AcqLockState::Declared), 0, 1).error() == AcqErrorCode::NotSynchronized);
}

//=============================================================================================================

void TestAcqTimebase::testMapBeforeStart()
{
    const auto dom = acqClockDomainId("spectrum:1");
    QVERIFY(mapSampleExact(receive(dom), transmit(dom, AcqLockState::PhaseLocked, 1000), 0).error() ==
            AcqErrorCode::BeforeStart);
}

//=============================================================================================================

void TestAcqTimebase::testVerifyPromotesDeclared()
{
    const auto dom = acqClockDomainId("scanner");
    const std::int64_t trueOrigin = 123'457;
    const auto obs = markers(trueOrigin, 0.0, 5, 1'000'000);
    auto tx = verify(transmit(dom, AcqLockState::Declared), obs, receive(dom), 2);
    QVERIFY(tx);
    QCOMPARE(tx->lock, AcqLockState::PhaseLocked);
    QCOMPARE(tx->originTick, trueOrigin);
    QCOMPARE(tx->originUncertaintyTicks, std::uint64_t(0));

    // After verification the instant of transmit sample 4096 maps back to exactly that sample.
    const AcqTimebase rx = receive(dom);
    const std::uint64_t rxSample = static_cast<std::uint64_t>(tx->tickOf(4'096) / 125);
    auto back = mapSampleExact(rx, *tx, rxSample, tx->tickOf(4'096) - rx.tickOf(rxSample));
    QVERIFY(back);
    QCOMPARE(*back, std::uint64_t(4'096));
}

//=============================================================================================================

void TestAcqTimebase::testVerifyDetectsDrift()
{
    const auto dom = acqClockDomainId("scanner");
    const auto obs = markers(5'000, 1.0, 5, 1'000'000); // 1 ppm: 8 ticks over 10^6 samples
    QVERIFY(verify(transmit(dom, AcqLockState::Declared), obs, receive(dom), 2).error() ==
            AcqErrorCode::NotSynchronized);
}

//=============================================================================================================

void TestAcqTimebase::testVerifyRejectsInsufficientMarkers()
{
    const auto dom = acqClockDomainId("scanner");
    const auto one = markers(5'000, 0.0, 1, 1);
    QVERIFY(verify(transmit(dom, AcqLockState::Declared), one, receive(dom), 2).error() ==
            AcqErrorCode::NotSynchronized);
    const auto close = markers(5'000, 0.0, 3, 10);
    QVERIFY(verify(transmit(dom, AcqLockState::Declared), close, receive(dom), 2).error() ==
            AcqErrorCode::NotSynchronized);
    QVERIFY(verify(transmit(acqClockDomainId("x"), AcqLockState::Declared), markers(5'000, 0.0, 5, 1'000'000),
                   receive(dom), 2)
                .error() == AcqErrorCode::NotSynchronized);
    AcqTimebase unlockedReference = receive(dom);
    unlockedReference.lock = AcqLockState::FrequencyLocked;
    QVERIFY(verify(transmit(dom, AcqLockState::Declared), markers(5'000, 0.0, 5, 1'000'000), unlockedReference, 2)
                .error() == AcqErrorCode::NotSynchronized);
    QVERIFY(verify(transmit(dom, AcqLockState::Free), markers(5'000, 0.0, 5, 1'000'000), receive(dom), 2).error() ==
            AcqErrorCode::NotSynchronized);
}

//=============================================================================================================

void TestAcqTimebase::testVerifyFrequencyLockedSingleMarker()
{
    const auto dom = acqClockDomainId("spectrum:1");
    auto tx = verify(transmit(dom, AcqLockState::FrequencyLocked), markers(777, 0.0, 1, 1), receive(dom), 0);
    QVERIFY(tx);
    QCOMPARE(tx->lock, AcqLockState::PhaseLocked);
    QCOMPARE(tx->originTick, std::int64_t(777));
}

//=============================================================================================================

void TestAcqTimebase::testClockModelRoundTrip()
{
    AcqClockModel in;
    in.sfreq = 1000.0;
    in.anchorSample = 500;
    in.anchorHostNs = 2'000'000'000;
    in.pipelineDelayNs = 3'000'000;
    AcqClockModel out = in;
    out.output = true;
    out.sfreq = 20'000.0;
    out.anchorSample = 0;
    out.anchorHostNs = 1'000'000'000;
    out.pipelineDelayNs = 1'000'000;

    QCOMPARE(in.physicalNs(500), std::int64_t(1'997'000'000));
    QCOMPARE(in.sampleAt(in.physicalNs(1234)), std::uint64_t(1234));
    QCOMPARE(out.sampleAt(out.physicalNs(98'765)), std::uint64_t(98'765));
    // Input sample 500 happened at 1.997 s; the output emits at 1.997 s the sample handed over at 1.996 s.
    QCOMPARE(mapSample(in, out, 500), std::uint64_t(19'920));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqTimebase)
#include "test_acq_timebase.moc"
