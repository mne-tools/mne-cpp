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
 * (make_inverse_operator, apply_inverse, estimate_snr, make_inverse_resolution_matrix,
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
#include <inv/inv_vector_source_estimate.h>
#include <inv/inv_volume_source_estimate.h>
#include <inv/minimum_norm/inv_cmne.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/morph/source_morph.h>
#include <inv/sparse/inv_gamma_map.h>
#include <inv/sparse/inv_mxne.h>
#include <inv/sparse/inv_tf_mxne.h>
#include <mne/mne_cluster_info.h>
#include <mne/mne_cortical_map.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fs/fs_annotationset.h>
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
    const MNEForwardSolution fwd = MNEForwardSolution(fwdFile).pick_regions({v1}); // 70 sources x 3 orientations
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

    //! [mne_mne_data_usage]
    const MNEMneData perTime = dspm.mneData(evoked);       // lambda2 estimated per time point (MNE-C mne_analyze)
    const MNEMneData atSnr3 = dspm.mneData(evoked, 9.0);   // power SNR 9: lambda2 = mean(sing^2) / 9
    const VectorXd amplitudeSnr = perTime.SNR.cwiseSqrt(); // == mne.minimum_norm.estimate_snr's snr
    const MatrixXd residual = evoked.pick_channels(inverse.noise_cov->names).data - atSnr3.predicted;
    //! [mne_mne_data_usage]
    // SNR = sum(whitened^2) / rank, the lambda2 search = MNE-C noise_regularization (both in numpy);
    // predicted = data - apply_inverse(evoked, inv, mean(sing**2) / 9, "MNE", return_residual=True)[1]
    ok &= expect(near(perTime.SNR.sum(), 146652.3428555677, 1e-6) && near(amplitudeSnr(100), std::sqrt(354.67510619465145), 1e-6) && near(perTime.lambda2.sum(), 3.26430206181887e-11, 1e-6) && near(atSnr3.lambda2(0), 0.19047619047619055, 1e-9) && near(atSnr3.predicted.cwiseAbs().sum(), 0.047335931752795365, 1e-6) && residual.rows() == 364,
                 "MNEMneData: SNR, lambda2 and predicted data match mne.minimum_norm.estimate_snr / apply_inverse");

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

    //! [mne_cortical_map_usage]
    // The same resolution matrix from a prepared inverse; the forward is rotated to the inverse's surface basis
    MNEInverseOperator prepared = inverse.prepare_inverse_operator(evoked.nave, lambda2, false, false);
    MatrixXd surfaceKernel;
    SparseMatrix<double> noNormals;
    QList<VectorXi> kernelVertices;
    prepared.assemble_kernel(FsLabel(), "MNE", false, surfaceKernel, noNormals, kernelVertices);
    prepared.getKernel() = surfaceKernel;
    const MatrixXd corticalMap = MNECorticalMap::makeCorticalMap(fwd, prepared, evoked.info);
    //! [mne_cortical_map_usage]
    // mne K (Cartesian) times the Cartesian gain, the same product in one basis: trace 26.1330, Frobenius norm 6.13353
    ok &= expect(corticalMap.rows() == 210 && corticalMap.cols() == 210 && near(corticalMap.trace(), 26.133038866971994, 1e-4) && near(corticalMap.norm(), 6.13352636100184, 1e-4),
                 "MNECorticalMap: K G in the inverse's surface basis matches mne");

    //! [mne_cluster_info_usage]
    // Cluster the V1 sources within their aparc regions into groups of about 20 (k-means on the gain columns;
    // squared Euclidean distance makes each centroid the mean of its sources)
    const FsAnnotationSet aparc(data + "/subjects/sample/label/lh.aparc.annot", data + "/subjects/sample/label/rh.aparc.annot");
    MatrixXd clusterMean; // 3 x 70 sources -> 3 x nClusters, averaging per orientation
    const MNEForwardSolution clustered = fwd.cluster_forward_solution(aparc, 20, clusterMean, FiffCov(), FiffInfo(), "sqeuclidean");
    const MNEClusterInfo& clusters = clustered.src.hemisphereAt(0)->cluster_info;
    int assigned = 0;
    for (const VectorXi& members : clusters.clusterVertnos) {
        assigned += static_cast<int>(members.size());
    }
    clusters.write(tmp.filePath("v1-clusters.txt")); // plus centroids_v1-clusters.txt
    //! [mne_cluster_info_usage]
    ok &= expect(clustered.isClustered() && clusters.numClust() == clustered.nsource && assigned == 70 && clusters.numClust() >= 4 && (clustered.sol->data - fwd.sol->data * clusterMean).norm() <= 1e-9 * fwd.sol->data.norm() && QFile::exists(tmp.filePath("centroids_v1-clusters.txt")),
                 "MNEClusterInfo: every V1 source in one cluster, clustered gain is the cluster mean");

    //! [inv_vector_source_estimate_usage]
    // The MNE kernel keeps x, y and z per source: an apply_inverse(..., pick_ori="vector") estimate.
    const MatrixXd goodData = evoked.pick_channels(inverse.noise_cov->names).data;
    const InvVectorSourceEstimate vectorStc(kernel * goodData, stc.vertices, stc.tmin, stc.tstep);
    const InvSourceEstimate magnitude = vectorStc.magnitude(); // 70 x 421 vector lengths
    //! [inv_vector_source_estimate_usage]
    // mne.minimum_norm.apply_inverse(evoked, inv, 1/9, "MNE", pick_ori="vector"): sum of lengths 4.31370e-4
    ok &= expect(vectorStc.nVertices() == 70 && near(magnitude.data.sum(), 0.0004313698431300483, 1e-4) && near(magnitude.data(0, 200), 3.865084798591962e-09, 1e-4),
                 "InvVectorSourceEstimate: vector MNE lengths match apply_inverse(pick_ori=\"vector\")");

    //! [inv_volume_source_estimate_usage]
    // A volume estimate keeps the grid shape, so a time slice can be written back into the full grid.
    InvVolumeSourceEstimate volumeStc(stc.data.topRows(4), VectorXi::LinSpaced(4, 0, 6), stc.tmin, stc.tstep);
    volumeStc.setShape({2, 2, 2}); // 8 voxels, 4 of them sources
    const VectorXd grid = volumeStc.toVolume(200);
    //! [inv_volume_source_estimate_usage]
    ok &= expect(grid.size() == 8 && grid(0) == stc.data(0, 200) && grid(6) == stc.data(3, 200) && grid(1) == 0.0,
                 "InvVolumeSourceEstimate: source values land on their voxels, the rest of the grid is zero");

    //! [source_morph_usage]
    // Morph the 70 V1 sources onto 35 targets by averaging neighbouring pairs (a sphere-based morph map in practice).
    SparseMatrix<double> pairAverage(35, 70);
    for (int i = 0; i < 35; ++i) {
        pairAverage.insert(i, 2 * i) = 0.5;
        pairAverage.insert(i, 2 * i + 1) = 0.5;
    }
    SourceMorph morph;
    morph.compute(stc.vertices, VectorXi::LinSpaced(35, 0, 34), pairAverage);
    const InvSourceEstimate morphed = morph.apply(stc);
    //! [source_morph_usage]
    ok &= expect(morph.isComputed() && morphed.data.rows() == 35 && near(morphed.data(3, 200), 0.5 * (stc.data(6, 200) + stc.data(7, 200)), 1e-12),
                 "SourceMorph: the morph matrix maps every time sample");

    //! [inv_label_time_course_usage]
    const MatrixXd labelMean = InvLabelTimeCourse::extract(stc, {v1}, "mean");      // 1 x 421
    const MatrixXd labelMax = InvLabelTimeCourse::extract(stc, {v1}, "max");        // the largest |value| per sample
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

    //! [inv_gamma_map_usage]
    // Gamma-MAP: sparse Bayesian learning with unit (whitened) noise, MacKay updates as in mne.inverse_sparse.gamma_map.
    const InvGammaMapResult gammaMap = InvGammaMap::compute(gain, measured, MatrixXd::Identity(gain.rows(), gain.rows()), 1000, 1e-8);
    //! [inv_gamma_map_usage]
    ok &= expect(!gammaMap.activeVertices.isEmpty() && gammaMap.activeVertices.size() < 70 && gammaMap.vecGamma.size() == 70,
                 QString("InvGammaMap: %1 of 70 V1 sources keep a nonzero variance").arg(gammaMap.activeVertices.size()));

    //! [inv_tf_mxne_usage]
    // TF-MxNE: sparse in space and in a Gabor time-frequency dictionary; 40 samples at 150 Hz.
    InvTfMxneParams tfParams;
    tfParams.dAlphaSpace = 0.5 * alphaMax;
    tfParams.dAlphaTime = 0.05 * alphaMax;
    tfParams.dSFreq = evoked.info.sfreq;
    tfParams.iNFreqs = 4;
    tfParams.dFMin = 5.0;
    tfParams.dFMax = 40.0;
    const InvTfMxneResult tfMxne = InvTfMxne::compute(gain, measured, tfParams);
    //! [inv_tf_mxne_usage]
    ok &= expect(!tfMxne.activeVertices.isEmpty() && tfMxne.activeVertices.size() <= 5 && tfMxne.activeVertices.contains(8),
                 QString("InvTfMxne: %1 V1 sources, including the strongest MxNE source 8").arg(tfMxne.activeVertices.size()));

    //! [inv_cmne_usage]
    // Contextual MNE: dSPM with a fixed-orientation kernel, then rectified and z-scored per source. Without an
    // ONNX LSTM model the result is the paper's control estimate.
    InvCMNESettings cmneSettings;
    cmneSettings.lambda2 = lambda2;
    cmneSettings.lookBack = 20;
    const MatrixXd fixedGain = fixedFwd.sol->data;
    const MatrixXd sourceCov = MatrixXd::Identity(fixedGain.cols(), fixedGain.cols());
    const InvCMNEResult cmne = InvCMNE::compute(goodData, fixedGain, inverse.noise_cov->data, sourceCov, cmneSettings);
    //! [inv_cmne_usage]
    // Every row of the dSPM kernel maps sensor noise with the noise covariance to unit variance.
    const MatrixXd& kDspm = cmne.matKernelDspm;
    const VectorXd unitNoise = (kDspm * inverse.noise_cov->data * kDspm.transpose()).diagonal();
    ok &= expect(kDspm.rows() == 70 && (unitNoise.array() - 1.0).abs().maxCoeff() < 1e-6 && cmne.stcCmne.data.rows() == 70 && cmne.stcCmne.data.cols() == 421,
                 "InvCMNE: dSPM kernel has unit noise variance per source, CMNE estimate for all 421 samples");

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
