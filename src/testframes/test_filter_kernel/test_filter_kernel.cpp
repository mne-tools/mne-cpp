// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>

#include <cmath>
#include <complex>

#include <dsp/filterkernel.h>
#include <dsp/parksmcclellan.h>
#include <dsp/cosinefilter.h>
#include <numbers>

using namespace mne;
using namespace Eigen;

class TestFilterKernel : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FilterKernel construction and getters/setters
    //=========================================================================
    void defaultCtor()
    {
        FilterKernel fk;
        QCOMPARE(fk.name(), std::string("Unknown"));
        QVERIFY(fk.sampling_frequency() >= 0);
    }

    void lpfCtor()
    {
        FilterKernel fk("TestLPF", 0, 128, 0.2, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestLPF"));
        QVERIFY(fk.filter_order() > 0);
        QVERIFY(fk.sampling_frequency() > 0);
    }

    void hpfCtor()
    {
        FilterKernel fk("TestHPF", 1, 128, 0.0, 0.01, 0.005, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestHPF"));
    }

    void bpfCtor()
    {
        FilterKernel fk("TestBPF", 2, 128, 0.02, 0.01, 0.005, 1000.0, 0);
        QCOMPARE(fk.name(), std::string("TestBPF"));
    }

    void gettersSetters()
    {
        FilterKernel fk("Test", 0, 64, 0.2, 0.0, 0.01, 500.0, 0);

        fk.set_name("NewName");
        QCOMPARE(fk.name(), std::string("NewName"));

        fk.set_sampling_frequency(2000.0);
        QVERIFY(std::abs(fk.sampling_frequency() - 2000.0) < 0.1);

        fk.set_filter_order(256);
        QCOMPARE(fk.filter_order(), 256);

        fk.set_center_frequency(0.1);
        QVERIFY(std::abs(fk.center_frequency() - 0.1) < 0.01);

        fk.set_bandwidth(0.05);
        QVERIFY(std::abs(fk.bandwidth() - 0.05) < 0.01);

        fk.set_parks_width(0.02);
        QVERIFY(std::abs(fk.parks_width() - 0.02) < 0.01);

        fk.set_highpass_freq(5.0);
        QVERIFY(std::abs(fk.highpass_freq() - 5.0) < 0.1);

        fk.set_lowpass_freq(40.0);
        QVERIFY(std::abs(fk.lowpass_freq() - 40.0) < 0.1);
    }

    void shortDescription()
    {
        // The pass band runs from the lower to the upper edge: 0 to the cutoff for a low-pass, the cutoff to Nyquist for a high-pass
        const FilterKernel lowPass("Desc", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(lowPass.short_description(), std::string("Cosine  -  0Hz to 100Hz  -  Ord: 64"));
        QCOMPARE(lowPass.highpass_freq(), 0.0);
        QCOMPARE(lowPass.lowpass_freq(), 100.0);
        const FilterKernel highPass("Desc", 1, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(highPass.short_description(), std::string("Cosine  -  100Hz to 500Hz  -  Ord: 64"));
        QCOMPARE(highPass.highpass_freq(), 100.0);
        QCOMPARE(highPass.lowpass_freq(), 500.0);
    }

    void coefficients()
    {
        FilterKernel fk("Coeff", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        const RowVectorXd& coeffs = fk.coefficients();
        QVERIFY(coeffs.size() > 0);
    }

    //=========================================================================
    // FilterKernel - prepare and apply
    //=========================================================================
    void prepareAndApplyConv()
    {
        FilterKernel fk("ConvTest", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        int N = 256;
        fk.prepare_filter(N);

        RowVectorXd data = RowVectorXd::Random(N);
        RowVectorXd filtered = fk.apply_conv_filter(data, false);
        QCOMPARE(filtered.size(), N);
    }

    void prepareAndApplyFft()
    {
        FilterKernel fk("FftTest", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        int N = 512;
        fk.prepare_filter(N);

        RowVectorXd data = RowVectorXd::Random(N);
        fk.apply_fft_filter(data, false);
        QCOMPARE(data.size(), N);
    }

    void designMethods()
    {
        FilterKernel fkCosine("Cosine", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        QVERIFY(fkCosine.coefficients().size() > 0);

        FilterKernel fkParks("Parks", 0, 64, 0.2, 0.0, 0.01, 1000.0, 1);
        QVERIFY(fkParks.coefficients().size() > 0);
    }

    void cosineBandPassHasHalfAmplitudeAtBothEdges()
    {
        // Like MNE-C mne_apply_filter: cos^2 tapers centred on 45 and 55 Hz, flat in between
        const RowVectorXd h = FilterKernel("BPF", 2, 511, 50.0 / 500.0, 10.0 / 500.0, 0.01, 1000.0, 0).coefficients();
        auto gain = [&h](double hz) {
            std::complex<double> response = 0.0;
            for (Index t = 0; t < h.size(); ++t) {
                response += h(t) * std::polar(1.0, -2.0 * std::numbers::pi * hz * static_cast<double>(t) / 1000.0);
            }
            return std::abs(response);
        };
        for (const double edge : {45.0, 55.0}) {
            QVERIFY2(std::abs(gain(edge) - 0.5) < 0.15, qPrintable(QString("%1 Hz: %2").arg(edge).arg(gain(edge))));
        }
        QVERIFY(std::abs(gain(50.0) - 1.0) < 0.05);
        QVERIFY(gain(35.0) < 0.05 && gain(65.0) < 0.05);

        // A pass band above Nyquist passes nothing
        const RowVectorXd none = FilterKernel("BPF", 2, 256, 40.0, 1.0, 0.0, 1000.0, 0).coefficients();
        QVERIFY(none.allFinite() && none.isZero());
    }

    void notchRemovesItsCenterFrequency_data()
    {
        QTest::addColumn<int>("designMethod");
        QTest::newRow("Cosine") << FilterKernel::design_method_index("Cosine");
        QTest::newRow("Tschebyscheff") << FilterKernel::design_method_index("Tschebyscheff");
    }

    void notchRemovesItsCenterFrequency()
    {
        QFETCH(int, designMethod);
        // A 50 Hz notch, 20 Hz wide, at 1 kHz; odd order as the Parks-McClellan notch requires
        const double sFreq = 1000.0;
        FilterKernel notch("Notch", FilterKernel::filter_type_index("NOTCH"), 255, 50.0 / 500.0, 20.0 / 500.0, 0.01, sFreq,
                           designMethod);
        const int n = 4096;
        auto amplitude = [&](double hz) {
            RowVectorXd data(n);
            for (int t = 0; t < n; ++t) {
                data(t) = std::sin(2.0 * std::numbers::pi * hz * t / sFreq);
            }
            notch.prepare_filter(n);
            notch.apply_fft_filter(data);
            return data.segment(n / 4, n / 2).cwiseAbs().maxCoeff();
        };
        QVERIFY2(amplitude(50.0) < 0.1, qPrintable(QString::number(amplitude(50.0))));
        QVERIFY2(amplitude(10.0) > 0.9, qPrintable(QString::number(amplitude(10.0))));
        QVERIFY2(amplitude(150.0) > 0.9, qPrintable(QString::number(amplitude(150.0))));
    }

    void cosineKernelOfOddOrderIsSymmetric()
    {
        // The cosine design has a real (zero-phase) response, so its taps are symmetric about the centre tap
        const RowVectorXd taps = FilterKernel("LPF", 0, 63, 0.1, 0.0, 0.02, 1000.0, 0).coefficients();
        QCOMPARE(taps.size(), Index(63));
        QVERIFY2((taps - taps.reverse()).cwiseAbs().maxCoeff() < 1e-12 * taps.cwiseAbs().maxCoeff(),
                 qPrintable(QString::number((taps - taps.reverse()).cwiseAbs().maxCoeff())));
        QCOMPARE(taps.cwiseAbs().maxCoeff(), std::abs(taps(31)));
    }

    void filterParameter()
    {
        FilterParameter fp;
        QCOMPARE(fp.name(), std::string("Unknown"));

        FilterParameter fp2("TestParam");
        QCOMPARE(fp2.name(), std::string("TestParam"));

        FilterParameter fp3("Param", "Description");
        QCOMPARE(fp3.name(), std::string("Param"));

        QVERIFY(fp2 == FilterParameter("TestParam"));
    }

    //=========================================================================
    // ParksMcClellan
    //=========================================================================
    void parksMcClellan_basic()
    {
        ParksMcClellan pm(33, 0.3, 0.2, 0.1, ParksMcClellan::LPF);
        QVERIFY(pm.coefficients.size() > 0);
    }

    //=========================================================================
    // CosineFilter
    //=========================================================================
    void cosineFilter_lpf()
    {
        CosineFilter cf(512, 40.0f, 5.0f, 0.0f, 0.0f, 1000.0, CosineFilter::LPF);
        QVERIFY(cf.m_vecCoeff.size() > 0);
    }

    void cosineFilter_hpf()
    {
        CosineFilter cf(512, 0.0f, 0.0f, 5.0f, 3.0f, 1000.0, CosineFilter::HPF);
        QVERIFY(cf.m_vecCoeff.size() > 0);
    }
};

QTEST_GUILESS_MAIN(TestFilterKernel)
#include "test_filter_kernel.moc"
