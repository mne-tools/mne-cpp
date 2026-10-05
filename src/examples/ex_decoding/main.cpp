//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Spatial decoders and ICA labelling on synthetic mixtures with a known ground truth.
 *
 * Four sources are mixed into four channels by a fixed matrix. Each decoder must
 * recover the source it was built for (CSP: a class-dependent variance, SPoC: a
 * variance that tracks a continuous target, SSD: a 10 Hz rhythm), and the ICA
 * labeller must tell ocular, cardiac, muscle and brain components apart. The
 * example exits non-zero otherwise.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <decoding/decoding_csp.h>
#include <decoding/decoding_ica_label.h>
#include <decoding/decoding_spoc.h>
#include <decoding/decoding_ssd.h>

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
#include <random>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 200.0;

//=============================================================================================================
/**
 * Standard normal deviate from the raw mt19937 stream via Box-Muller, identical on every standard library.
 */
double gaussian(std::mt19937& gen)
{
    const double u1 = (static_cast<double>(gen()) + 0.5) / 4294967296.0;
    const double u2 = (static_cast<double>(gen()) + 0.5) / 4294967296.0;
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
}

//=============================================================================================================

MatrixXd noise(std::mt19937& gen, int rows, int cols)
{
    MatrixXd m(rows, cols);
    for (int j = 0; j < cols; ++j) {
        for (int i = 0; i < rows; ++i) {
            m(i, j) = gaussian(gen);
        }
    }
    return m;
}

//=============================================================================================================

double correlation(const VectorXd& x, const VectorXd& y)
{
    const VectorXd xc = x.array() - x.mean();
    const VectorXd yc = y.array() - y.mean();
    return xc.dot(yc) / (xc.norm() * yc.norm());
}

//=============================================================================================================

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    std::seed_seq seed{7}; // fixed on purpose: the checks below need reproducible data
    std::mt19937 gen(seed);
    bool ok = true;

    MatrixXd mixing(4, 4); // channels x sources
    mixing << 1.0, 0.4, 0.2, 0.1, 0.3, 1.0, 0.5, 0.2, 0.2, 0.3, 1.0, 0.4, 0.1, 0.2, 0.3, 1.0;

    // CSP: source 0 is strong in class 0, source 1 is strong in class 1
    std::vector<MatrixXd> cspEpochs;
    VectorXi classes(40);
    for (int e = 0; e < 40; ++e) {
        classes(e) = e % 2;
        MatrixXd sources = noise(gen, 4, 200);
        sources.row(classes(e) == 0 ? 0 : 1) *= 4.0;
        cspEpochs.push_back(mixing * sources);
    }

    //! [decoding_csp_fit_transform]
    DecodingCsp csp(2); // one filter per class
    csp.fit(cspEpochs, classes);
    const MatrixXd cspFeatures = csp.transform(cspEpochs); // epochs x components, log band power
    //! [decoding_csp_fit_transform]
    // Which component favours which class depends on the eigenvector order; score both assignments
    int agree = 0;
    for (int e = 0; e < 40; ++e) {
        agree += (cspFeatures(e, 0) > cspFeatures(e, 1)) == (classes(e) == 1);
    }
    const int cspCorrect = std::max(agree, 40 - agree);
    ok &= expect(csp.isFitted() && cspFeatures.rows() == 40 && cspFeatures.cols() == 2 && cspCorrect == 40,
                 QString("CSP log-power features separate %1 of 40 epochs").arg(cspCorrect));

    // SPoC: the variance of source 2 tracks a continuous target
    std::vector<MatrixXd> spocEpochs;
    VectorXd target(40);
    for (int e = 0; e < 40; ++e) {
        target(e) = 1.0 + 3.0 * (e % 8) / 7.0;
        MatrixXd sources = noise(gen, 4, 200);
        sources.row(2) *= target(e);
        spocEpochs.push_back(mixing * sources);
    }

    //! [decoding_spoc_fit_transform]
    DecodingSpoc spoc(1);
    spoc.fit(spocEpochs, target);
    const MatrixXd spocFeatures = spoc.transform(spocEpochs); // epochs x 1, log band power
    //! [decoding_spoc_fit_transform]
    const double spocCorr = correlation(spocFeatures.col(0), target.array().log().matrix());
    ok &= expect(std::fabs(spocCorr) > 0.9, QString("SPoC power follows log(target): |r| %1 > 0.9").arg(std::fabs(spocCorr)));

    // SSD: source 3 is a 10 Hz rhythm, the others are broadband
    MatrixXd ssdSources = noise(gen, 4, 4000);
    for (int t = 0; t < ssdSources.cols(); ++t) {
        ssdSources(3, t) = 3.0 * std::sin(2.0 * kPi * 10.0 * t / kSFreq) + 0.3 * ssdSources(3, t);
    }
    const MatrixXd continuous = mixing * ssdSources;

    //! [decoding_ssd_fit_transform]
    DecodingSsd ssd(2);
    ssd.fit(continuous, kSFreq, 8.0, 12.0, 3.0, 20.0); // signal band, flanking noise band
    const MatrixXd rhythm = ssd.transform(continuous); // components x samples, strongest SNR first
    //! [decoding_ssd_fit_transform]
    const double ssdCorr = std::fabs(correlation(rhythm.row(0).transpose(), ssdSources.row(3).transpose()));
    ok &= expect(ssdCorr > 0.9 && ssd.eigenvalues()(0) > ssd.eigenvalues()(1),
                 QString("SSD component 0 recovers the 10 Hz source: |r| %1 > 0.9").arg(ssdCorr));

    // ICA labelling: blink-like, heartbeat-like, white (muscle-like) and slow (brain-like) components
    const int n = 2000;
    MatrixXd eog(1, n), ecg(1, n), icaSources(4, n);
    const MatrixXd white = noise(gen, 3, n);
    for (int t = 0; t < n; ++t) {
        eog(0, t) = std::exp(-std::pow(std::fmod(t, 400.0) - 200.0, 2) / 200.0);
        ecg(0, t) = std::exp(-std::pow(std::fmod(t, 170.0) - 85.0, 2) / 8.0);
        icaSources(0, t) = eog(0, t) + 0.05 * white(0, t);
        icaSources(1, t) = ecg(0, t) + 0.05 * white(1, t);
        icaSources(2, t) = white(2, t);
        icaSources(3, t) = std::sin(2.0 * kPi * 6.0 * t / kSFreq);
    }

    //! [ml_ica_label_classify]
    const QList<IcaLabelResult> labels = MlIcaLabel::classify(icaSources, eog, ecg, kSFreq);
    const QVector<int> reject = MlIcaLabel::findArtifactComponents(labels); // indices to drop before back-projection
    for (const IcaLabelResult& label : labels) {
        qInfo().noquote() << "component" << label.componentIndex << IcaLabelResult::labelToString(label.label)
                          << "confidence" << label.confidence;
    }
    //! [ml_ica_label_classify]
    ok &= expect(labels.size() == 4 && labels[0].label == IcaComponentLabel::Eog && labels[1].label == IcaComponentLabel::Ecg && labels[2].label == IcaComponentLabel::Muscle && labels[3].label == IcaComponentLabel::Brain,
                 "ICA labels: EOG, ECG, muscle, brain");
    ok &= expect(reject == QVector<int>({0, 1, 2}), "components 0-2 are rejected");

    qInfo() << (ok ? "All decoding checks passed." : "Decoding checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
