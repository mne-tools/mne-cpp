//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Contextual minimum-norm estimate (CMNE) of the auditory evoked response in both auditory cortices.
 *
 * Computes the dSPM estimate of the "Left Auditory" response on the superior and transverse temporal sources
 * of the sample subject, rectifies and z-scores it, and weights every sample by its temporal context
 * (Dinh et al., 2021). With --onnx an LSTM exported by cmne.export_onnx (or trained with mne_compute_cmne
 * --mode train) predicts the context; without it the paper's control estimate is used. The peaks of dSPM
 * and CMNE are compared, and --stcOut writes both estimates as .stc files. Exits non-zero when a check fails.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/inv/inv_source_estimate.h>
#include <mne/inv/minimum_norm/inv_cmne.h>
#include <mne/inv/minimum_norm/inv_cmne_settings.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fs/fs_label.h>
#include <mne/fs/fs_surfaceset.h>

#include <mne/disp_widgets/plots/helpers/figureexporter.h>
#include <mne/disp_widgets/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <format>
#include <print>
#include <string_view>
#include <tuple>

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

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

//=============================================================================================================

std::string describePeak(const InvSourceEstimate& stc, int from, int to)
{
    Index source = 0, sample = 0;
    stc.data.middleCols(from, to - from).cwiseAbs().maxCoeff(&source, &sample);
    return std::format("{} vertex {} at {} ms", source < stc.nVerticesLh ? "lh" : "rh", stc.vertices(source), std::lround(1000.0 * (stc.tmin + (from + sample) * stc.tstep)));
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Contextual minimum-norm estimate (CMNE) example");
    parser.addHelpOption();
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    QCommandLineOption onnxOption("onnx", "CMNE LSTM <model> exported by cmne.export_onnx for these sources.", "model");
    QCommandLineOption lookBackOption("lookBack", "Number of past <samples> forming the context.", "samples", "20");
    QCommandLineOption stcOption("stcOut", "Write <prefix>-dspm-{lh,rh}.stc and <prefix>-cmne-{lh,rh}.stc.", "prefix");
    parser.addOption(dataOption);
    parser.addOption(onnxOption);
    parser.addOption(lookBackOption);
    parser.addOption(stcOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    bool ok = true;

    // Fixed-orientation gain of the superior and transverse temporal sources of both hemispheres
    const QString subjects = data + "/subjects";
    const FsAnnotationSet aparc("sample", 2, "aparc", subjects.toStdU16String());
    std::vector<FsLabel> auditory;
    std::vector<RowVector4i> colors;
    aparc.toLabels(FsSurfaceSet("sample", 2, "white", subjects.toStdU16String()), auditory, colors,
                   {"superiortemporal-lh", "transversetemporal-lh", "superiortemporal-rh", "transversetemporal-rh"});
    const MNEForwardSolution fwd = MNEForwardSolution((data + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif").toStdU16String(), {.force_fixed = true, .surf_ori = true}).pick_regions({auditory.begin(), auditory.end()});
    const FiffCov noiseCov = FiffCov((data + "/MEG/sample/sample_audvis-cov.fif").toStdString()).pick_channels(fwd.sol.row_names);
    const FiffEvoked evoked = FiffEvoked((data + "/MEG/sample/sample_audvis-ave.fif").toStdU16String(), 0).pick_channels(fwd.sol.row_names); // "Left Auditory"
    const std::vector<VectorXi> vertno = fwd.src.get_vertno();
    const int nLh = static_cast<int>(vertno[0].size());
    VectorXi vertices(nLh + vertno[1].size());
    vertices << vertno[0], vertno[1];

    //! [inv_cmne_settings_usage]
    InvCMNESettings settings;
    settings.lambda2 = 1.0 / 9.0;                             // SNR 3
    settings.lookBack = parser.value(lookBackOption).toInt(); // k = 20 samples = 133 ms at 150 Hz
    settings.numSources = fwd.nsource;
    settings.onnxModelPath = parser.value(onnxOption).toStdString(); // empty: the paper's control estimate
    //! [inv_cmne_settings_usage]

    //! [inv_cmne_usage]
    const MatrixXd sourceCov = MatrixXd::Identity(fwd.nsource, fwd.nsource);
    InvCMNEResult cmne = InvCMNE::compute(evoked.data, fwd.sol.data, noiseCov.data, sourceCov, settings);
    if (cmne.stcCmne.empty()) {
        std::println(stderr, "The CMNE model does not fit {} sources.", fwd.nsource);
        return EXIT_FAILURE;
    }
    for (InvSourceEstimate* stc : {&cmne.stcDspm, &cmne.stcSensing, &cmne.stcCmne}) {
        stc->vertices = vertices; // compute() knows neither the source space nor the sampling
        stc->nVerticesLh = nLh;
        stc->tmin = evoked.times(0);
        stc->tstep = 1.0f / evoked.info.sfreq;
    }
    //! [inv_cmne_usage]

    const int n = static_cast<int>(evoked.data.cols());
    const int k = settings.lookBack;
    const MatrixXd& kernel = cmne.matKernelDspm;
    const VectorXd unitNoise = (kernel * noiseCov.data * kernel.transpose()).diagonal();
    ok &= expect(cmne.stcCmne.data.rows() == fwd.nsource && cmne.stcCmne.data.cols() == n && (unitNoise.array() - 1.0).abs().maxCoeff() < 1e-6,
                 std::format("{} auditory sources x {} samples; the dSPM kernel maps sensor noise to unit variance", fwd.nsource, n));
    // The LSTM runs in float32, so compare to single precision.
    ok &= expect((cmne.stcCmne.data.leftCols(k) - cmne.stcSensing.data.leftCols(k)).cwiseAbs().maxCoeff() < 1e-5 && (cmne.stcSensing.data - InvCMNE::zScoreRectify(cmne.stcDspm.data)).norm() == 0.0,
                 std::format("the first k = {} samples have no context: CMNE equals the rectified, z-scored dSPM", k));
    if (settings.onnxModelPath.empty()) {
        ok &= expect(cmne.stcCmne.data == InvCMNE::controlEstimate(cmne.stcDspm.data, k),
                     "without a model, CMNE is the control estimate");
    }

    // Peaks of the N100m window, 50-150 ms after the tone
    const int from = static_cast<int>(std::lround((0.05 - evoked.times(0)) * evoked.info.sfreq));
    const int to = static_cast<int>(std::lround((0.15 - evoked.times(0)) * evoked.info.sfreq));
    std::println("  dSPM peak: {}", describePeak(cmne.stcDspm, from, to));
    std::println("  CMNE peak: {}", describePeak(cmne.stcCmne, from, to));

    // Mean over the auditory sources of each hemisphere
    std::vector<double> ms(n);
    for (int s = 0; s < n; ++s) {
        ms[s] = 1000.0 * (evoked.times(0) + s / evoked.info.sfreq);
    }
    const auto meanOver = [](const MatrixXd& m, int first, int count) {
        const VectorXd mean = m.middleRows(first, count).colwise().mean().transpose();
        return std::vector<double>(mean.data(), mean.data() + mean.size());
    };
    const QList<std::tuple<QString, int, int>> hemispheres = {{"left", 0, nLh}, {"right", nLh, fwd.nsource - nLh}};
    for (const auto& [hemi, first, count] : hemispheres) {
        LinePlot courses;
        courses.setTitle(QString("Auditory cortex, %1 hemisphere: mean of %2 sources").arg(hemi).arg(count));
        courses.setXLabel("Time [ms]");
        courses.setYLabel("z-score");
        courses.addSeries(ms, meanOver(cmne.stcSensing.data, first, count), "dSPM, rectified and z-scored", QColor(150, 150, 150));
        courses.addSeries(ms, meanOver(cmne.stcCmne.data, first, count), settings.onnxModelPath.empty() ? "CMNE, control estimate" : "CMNE, LSTM context");
        FigureExporter::save(courses, "cmne_" + hemi.toStdString(), QString("'Left Auditory' response in the %1 superior and transverse temporal sources: CMNE weights each sample by its %2-sample context.").arg(hemi).arg(k).toStdString());
    }

    if (parser.isSet(stcOption)) {
        const QString prefix = parser.value(stcOption);
        ok &= expect(cmne.stcDspm.write_hemisphere_pair((prefix + "-dspm").toStdU16String()) && cmne.stcCmne.write_hemisphere_pair((prefix + "-cmne").toStdU16String()),
                     std::format("wrote {}-{{dspm,cmne}}-{{lh,rh}}.stc", prefix.toStdString()));
    }

    std::println("{}", ok ? "All CMNE checks passed." : "CMNE checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
