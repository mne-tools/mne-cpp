// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>
#include <dsp/rt/rt_detect_trigger.h>
#include <dsp/parksmcclellan.h>
#include <dsp/filterkernel.h>
#include <dsp/rt/rt_filter.h>
#include <numbers>

using namespace mne;
using namespace Eigen;

class TestRtProcessingExtended : public QObject
{
    Q_OBJECT

private slots:
    // ==================== DetectTrigger tests ====================
    void testDetectTriggerFlanksMaxSingleChannel()
    {
        MatrixXd data = MatrixXd::Zero(1, 200);
        data.block(0, 50, 1, 3).setConstant(5.0);
        data.block(0, 150, 1, 2).setConstant(5.0);

        // The burst length skips the rest of each pulse; the offset index shifts the reported samples
        const std::vector<TriggerFlank> triggers = detect_trigger_flanks_max(data, 0, 1000, 1.0, true, 5);
        QVERIFY(triggers == (std::vector<TriggerFlank>{{1050, 5.0}, {1150, 5.0}}));
    }

    void testDetectTriggerFlanksMaxMultiChannel()
    {
        MatrixXd data = MatrixXd::Zero(3, 200);
        data.block(1, 70, 1, 3).setConstant(10.0);
        data(2, 120) = 4.0;

        const auto triggers = detect_trigger_flanks_max(data, std::vector<int>{1, 2}, 0, 1.0, true, 5);
        QVERIFY(triggers.at(1) == (std::vector<TriggerFlank>{{70, 10.0}}));
        QVERIFY(triggers.at(2) == (std::vector<TriggerFlank>{{120, 4.0}}));
    }

    void testDetectTriggerFlanksGradSingleChannel()
    {
        MatrixXd data = MatrixXd::Zero(1, 200);
        data.block(0, 50, 1, 5).setConstant(10.0);

        const auto triggers = detect_trigger_flanks_grad(data, 0, 0, 1.0, FlankDirection::Rising, 5);
        QVERIFY(triggers == (std::vector<TriggerFlank>{{50, 10.0}}));

        // A change has no offset: a 2 V step on a line idling at 5 V is found
        MatrixXd idling = MatrixXd::Constant(1, 200, 5.0);
        idling.rightCols(100).array() += 2.0;
        QVERIFY(detect_trigger_flanks_grad(idling, 0, 0, 1.0, FlankDirection::Rising, 5) == (std::vector<TriggerFlank>{{100, 2.0}}));
    }

    void testDetectTriggerFlanksGradFalling()
    {
        MatrixXd data = MatrixXd::Constant(1, 200, 10.0);
        data.rightCols(150).setZero();

        const auto triggers = detect_trigger_flanks_grad(data, 0, 0, 1.0, FlankDirection::Falling, 5);
        QVERIFY(triggers == (std::vector<TriggerFlank>{{50, 10.0}}));
    }

    void testDetectTriggerFlanksGradMultiChannel()
    {
        MatrixXd data = MatrixXd::Zero(3, 200);
        data.block(2, 80, 1, 5).setConstant(10.0);
        data.block(0, 30, 1, 5).setConstant(3.0);

        const auto triggers = detect_trigger_flanks_grad(data, std::vector<int>{2, 0}, 0, 1.0, FlankDirection::Rising, 5);
        QVERIFY(triggers.at(2) == (std::vector<TriggerFlank>{{80, 10.0}}));
        QVERIFY(triggers.at(0) == (std::vector<TriggerFlank>{{30, 3.0}}));
    }

    void testDetectTriggerFlanksRejectsChannelsOutsideTheData()
    {
        // Row index == rows() is outside the data; for a single-row matrix it would alias the next column
        MatrixXd data = MatrixXd::Zero(1, 200);
        data(0, 100) = 5.0;
        QVERIFY(detect_trigger_flanks_max(data, 1, 0, 1.0, false, 5).empty());
        QVERIFY(detect_trigger_flanks_grad(data, 1, 0, 1.0, FlankDirection::Rising, 5).empty());

        // An invalid channel must not stop detection on the channels listed after it
        const std::vector<int> channels{1, 0};
        const auto max = detect_trigger_flanks_max(data, channels, 0, 1.0, false, 5);
        QVERIFY(max.at(1).empty());
        QVERIFY(max.at(0) == (std::vector<TriggerFlank>{{100, 5.0}}));
        const auto grad = detect_trigger_flanks_grad(data, channels, 0, 1.0, FlankDirection::Rising, 5);
        QVERIFY(grad.at(1).empty());
        QVERIFY(grad.at(0) == (std::vector<TriggerFlank>{{100, 5.0}}));
    }

    void testToEventMatrix()
    {
        // One event matrix per channel, in channel order: sample, 0, trigger value
        const std::map<int, std::vector<TriggerFlank>> triggers{{0, {{100, 5.0}, {200, 10.0}}}, {1, {{300, 2.0}}}};
        const std::vector<MatrixXi> events = to_event_matrix(triggers);
        QCOMPARE(events.size(), std::size_t(2));
        QVERIFY(events[0] == (MatrixXi(2, 3) << 100, 0, 5, 200, 0, 10).finished());
        QVERIFY(events[1] == (MatrixXi(1, 3) << 300, 0, 2).finished());
    }

    // ==================== ParksMcClellan tests ====================

    void testParksMcClellanLPF()
    {
        int numTaps = 51;
        double omegaC = 0.25; // Cutoff at Nyquist/4
        ParksMcClellan pm(numTaps, omegaC, 0.0, 0.01, ParksMcClellan::LPF);
        // Verify coefficients exist and have correct length
        QCOMPARE(static_cast<int>(pm.coefficients.size()), numTaps);
        // Coefficients should be symmetric (linear phase FIR)
        for (int i = 0; i < numTaps / 2; ++i) {
            QVERIFY(std::abs(pm.coefficients[i] - pm.coefficients[numTaps - 1 - i]) < 1e-10);
        }
    }

    void testParksMcClellanHPF()
    {
        int numTaps = 51; // Must be odd for HPF
        double omegaC = 0.25;
        ParksMcClellan pm(numTaps, omegaC, 0.0, 0.01, ParksMcClellan::HPF);
        QCOMPARE(static_cast<int>(pm.coefficients.size()), numTaps);
    }

    void testParksMcClellanBPF()
    {
        int numTaps = 51;
        double omegaC = 0.25;
        double bw = 0.1;
        ParksMcClellan pm(numTaps, omegaC, bw, 0.01, ParksMcClellan::BPF);
        QCOMPARE(static_cast<int>(pm.coefficients.size()), numTaps);
    }

    void testParksMcClellanNOTCH()
    {
        int numTaps = 51;
        double omegaC = 0.25;
        double bw = 0.1;
        ParksMcClellan pm(numTaps, omegaC, bw, 0.01, ParksMcClellan::NOTCH);
        QCOMPARE(static_cast<int>(pm.coefficients.size()), numTaps);
    }

    void testParksMcClellanDifferentTapSizes()
    {
        // Test various tap sizes
        for (int taps : {11, 21, 31, 65, 101}) {
            ParksMcClellan pm(taps, 0.3, 0.0, 0.01, ParksMcClellan::LPF);
            QCOMPARE(static_cast<int>(pm.coefficients.size()), taps);
        }
    }

    // ==================== FilterKernel tests ====================
    void testFilterKernelDefaultCtor()
    {
        FilterKernel fk;
        QCOMPARE(fk.name(), std::string("Unknown"));
    }

    void testFilterKernelLPF()
    {
        FilterKernel fk("TestLPF",
                        0,      // LPF type
                        64,     // filter order
                        0.1,    // center frequency (normed to sFreq/2)
                        0.0,    // bandwidth (ignored for LPF)
                        0.01,   // transition bandwidth
                        1000.0, // sampling frequency
                        0);     // design method = Parks-McClellan

        QCOMPARE(fk.name(), std::string("TestLPF"));
        QVERIFY(fk.filter_order() > 0);
        QVERIFY(fk.sampling_frequency() > 0);
        QVERIFY(fk.coefficients().size() > 0);
    }

    void testFilterKernelHPF()
    {
        FilterKernel fk("TestHPF", 1, 64, 0.3, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestHPF"));
        QVERIFY(fk.coefficients().size() > 0);
    }

    void testFilterKernelBPF()
    {
        FilterKernel fk("TestBPF", 2, 64, 0.25, 0.1, 0.01, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestBPF"));
    }

    void testFilterKernelGetters()
    {
        FilterKernel fk("TestFilter", 0, 64, 0.1, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestFilter"));
        QVERIFY(fk.sampling_frequency() - 1000.0 < 0.1);
        QVERIFY(fk.filter_order() > 0);
        QVERIFY(!fk.short_description().empty());

        QCOMPARE(fk.design_method().name(), std::string("Cosine"));
        QCOMPARE(fk.filter_type().name(), std::string("LPF"));
    }

    void testFilterKernelSetters()
    {
        FilterKernel fk;
        fk.set_design_method(1);
        fk.set_filter_type(1);
        QCOMPARE(fk.design_method().name(), std::string("Tschebyscheff"));
        QCOMPARE(fk.filter_type().name(), std::string("HPF"));

        // Unknown indices (e.g. index_of() of an unknown name) fall back to the first entry
        fk.set_design_method(-1);
        fk.set_filter_type(99);
        QCOMPARE(fk.design_method().name(), std::string("Cosine"));
        QCOMPARE(fk.filter_type().name(), std::string("LPF"));
    }

    void testFilterKernelPrepareFilter()
    {
        // Coefficients are transformed for the next power of two that holds data plus filter
        FilterKernel fk("TestLPF", 0, 64, 0.1, 0.0, 0.01, 1000.0, 0);
        fk.prepare_filter(256);
        QCOMPARE(fk.fft_coefficients().cols(), Eigen::Index(512 / 2 + 1));
    }

    void testFilterKernelApplyConvFilter()
    {
        FilterKernel fk("TestLPF", 0, 32, 0.1, 0.0, 0.01, 1000.0, 0);
        RowVectorXd signal = RowVectorXd::Random(256);
        RowVectorXd filtered = fk.apply_conv_filter(signal, false);
        QVERIFY(filtered.size() > 0);
    }

    void testFilterKernelApplyFftFilter()
    {
        FilterKernel fk("TestLPF", 0, 32, 0.1, 0.0, 0.01, 1000.0, 0);
        fk.prepare_filter(256);
        RowVectorXd signal = RowVectorXd::Random(256);
        fk.apply_fft_filter(signal, false);
        // signal is filtered in-place
        QVERIFY(signal.size() == 256);
    }

    // ==================== Filter function tests ====================
    void testFilterDataWithParameters()
    {
        int nCh = 3, nSamples = 512;
        MatrixXd data = MatrixXd::Random(nCh, nSamples);

        // LPF at 100 Hz, sFreq 1000
        MatrixXd filtered = filterData(data, 0, 100.0, 0.0, 5.0, 1000.0, 64, 0);
        QCOMPARE(filtered.rows(), nCh);
        QVERIFY(filtered.cols() > 0);
    }

    void testFilterDataWithKernel()
    {
        int nCh = 3, nSamples = 512;
        MatrixXd data = MatrixXd::Random(nCh, nSamples);

        FilterKernel fk("LPF", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        // Provide explicit picks to avoid LinSpaced bug
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i)
            picks(i) = i;
        MatrixXd filtered = filterData(data, fk, picks);
        QCOMPARE(filtered.rows(), nCh);
    }

    void testFilterDataBlock()
    {
        int nCh = 3, nSamples = 256;
        MatrixXd data = MatrixXd::Random(nCh, nSamples);

        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);
        // Provide explicit picks to avoid LinSpaced bug with empty picks
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i)
            picks(i) = i;
        MatrixXd filtered = filterDataBlock(data, picks, fk, false);
        QCOMPARE(filtered.rows(), nCh);
    }

    void testFilterDataLowPassEffect()
    {
        // Create a signal with low freq + high freq component
        int nSamples = 1024;
        int nCh = 3;
        double sFreq = 1000.0;
        MatrixXd data(nCh, nSamples);
        for (int i = 0; i < nSamples; ++i) {
            double t = i / sFreq;
            data(0, i) = std::sin(2.0 * std::numbers::pi * 10.0 * t) +
                std::sin(2.0 * std::numbers::pi * 200.0 * t);
            data(1, i) = data(0, i);
            data(2, i) = data(0, i);
        }

        // Provide explicit picks to avoid LinSpaced bug in filterData
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i)
            picks(i) = i;

        // Low-pass at 50 Hz
        MatrixXd filtered = filterData(data, 0, 50.0, 0.0, 5.0, sFreq, 128, 0, picks);

        // The 200 Hz component should be attenuated
        double origVar = data.block(0, 256, 1, 512).squaredNorm() / 512.0;
        double filtVar = filtered.block(0, 256, 1, 512).squaredNorm() / 512.0;
        QVERIFY(filtVar < origVar);
    }

    void testFilterOverlapAdd()
    {
        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);
        FilterOverlapAdd filter;

        int nCh = 2;
        RowVectorXi picks(nCh);
        for (int i = 0; i < nCh; ++i)
            picks(i) = i;

        // Process two blocks with explicit picks
        MatrixXd block1 = MatrixXd::Random(nCh, 256);
        MatrixXd result1 = filter.calculate(block1, fk, picks);
        QCOMPARE(result1.rows(), nCh);

        MatrixXd block2 = MatrixXd::Random(nCh, 256);
        MatrixXd result2 = filter.calculate(block2, fk, picks);
        QCOMPARE(result2.rows(), nCh);

        // After a reset the next block is filtered like a first block
        filter.reset();
        FilterOverlapAdd fresh;
        QCOMPARE(filter.calculate(block2, fk, picks), fresh.calculate(block2, fk, picks));
    }

    void testFilterOverlapAddWithPicks()
    {
        FilterKernel fk("LPF", 0, 32, 0.2, 0.0, 0.01, 1000.0, 0);
        FilterOverlapAdd filter;

        RowVectorXi picks(2);
        picks << 0, 2;

        MatrixXd block = MatrixXd::Random(3, 256);
        MatrixXd result = filter.calculate(block, fk, picks);
        QCOMPARE(result.rows(), 3);
    }
};

QTEST_GUILESS_MAIN(TestRtProcessingExtended)
#include "test_dsp_rt_extended.moc"
