//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Statistics library on small literal data sets with known answers.
 *
 * The expected numbers come from scipy 1.16 (t- and F-tests), scikit-learn
 * (Ledoit-Wolf, OAS) and MNE-Python 1.11 (FDR, Bonferroni) on the same arrays,
 * or from closed forms. The example exits non-zero if any value disagrees.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <sts/sts_adjacency.h>
#include <sts/sts_cluster.h>
#include <sts/sts_correction.h>
#include <sts/sts_cov_estimators.h>
#include <sts/sts_ftest.h>
#include <sts/sts_source_metrics.h>
#include <sts/sts_ttest.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <initializer_list>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
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

//=============================================================================================================

bool expectRow(const MatrixXd& actual, std::initializer_list<double> expected, double tol, const QString& what)
{
    double err = 0.0;
    int k = 0;
    for (double value : expected) {
        err = std::max(err, std::fabs(actual.data()[k++] - value));
    }
    return expect(k == actual.size() && err < tol, QString("%1 (max error %2)").arg(what).arg(err));
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    // 6 subjects (rows) x 3 variables (columns) per condition
    MatrixXd a(6, 3), b(6, 3), c(6, 3);
    a << 1.2, 0.1, -0.3, 0.9, -0.4, 0.2, 1.5, 0.3, 0.1, 1.1, -0.2, -0.5, 0.8, 0.5, 0.4, 1.4, -0.1, 0.0;
    b << 0.2, 0.0, -0.1, 0.1, -0.2, 0.3, 0.6, 0.4, 0.0, 0.3, -0.3, -0.2, 0.0, 0.2, 0.6, 0.5, 0.1, -0.3;
    c << 0.1, 0.3, 0.2, -0.2, 0.1, 0.5, 0.4, -0.1, 0.1, 0.0, 0.2, -0.3, -0.1, 0.6, 0.2, 0.3, 0.0, 0.4;

    //! [stats_ttest_usage]
    const StatsTtestResult oneSample = StatsTtest::oneSample(a);        // H0: column mean = 0
    const StatsTtestResult paired = StatsTtest::paired(a, b);           // H0: mean(a - b) = 0
    const StatsTtestResult independent = StatsTtest::independent(a, b); // pooled variance, df = 10
    const StatsTtestResult rightTail = StatsTtest::oneSample(a, 0.0, StatsTailType::Right);
    //! [stats_ttest_usage]
    // scipy.stats.ttest_1samp / ttest_rel / ttest_ind
    ok &= expectRow(oneSample.matTstat, {10.285912696499036, 0.24544034683690788, -0.12327841818038444}, 1e-10, "one-sample t = scipy");
    ok &= expectRow(oneSample.matPval, {0.00014930867649425364, 0.8158714843641037, 0.9066888237538449}, 1e-10, "one-sample p = scipy");
    ok &= expectRow(paired.matTstat, {26.0, 0.0, -0.7254762501100115}, 1e-10, "paired t = scipy");
    ok &= expectRow(paired.matPval, {1.5724552146685355e-06, 1.0, 0.5006803901900951}, 1e-10, "paired p = scipy");
    ok &= expectRow(independent.matTstat, {5.918268896877812, 0.0, -0.34452048127477725}, 1e-10, "independent t = scipy");
    ok &= expectRow(independent.matPval, {0.0001473880425533016, 1.0, 0.7375904333691938}, 1e-10, "independent p = scipy");
    ok &= expect(oneSample.degreesOfFreedom == 5 && independent.degreesOfFreedom == 10, "df 5 and 10");
    ok &= expect(std::fabs(rightTail.matPval(0, 0) - oneSample.matPval(0, 0) / 2.0) < 1e-12, "right-tailed p is half the two-tailed p");

    //! [stats_ftest_usage]
    const StatsFtestResult anova = StatsFtest::oneWay({a, b, c}); // one-way ANOVA per column
    //! [stats_ftest_usage]
    // scipy.stats.f_oneway
    ok &= expectRow(anova.matFstat, {31.736745886654532, 0.5648535564853556, 0.6174200661521498}, 1e-10, "F = scipy");
    ok &= expectRow(anova.matPval, {4.076262200363868e-06, 0.5800772485078302, 0.5524898704594713}, 1e-10, "ANOVA p = scipy");
    ok &= expect(anova.dfBetween == 2 && anova.dfWithin == 15, "df 2 and 15");

    //! [stats_mc_correction_usage]
    MatrixXd pValues(1, 8);
    pValues << 0.001, 0.008, 0.039, 0.041, 0.042, 0.06, 0.074, 0.205;
    const MatrixXd pBonferroni = StatsMcCorrection::bonferroni(pValues);
    const MatrixXd pHolm = StatsMcCorrection::holmBonferroni(pValues);
    const MatrixXd pFdr = StatsMcCorrection::fdr(pValues); // Benjamini-Hochberg adjusted p-values
    //! [stats_mc_correction_usage]
    // mne.stats.bonferroni_correction / fdr_correction; Holm step-down closed form
    ok &= expectRow(pBonferroni, {0.008, 0.064, 0.312, 0.328, 0.336, 0.48, 0.592, 1.0}, 1e-12, "Bonferroni = mne");
    ok &= expectRow(pHolm, {0.008, 0.056, 0.234, 0.234, 0.234, 0.234, 0.234, 0.234}, 1e-12, "Holm = closed form");
    ok &= expectRow(pFdr, {0.008, 0.032, 0.0672, 0.0672, 0.0672, 0.08, 0.08457142857142856, 0.205}, 1e-12, "FDR = mne");

    //! [sts_cov_estimators_usage]
    MatrixXd data(3, 8); // channels x samples
    data << 0.5, -1.2, 0.3, 0.8, -0.4, 1.1, -0.9, 0.2,
        0.4, -1.0, 0.1, 0.9, -0.2, 0.8, -1.1, 0.3,
        -0.3, 0.2, 0.7, -0.5, 0.4, -0.1, 0.1, 0.6;
    data = data.colwise() - data.rowwise().mean(); // the estimators expect zero-mean data
    const auto [covLw, shrinkLw] = StsCovEstimators::ledoitWolf(data);
    const auto [covOas, shrinkOas] = StsCovEstimators::oas(data);
    const auto [covBest, method] = StsCovEstimators::autoSelect(data); // cross-validated choice
    //! [sts_cov_estimators_usage]
    // sklearn.covariance.ledoit_wolf / oas with assume_centered=True
    ok &= expect(std::fabs(shrinkLw - 0.201822477072047) < 1e-12 && std::fabs(shrinkOas - 0.4393235385375307) < 1e-12,
                 QString("shrinkage LW %1, OAS %2 = sklearn").arg(shrinkLw).arg(shrinkOas));
    ok &= expectRow(covLw.row(0), {0.5436421771536946, 0.4140545900188756, -0.08730066657024485}, 1e-12, "LW covariance row 0 = sklearn");
    ok &= expectRow(covOas.row(0), {0.5037989001234694, 0.29085091438365596, -0.06132398797245758}, 1e-12, "OAS covariance row 0 = sklearn");
    ok &= expect(method >= 0 && method <= 5 && covBest.rows() == 3, "autoSelect returns a 3 x 3 covariance and a method index 0-5");

    //! [stats_adjacency_usage]
    // Two triangles sharing the edge 1-2 form a 4-vertex mesh
    MatrixX3i tris(2, 3);
    tris << 0, 1, 2, 1, 2, 3;
    const SparseMatrix<int> spatial = StatsAdjacency::fromSourceSpace(tris, 4);
    const SparseMatrix<int> spatioTemporal = StatsAdjacency::fromSourceSpaceTemporal(tris, 4, 5); // index = vertex * 5 + time
    //! [stats_adjacency_usage]
    const MatrixXi dense = MatrixXi(spatial);
    ok &= expect(dense(0, 1) == 1 && dense(1, 3) == 1 && dense(0, 3) == 0 && dense == dense.transpose(),
                 "mesh adjacency: 0-1 and 1-3 connected, 0-3 not, symmetric");
    ok &= expect(spatioTemporal.rows() == 20 && spatioTemporal.coeff(0 * 5 + 2, 0 * 5 + 3) == 1 && spatioTemporal.coeff(0 * 5 + 2, 1 * 5 + 2) == 1,
                 "spatio-temporal adjacency links (v0,t2) to (v0,t3) and (v1,t2)");

    //! [stats_cluster_usage]
    // 12 subjects, 8 channels on a line, 1 time point; channels 2-4 carry an effect in condition B
    SparseMatrix<int> chain(8, 8);
    for (int i = 0; i + 1 < 8; ++i) {
        chain.insert(i, i + 1) = 1;
        chain.insert(i + 1, i) = 1;
    }
    QVector<MatrixXd> condA, condB;
    for (int s = 0; s < 12; ++s) {
        MatrixXd noiseA(8, 1), noiseB(8, 1);
        for (int ch = 0; ch < 8; ++ch) {
            noiseA(ch, 0) = std::sin(1.3 * s + 0.7 * ch);
            noiseB(ch, 0) = std::cos(0.9 * s + 1.1 * ch) + ((ch >= 2 && ch <= 4) ? 3.0 : 0.0);
        }
        condA.append(noiseA);
        condB.append(noiseB);
    }
    const StatsClusterResult clusters = StatsCluster::permutationTest(condA, condB, chain, 500);
    // Cluster ids are negative where A < B; |id| - 1 indexes vecClusterStats and vecClusterPvals
    const int effectCluster = clusters.matClusterIds(3, 0);
    //! [stats_cluster_usage]
    const bool clusterOk = effectCluster < 0 && clusters.matClusterIds(2, 0) == effectCluster && clusters.matClusterIds(4, 0) == effectCluster && clusters.matClusterIds(7, 0) != effectCluster;
    const double clusterP = clusterOk ? clusters.vecClusterPvals[-effectCluster - 1] : -1.0;
    // The observed statistic counts as one permutation, so p >= 1 / (500 + 1) as in MNE-Python
    ok &= expect(clusterOk && clusterP >= 1.0 / 501.0 && clusterP < 0.05,
                 QString("channels 2-4 form one negative cluster, 1/501 <= p %1 < 0.05").arg(clusterP));

    //! [stats_source_metrics_usage]
    VectorXd amplitudes(4);
    amplitudes << 0.1, 2.0, 0.5, -0.4;
    MatrixXd positions(4, 3); // metres
    positions << 0.00, 0.00, 0.00, 0.01, 0.00, 0.00, 0.02, 0.00, 0.00, 0.01, 0.01, 0.00;
    const int peak = StatsSourceMetrics::findPeakIndex(amplitudes);
    const double localizationError = StatsSourceMetrics::peakLocalizationError(Vector3d(0.01, 0.0, 0.003),
                                                                               positions.row(peak).transpose());
    const double dispersion = StatsSourceMetrics::spatialDispersion(amplitudes, positions, peak);
    //! [stats_source_metrics_usage]
    // SD = (0.1 * 0.01 + 0.5 * 0.01 + 0.4 * 0.01) / (0.1 + 2.0 + 0.5 + 0.4)
    ok &= expect(peak == 1 && std::fabs(localizationError - 0.003) < 1e-15 && std::fabs(dispersion - 0.01 / 3.0) < 1e-15,
                 QString("peak 1, PLE 3 mm, SD %1 m = 1/300").arg(dispersion));

    qInfo() << (ok ? "All statistics checks passed." : "Statistics checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
