//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Math library on inputs with known answers.
 *
 * Expected values come from NumPy (pseudo-inverse, condition number, FFT
 * frequencies), MNE-Python 1.11 (baseline rescaling) or closed forms (sphere,
 * minimum, clusters, warp of a pure translation). The example exits non-zero
 * if any value disagrees.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math/kmeans.h>
#include <math/linalg.h>
#include <math/numerics.h>
#include <math/simplex_algorithm.h>
#include <math/spectral.h>
#include <math/sphere.h>
#include <math/warp.h>

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
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================

bool expectValues(const MatrixXd& actual, std::initializer_list<double> expected, double tol, const QString& what)
{
    double err = 0.0;
    int k = 0;
    for (double value : expected) {
        err = std::max(err, std::fabs(actual.data()[k++] - value)); // column-major, as NumPy ravel(order="F")
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

    //! [linalg_usage]
    MatrixXd tall(3, 2);
    tall << 1, 2, 3, 4, 5, 6;
    const MatrixXd tallPinv = Linalg::pinv(tall); // Moore-Penrose, 2 x 3

    MatrixXd square(2, 2);
    square << 4, 1, 2, 3;
    VectorXd singular;
    const double condition = Linalg::getConditionNumber(square, singular); // s_max / s_min
    //! [linalg_usage]
    ok &= expectValues(tallPinv, {-1.3333333333333324, 1.0833333333333326, -0.3333333333333325, 0.33333333333333265, 0.6666666666666657, -0.41666666666666596}, 1e-12, "pinv = numpy.linalg.pinv");
    ok &= expect(std::fabs(condition - 2.6180339887498945) < 1e-12, QString("condition number %1 = numpy.linalg.cond").arg(condition));

    //! [numerics_rescale]
    MatrixXd data(2, 6);
    data << 1, 2, 3, 4, 6, 8, 2, 2, 2, 4, 4, 4;
    RowVectorXf times(6);
    times << -0.2f, -0.1f, 0.0f, 0.1f, 0.2f, 0.3f;
    const MatrixXd percent = Numerics::rescale(data, times, std::make_pair(-0.2f, 0.0f), std::string("percent")); // (x - mean) / mean over baseline
    //! [numerics_rescale]
    ok &= expectValues(percent, {-0.5, 0.0, 0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 2.0, 1.0, 3.0, 1.0}, 1e-12, "percent baseline = mne.baseline.rescale");
    ok &= expect(Numerics::gcd(84, 36) == 12 && Numerics::nchoose2(10) == 45, "gcd(84, 36) = 12, 10 choose 2 = 45");

    //! [spectral_psd]
    const int nSamples = 200;
    const double sFreq = 100.0;
    RowVectorXd signal(nSamples);
    for (int t = 0; t < nSamples; ++t) {
        signal(t) = 2.0 * std::sin(2.0 * kPi * 10.0 * t / sFreq); // 10 Hz, amplitude 2
    }
    const auto [tapers, weights] = Spectral::generateTapers(nSamples, std::string("hanning"));
    const MatrixXcd spectra = Spectral::computeTaperedSpectraRow(signal, tapers, nSamples);
    const RowVectorXd psd = Spectral::psdFromTaperedSpectra(spectra, weights, nSamples, sFreq);
    const VectorXd freqs = Spectral::calculateFFTFreqs(nSamples, sFreq);
    //! [spectral_psd]
    Index peak = 0;
    psd.maxCoeff(&peak);
    const double power = psd.sum() * (freqs(1) - freqs(0)); // integrated one-sided PSD = signal variance
    ok &= expect(freqs.size() == nSamples / 2 + 1 && std::fabs(freqs(peak) - 10.0) < 1e-12,
                 QString("PSD peaks at %1 Hz; %2 bins match numpy.fft.rfftfreq").arg(freqs(peak)).arg(freqs.size()));
    ok &= expect(std::fabs(power - 2.0) < 0.05, QString("Parseval: integrated PSD %1 = variance 2").arg(power));

    //! [sphere_fit]
    MatrixX3f points(6, 3); // six points on a sphere of radius 0.09 around (0, 0, 0.04)
    points << 0.09f, 0.0f, 0.04f, -0.09f, 0.0f, 0.04f, 0.0f, 0.09f, 0.04f,
        0.0f, -0.09f, 0.04f, 0.0f, 0.0f, 0.13f, 0.0f, 0.0f, -0.05f;
    Sphere sphere = Sphere::fit_sphere(points);
    //! [sphere_fit]
    ok &= expect((sphere.center() - Vector3f(0.0f, 0.0f, 0.04f)).norm() < 1e-5f && std::fabs(sphere.radius() - 0.09f) < 1e-5f,
                 QString("sphere centre (0, 0, 0.04), radius %1").arg(sphere.radius()));

    //! [simplex_minimize]
    // Minimise the Rosenbrock function from a 3-vertex simplex around (-1, 1); ftol is relative, so the
    // minimum value is offset to 1 (a minimum of exactly 0 never satisfies a relative tolerance)
    auto rosenbrock = [](const VectorXd& x) {
        return 1.0 + 100.0 * std::pow(x(1) - x(0) * x(0), 2) + std::pow(1.0 - x(0), 2);
    };
    MatrixXd simplex(3, 2);
    simplex << -1.0, 1.0, -0.9, 1.0, -1.0, 1.1;
    VectorXd values(3);
    for (int i = 0; i < 3; ++i) {
        values(i) = rosenbrock(simplex.row(i).transpose());
    }
    int evaluations = 0;
    const bool converged = SimplexAlgorithm::simplex_minimize<double>(simplex, values, 1e-12, 0.0, rosenbrock, 5000, evaluations);
    Index best = 0;
    values.minCoeff(&best); // vertices are not sorted on return
    //! [simplex_minimize]
    ok &= expect(converged && (simplex.row(best) - RowVector2d(1.0, 1.0)).norm() < 1e-3,
                 QString("Rosenbrock minimum (1, 1) after %1 evaluations").arg(evaluations));

    //! [kmeans_calculate]
    MatrixXd X(6, 2); // two well separated groups of three points
    X << 0.0, 0.0, 0.1, 0.0, 0.0, 0.1, 5.0, 5.0, 5.1, 5.0, 5.0, 5.1;
    KMeans kmeans(KMeansDistance::SquaredEuclidean, KMeansStart::Sample, 5); // best of 5 replicates
    VectorXi idx;
    MatrixXd centroids, distances;
    VectorXd sumD;
    kmeans.calculate(X, 2, idx, centroids, sumD, distances);
    //! [kmeans_calculate]
    const bool grouped = idx(0) == idx(1) && idx(1) == idx(2) && idx(3) == idx(4) && idx(4) == idx(5) && idx(0) != idx(3);
    const double lowCentroid = std::min(centroids(0, 0), centroids(1, 0));
    ok &= expect(grouped && std::fabs(lowCentroid - 0.1 / 3.0) < 1e-12, "KMeans finds both groups and their means");

    //! [warp_calculate]
    // Landmarks moved by a pure translation: the thin-plate spline must reproduce it exactly
    MatrixXf sourceLm(5, 3);
    sourceLm << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1;
    const MatrixXf destLm = sourceLm.rowwise() + RowVector3f(0.5f, -0.2f, 0.1f);
    MatrixXf vertices(2, 3);
    vertices << 0.3f, 0.4f, 0.5f, 0.9f, 0.1f, 0.2f;
    Warp warp;
    const MatrixXf warped = warp.calculate(sourceLm, destLm, vertices);
    //! [warp_calculate]
    const float warpErr = (warped - (vertices.rowwise() + RowVector3f(0.5f, -0.2f, 0.1f))).cwiseAbs().maxCoeff();
    ok &= expect(warpErr < 1e-4f, QString("TPS warp reproduces the translation (max error %1)").arg(warpErr));

    qInfo() << (ok ? "All math checks passed." : "Math checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
