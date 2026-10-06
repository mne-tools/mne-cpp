//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Inverse library: minimum-norm estimates, resolution matrices, label time courses, sparse solvers
 *           and source-estimate I/O.
 *
 * Builds a loose-orientation inverse operator for the 70 lh.V1 sources of the
 * sample forward solution and compares every result with MNE-Python 1.11
 * (make_inverse_operator, apply_inverse, make_inverse_resolution_matrix,
 * extract_label_time_course, mixed_norm_solver). Exits non-zero on
 * any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_convenience.h>
#include <inv/inv_label_time_course.h>
#include <inv/inv_resolution_matrix.h>
#include <inv/inv_source_estimate.h>
#include <inv/inv_source_estimate_io.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/sparse/inv_mxne.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fs/fs_label.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

bool near(double value, double expected, double relTol)
{
    return std::fabs(value - expected) <= relTol * std::fabs(expected);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    parser.addOption(dataOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

    //! [inv_minimum_norm_usage]
    FsLabel v1;
    FsLabel::read(data + "/subjects/sample/label/lh.V1.label", v1);
    QFile fwdFile(data + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    // surf_ori = true: loose orientation constraints are relative to the cortical normals
    const MNEForwardSolution fwd = MNEForwardSolution(fwdFile, false, true).pick_regions({v1}); // 70 sources x 3
    QFile covFile(data + "/MEG/sample/sample_audvis-cov.fif");
    const FiffCov noiseCov(covFile);
    QFile aveFile(data + "/MEG/sample/sample_audvis-ave.fif");
    const FiffEvoked evoked(aveFile, 0); // "Left Auditory"

    const MNEInverseOperator inverse = MNEInverseOperator::make_inverse_operator(evoked.info, fwd, noiseCov, 0.2f, 0.8f);
    const float lambda2 = 1.0f / 9.0f; // SNR 3
    InvMinimumNorm dspm(inverse, lambda2, "dSPM");
    const InvSourceEstimate stc = dspm.calculateInverse(evoked); // one row per source, combined over orientations
    //! [inv_minimum_norm_usage]
    // mne.minimum_norm.apply_inverse(evoked, inv, 1/9, "dSPM"): 70 x 421, sum |stc| = 372483.6, stc[0, 0] = 5.00927
    ok &= expect(stc.data.rows() == 70 && stc.data.cols() == 421 && near(stc.data.cwiseAbs().sum(), 372483.6177246599, 1e-4) && near(stc.data(0, 0), 5.0092691144321755, 1e-4) && near(stc.tmin, -0.19979521, 1e-6),
                 "InvMinimumNorm/InvSourceEstimate: dSPM on 70 V1 sources matches mne.minimum_norm.apply_inverse");

    //! [inv_resolution_matrix_usage]
    InvMinimumNorm mne(inverse, lambda2, "MNE");
    mne.doInverseSetup(evoked.nave);
    const MatrixXd& kernel = mne.getKernel(); // 210 source components x 364 good channels
    const MNEForwardSolution goodFwd = fwd.pick_channels(inverse.noise_cov->names);
    const MatrixXd resolution = InvResolutionMatrix::compute(kernel, goodFwd.sol->data);
    //! [inv_resolution_matrix_usage]
    // mne.minimum_norm.make_inverse_resolution_matrix(fwd, inv, "MNE", 1/9): 210 x 210, Frobenius norm 6.13353
    ok &= expect(resolution.rows() == 210 && resolution.cols() == 210 && near(resolution.norm(), 6.13352636100184, 1e-4),
                 "InvResolutionMatrix: MNE resolution matrix matches mne.minimum_norm.make_inverse_resolution_matrix");

    //! [inv_label_time_course_usage]
    const MatrixXd labelMean = InvLabelTimeCourse::extract(stc, {v1}, "mean");     // 1 x 421
    const MatrixXd labelMax = InvLabelTimeCourse::extract(stc, {v1}, "max");       // the largest |value| per sample
    const MatrixXd labelFlip = InvLabelTimeCourse::extract(stc, {v1}, "mean_flip"); // signs flipped by the patch normals
    //! [inv_label_time_course_usage]
    // mne.extract_label_time_course(stc, v1, src, mode=...): sum |tc| for "mean" 5321.19 and "max" 18182.6
    ok &= expect(labelMean.rows() == 1 && near(labelMean.cwiseAbs().sum(), 5321.1945389237135, 1e-4) && near(labelMax.cwiseAbs().sum(), 18182.625800623755, 1e-4) && labelFlip.cols() == 421,
                 "InvLabelTimeCourse: V1 mean and max time courses match mne.extract_label_time_course");

    //! [inv_sparse_usage]
    // Whitened data and a gain per nAm make the noise level 1, as mne.inverse_sparse.mixed_norm does.
    const auto [whitener, rank] = computeWhitener(*inverse.noise_cov); // rank 360: 364 channels minus 4 projectors
    QFile fixedFile(fwdFile.fileName());
    const MNEForwardSolution fixedFwd = MNEForwardSolution(fixedFile, true, true).pick_regions({v1}).pick_channels(inverse.noise_cov->names);
    const MatrixXd gain = whitener * fixedFwd.sol->data * 1e-9;
    const MatrixXd measured = whitener * evoked.pick_channels(inverse.noise_cov->names).data.middleCols(170, 40); // 83-148 ms
    const double alphaMax = (gain.transpose() * measured).rowwise().norm().maxCoeff();
    const InvMxneResult mxne = InvMxne::compute(gain, measured, 0.5 * alphaMax, 1000, 1e-10);
    //! [inv_sparse_usage]
    // mne mixed_norm_solver(M, G, 0.5 * alpha_max, n_orient=1, debias=False): sources 8, 30, 37 with 21.37, 1.019, 4.455 nAm
    ok &= expect(rank == 360 && near(alphaMax, 14.508623946749108, 1e-4) && mxne.activeVertices == QVector<int>({8, 30, 37}) && near(mxne.stc.data.row(0).norm(), 21.373078745157443, 1e-3) && near(mxne.stc.data.row(2).norm(), 4.455019654645694, 1e-3),
                 "computeWhitener/InvMxne: whitened rank 360, MxNE picks the 3 V1 sources mne.inverse_sparse finds");

    //! [inv_source_estimate_io_usage]
    InvSourceEstimateIO::writeCsv(stc, tmp.filePath("v1-dspm.csv"));
    const InvSourceEstimate fromCsv = InvSourceEstimateIO::readCsv(tmp.filePath("v1-dspm.csv"));
    QFile stcFile(tmp.filePath("v1-dspm-lh.stc"));
    InvSourceEstimate(stc).write(stcFile); // MNE .stc format, readable by mne.read_source_estimate
    QFile stcBack(stcFile.fileName());
    const InvSourceEstimate fromStc(stcBack);
    //! [inv_source_estimate_io_usage]
    ok &= expect(fromCsv.data.rows() == 70 && (fromCsv.data - stc.data).norm() <= 1e-6 * stc.data.norm() && fromStc.vertices == stc.vertices && (fromStc.data - stc.data).norm() <= 1e-6 * stc.data.norm(),
                 "InvSourceEstimateIO/InvSourceEstimate: CSV and .stc round trips");

    qInfo().noquote() << (ok ? "All inv checks passed." : "inv checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
