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

#include <mne/dsp/cosinefilter.h>
#include <mne/dsp/filterio.h>
#include <mne/dsp/filterkernel.h>
#include <mne/dsp/parksmcclellan.h>
#include <mne/dsp/rt/rt_detect_trigger.h>
#include <mne/dsp/rt/rt_filter.h>

#include <mne/disp_widgets/plots/helpers/figureexporter.h>
#include <mne/disp_widgets/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <complex>
#include <cstdlib>
#include <format>
#include <print>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 1000.0;

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
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
    QApplication app(argc, argv);
    bool ok = true;

    //! [filter_kernel_design]
    // Frequencies are normalised to Nyquist (500 Hz): 40 Hz cut-off, 10 Hz transition
    const int typeLpf = FilterKernel::filter_type_index("LPF");
    const int cosine = FilterKernel::design_method_index("Cosine");
    const int tschebyscheff = FilterKernel::design_method_index("Tschebyscheff");
    FilterKernel lowPass("lp40", typeLpf, 256, 40.0 / 500.0, 0.0, 10.0 / 500.0, kSFreq, cosine);
    FilterKernel lowPassPm("lp40pm", typeLpf, 256, 40.0 / 500.0, 0.0, 10.0 / 500.0, kSFreq, tschebyscheff);
    //! [filter_kernel_design]
    for (const FilterKernel* kernel : {&lowPass, &lowPassPm}) {
        const RowVectorXd taps = kernel->coefficients();
        const double g10 = gain(taps, 10.0, kSFreq);
        const double g100 = gain(taps, 100.0, kSFreq);
        ok &= expect(std::fabs(g10 - 1.0) < 0.01 && g100 < 0.01,
                     std::format("{} ({}): |H(10 Hz)| {:g}, |H(100 Hz)| {:g}", kernel->name(), kernel->design_method().name(), g10, g100));
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
                 std::format("CosineFilter response at 10/40/100 Hz: {:g} / {:g} / {:g}", c10, c40, c100));

    //! [parks_mcclellan_design]
    // Equiripple 65-tap low-pass, corner 0.08 pi (40 Hz at 1 kHz), transition 0.04 pi
    ParksMcClellan parksMcClellan(65, 0.08, 0.0, 0.04, ParksMcClellan::LPF);
    //! [parks_mcclellan_design]
    const double pm10 = gain(parksMcClellan.coefficients, 10.0, kSFreq);
    const double pm100 = gain(parksMcClellan.coefficients, 100.0, kSFreq);
    ok &= expect(parksMcClellan.coefficients.size() == 65 && std::fabs(pm10 - 1.0) < 0.05 && pm100 < 0.05,
                 std::format("ParksMcClellan 65 taps: |H(10 Hz)| {:g}, |H(100 Hz)| {:g}", pm10, pm100));

    const int n = 4000;
    MatrixXd data(2, n);
    data.row(0) = tone(10.0, n) + tone(100.0, n);
    data.row(1) = 0.5 * tone(10.0, n) + tone(100.0, n);

    //! [filter_data_usage]
    const MatrixXd filtered = filterData(data, lowPass); // zero-phase: the filter delay is removed
    //! [filter_data_usage]
    const double filterErr = (filtered.row(0) - tone(10.0, n)).segment(500, n - 1000).cwiseAbs().maxCoeff();
    ok &= expect(filtered.cols() == n && filterErr < 0.02,
                 std::format("filterData removes 100 Hz and keeps 10 Hz in phase (max error {:g})", filterErr));

    //! [filter_overlap_add_usage]
    FilterOverlapAdd overlapAdd; // keeps the filter tail between calls
    const int block = 1000;
    MatrixXd streamed(2, n);
    for (int start = 0; start < n; start += block) {
        streamed.middleCols(start, block) = overlapAdd.calculate(data.middleCols(start, block), lowPass);
    }
    //! [filter_overlap_add_usage]
    // Streaming output is delayed by half the filter length; compare the steady state with the causal tone
    const int delay = lowPass.filter_order() / 2;
    RowVectorXd delayedTone(n);
    for (int t = 0; t < n; ++t) {
        delayedTone(t) = std::sin(2.0 * kPi * 10.0 * (t - delay) / kSFreq);
    }
    const double streamErr = (streamed.row(0) - delayedTone).segment(1000, n - 1500).cwiseAbs().maxCoeff();
    ok &= expect(streamErr < 0.02, std::format("overlap-add over 4 blocks equals the delayed 10 Hz tone (max error {:g})", streamErr));

    //! [trigger_detection_usage]
    MatrixXd stim = MatrixXd::Zero(1, 3000); // a stim channel with code 5 at 0.5 s and code 3 at 2 s, 100 ms each
    stim.block(0, 500, 1, 100).setConstant(5.0);
    stim.block(0, 2000, 1, 100).setConstant(3.0);
    const std::vector<TriggerFlank> onsets = detect_trigger_flanks_max(stim, 0, 0, 0.5, false, 150);
    const std::vector<TriggerFlank> steps = detect_trigger_flanks_grad(stim, 0, 0, 0.5, FlankDirection::Falling, 150);
    //! [trigger_detection_usage]
    ok &= expect(onsets == std::vector<TriggerFlank>{{500, 5.0}, {2000, 3.0}} && steps == std::vector<TriggerFlank>{{600, 5.0}, {2100, 3.0}},
                 "trigger detection finds the two onsets and their ends");

    //! [filter_io_round_trip]
    QTemporaryDir dir;
    const std::filesystem::path path = dir.filePath("lp40.txt").toStdString();
    const bool written = FilterIO::write_filter(path, lowPass).has_value();
    const auto reloaded = FilterIO::read_filter(path); // the taps come back bit for bit
    //! [filter_io_round_trip]
    ok &= expect(written && reloaded && reloaded->coefficients() == lowPass.coefficients(),
                 std::format("FilterIO round trip keeps all {} taps exactly", lowPass.coefficients().size()));

    // Magnitude responses of the three designs from 0 to 150 Hz
    std::vector<double> freqs;
    std::vector<double> gainCosine;
    std::vector<double> gainTscheb;
    std::vector<double> gainPm;
    for (int step = 0; step <= 300; ++step) {
        const double f = 0.5 * step;
        freqs.push_back(f);
        gainCosine.push_back(gain(lowPass.coefficients(), f, kSFreq));
        gainTscheb.push_back(gain(lowPassPm.coefficients(), f, kSFreq));
        gainPm.push_back(gain(parksMcClellan.coefficients, f, kSFreq));
    }
    LinePlot designs;
    designs.setTitle("40 Hz low-pass designs");
    designs.setXLabel("Frequency [Hz]");
    designs.setYLabel("|H(f)|");
    designs.addSeries(freqs, gainCosine, "FilterKernel, cosine, 256 taps");
    designs.addSeries(freqs, gainTscheb, "FilterKernel, Tschebyscheff, 256 taps");
    designs.addSeries(freqs, gainPm, "ParksMcClellan, 65 taps");
    FigureExporter::save(designs, "frequency_response", "Magnitude response of the three 40 Hz low-pass designs: 10 Hz passes, 100 Hz is stopped.");

    // The first 200 ms of the 10 + 100 Hz input, filtered at once and streamed block by block
    std::vector<double> ms;
    std::vector<double> input;
    std::vector<double> once;
    std::vector<double> stream;
    for (int t = 0; t < 200; ++t) {
        ms.push_back(t);
        input.push_back(data(0, t + 1000));
        once.push_back(filtered(0, t + 1000));
        stream.push_back(streamed(0, t + 1000 + delay));
    }
    LinePlot traces;
    traces.setTitle("10 Hz + 100 Hz through the 40 Hz low-pass");
    traces.setXLabel("Time [ms]");
    traces.setYLabel("Amplitude");
    traces.addSeries(ms, input, "input", QColor(190, 190, 190));
    traces.addSeries(ms, once, "filterData (zero phase)");
    traces.addSeries(ms, stream, "FilterOverlapAdd, delay removed", QColor(), Qt::DashLine);
    FigureExporter::save(traces, "filtered_signal", "filterData and the block-wise FilterOverlapAdd both keep the 10 Hz tone and remove the 100 Hz tone.");

    std::println("{}", ok ? "All dsp filter checks passed." : "dsp filter checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
