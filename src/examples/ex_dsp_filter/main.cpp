//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    FIR filter design, application and storage with the dsp library.
 *
 * A 40 Hz low-pass is designed with both design methods, its frequency
 * response is evaluated from the coefficients, applied to a 10 Hz + 100 Hz
 * signal at once and block by block with overlap-add, and round-tripped
 * through the text filter format. The example exits non-zero if the filter
 * does not pass 10 Hz, does not stop 100 Hz, or the variants disagree.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/cosinefilter.h>
#include <dsp/filterio.h>
#include <dsp/filterkernel.h>
#include <dsp/parksmcclellan.h>
#include <dsp/rt/rt_filter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <complex>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 1000.0;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================
/**
 * |H(f)| of an FIR filter evaluated directly from its taps.
 */
double gain(const RowVectorXd& taps, double freq, double sFreq)
{
    std::complex<double> h(0.0, 0.0);
    for (int k = 0; k < taps.size(); ++k) {
        h += taps(k) * std::polar(1.0, -2.0 * kPi * freq * k / sFreq);
    }
    return std::abs(h);
}

//=============================================================================================================

RowVectorXd tone(double freq, int n)
{
    RowVectorXd x(n);
    for (int t = 0; t < n; ++t) {
        x(t) = std::sin(2.0 * kPi * freq * t / kSFreq);
    }
    return x;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    //! [filter_kernel_design]
    // Frequencies are normalised to Nyquist (500 Hz): 40 Hz cut-off, 10 Hz transition
    const int typeLpf = FilterKernel::m_filterTypes.indexOf(FilterParameter("LPF"));
    const int cosine = FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine"));
    const int tschebyscheff = FilterKernel::m_designMethods.indexOf(FilterParameter("Tschebyscheff"));
    FilterKernel lowPass("lp40", typeLpf, 256, 40.0 / 500.0, 0.0, 10.0 / 500.0, kSFreq, cosine);
    FilterKernel lowPassPm("lp40pm", typeLpf, 256, 40.0 / 500.0, 0.0, 10.0 / 500.0, kSFreq, tschebyscheff);
    //! [filter_kernel_design]
    for (const FilterKernel* kernel : {&lowPass, &lowPassPm}) {
        const RowVectorXd taps = kernel->getCoefficients();
        const double g10 = gain(taps, 10.0, kSFreq);
        const double g100 = gain(taps, 100.0, kSFreq);
        ok &= expect(std::fabs(g10 - 1.0) < 0.01 && g100 < 0.01,
                     QString("%1 (%2): |H(10 Hz)| %3, |H(100 Hz)| %4").arg(kernel->getName(), kernel->getDesignMethod().getName()).arg(g10).arg(g100));
    }

    //! [cosine_filter_design]
    // CosineFilter works in absolute Hz: low-pass edge 40 Hz with a 10 Hz slope, FFT length 1024
    CosineFilter cosineLp(1024, 40.0f, 10.0f, 0.0f, 0.0f, kSFreq, CosineFilter::LPF);
    const RowVectorXcd& response = cosineLp.m_vecFftCoeff; // one-sided response, bin k at k * 1000 / 1024 Hz
    //! [cosine_filter_design]
    // Cosine-squared slope from 35 to 45 Hz: unity well below, zero well above
    const double c10 = std::abs(response(10));
    const double c40 = std::abs(response(41));
    const double c100 = std::abs(response(102));
    ok &= expect(response.size() == 513 && c10 == 1.0 && c40 > 0.3 && c40 < 0.7 && c100 == 0.0,
                 QString("CosineFilter response at 10/40/100 Hz: %1 / %2 / %3").arg(c10).arg(c40).arg(c100));

    //! [parks_mcclellan_design]
    // Equiripple 65-tap low-pass, corner 0.08 pi (40 Hz at 1 kHz), transition 0.04 pi
    ParksMcClellan parksMcClellan(65, 0.08, 0.0, 0.04, ParksMcClellan::LPF);
    //! [parks_mcclellan_design]
    const double pm10 = gain(parksMcClellan.FirCoeff, 10.0, kSFreq);
    const double pm100 = gain(parksMcClellan.FirCoeff, 100.0, kSFreq);
    ok &= expect(parksMcClellan.FirCoeff.size() == 65 && std::fabs(pm10 - 1.0) < 0.05 && pm100 < 0.05,
                 QString("ParksMcClellan 65 taps: |H(10 Hz)| %1, |H(100 Hz)| %2").arg(pm10).arg(pm100));

    const int n = 4000;
    MatrixXd data(2, n);
    data.row(0) = tone(10.0, n) + tone(100.0, n);
    data.row(1) = 0.5 * tone(10.0, n) + tone(100.0, n);

    //! [filter_data_usage]
    const MatrixXd filtered = filterData(data, lowPass); // zero-phase: the filter delay is removed
    //! [filter_data_usage]
    const double filterErr = (filtered.row(0) - tone(10.0, n)).segment(500, n - 1000).cwiseAbs().maxCoeff();
    ok &= expect(filtered.cols() == n && filterErr < 0.02,
                 QString("filterData removes 100 Hz and keeps 10 Hz in phase (max error %1)").arg(filterErr));

    //! [filter_overlap_add_usage]
    FilterOverlapAdd overlapAdd; // keeps the filter tail between calls
    const int block = 1000;
    MatrixXd streamed(2, n);
    for (int start = 0; start < n; start += block) {
        streamed.middleCols(start, block) = overlapAdd.calculate(data.middleCols(start, block), lowPass);
    }
    //! [filter_overlap_add_usage]
    // Streaming output is delayed by half the filter length; compare the steady state with the causal tone
    const int delay = lowPass.getFilterOrder() / 2;
    RowVectorXd delayedTone(n);
    for (int t = 0; t < n; ++t) {
        delayedTone(t) = std::sin(2.0 * kPi * 10.0 * (t - delay) / kSFreq);
    }
    const double streamErr = (streamed.row(0) - delayedTone).segment(1000, n - 1500).cwiseAbs().maxCoeff();
    ok &= expect(streamErr < 0.02, QString("overlap-add over 4 blocks equals the delayed 10 Hz tone (max error %1)").arg(streamErr));

    //! [filter_io_round_trip]
    QTemporaryDir dir;
    const QString path = dir.filePath("lp40.txt");
    FilterKernel reloaded;
    const bool written = FilterIO::writeFilter(path, lowPass);
    const bool read = FilterIO::readFilter(path, reloaded);
    //! [filter_io_round_trip]
    const double ioErr = (reloaded.getCoefficients() - lowPass.getCoefficients()).cwiseAbs().maxCoeff();
    ok &= expect(written && read && reloaded.getCoefficients().size() == lowPass.getCoefficients().size() && ioErr < 1e-6,
                 QString("FilterIO round trip keeps %1 taps (max error %2)").arg(reloaded.getCoefficients().size()).arg(ioErr));

    qInfo() << (ok ? "All dsp filter checks passed." : "dsp filter checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
