//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Connectivity metrics on synthetic data with a known coupling.
 *
 * Channel 1 is channel 0 delayed by 12 ms plus independent noise; channel 2 is
 * independent noise. Every metric of the connectivity library is computed and
 * the example exits non-zero unless it recovers that structure, so it doubles
 * as a deterministic check.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/metrics/abstractmetric.h>
#include <connectivity/metrics/coherence.h>
#include <connectivity/metrics/coherency.h>
#include <connectivity/metrics/correlation.h>
#include <connectivity/metrics/crosscorrelation.h>
#include <connectivity/metrics/debiasedsquaredweightedphaselagindex.h>
#include <connectivity/metrics/directed_transfer_function.h>
#include <connectivity/metrics/granger_causality.h>
#include <connectivity/metrics/imagcoherence.h>
#include <connectivity/metrics/mvar_model.h>
#include <connectivity/metrics/partial_directed_coherence.h>
#include <connectivity/metrics/phaselagindex.h>
#include <connectivity/metrics/phaselockingvalue.h>
#include <connectivity/metrics/unbiasedsquaredphaselagindex.h>
#include <connectivity/metrics/weightedphaselagindex.h>
#include <connectivity/network/network.h>
#include <connectivity/network/networkedge.h>
#include <connectivity/network/networknode.h>

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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr int kSFreq = 250;
constexpr int kSamples = 500;
constexpr int kTrials = 20;
constexpr int kDelay = 3; // samples, 12 ms

//=============================================================================================================
/**
 * Standard normal deviate from the raw mt19937 stream via Box-Muller, so the data are identical on every
 * standard library (std::normal_distribution is implementation-defined).
 */
double gaussian(std::mt19937& gen)
{
    const double u1 = (static_cast<double>(gen()) + 0.5) / 4294967296.0;
    const double u2 = (static_cast<double>(gen()) + 0.5) / 4294967296.0;
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
}

//=============================================================================================================

MatrixXd makeTrial(std::mt19937& gen)
{
    const double phase = 2.0 * kPi * (static_cast<double>(gen()) / 4294967296.0);
    VectorXd source(kSamples + kDelay);
    for (int t = 0; t < source.size(); ++t) {
        source(t) = std::sin(2.0 * kPi * 10.0 * t / kSFreq + phase) + 0.5 * gaussian(gen);
    }

    MatrixXd trial(3, kSamples);
    for (int t = 0; t < kSamples; ++t) {
        trial(0, t) = source(t + kDelay);
        trial(1, t) = source(t) + 0.5 * gaussian(gen);
        trial(2, t) = gaussian(gen);
    }
    return trial;
}

//=============================================================================================================
/**
 * Band-averaged weight of the edge start -> end, or -1 if the network has no such edge.
 */
double edgeWeight(const Network& network, int start, int end)
{
    for (const NetworkEdge::SPtr& edge : network.getFullEdges()) {
        if (edge->getStartNodeID() == start && edge->getEndNodeID() == end) {
            return edge->getWeight();
        }
    }
    return -1.0;
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
    std::seed_seq seed{42}; // fixed on purpose: the checks below need reproducible data
    std::mt19937 gen(seed);
    bool ok = true;

    //! [connectivity_settings_setup]
    ConnectivitySettings settings;
    for (int i = 0; i < kTrials; ++i) {
        settings.append(makeTrial(gen)); // channels x samples, one call per trial
    }
    settings.setSamplingFrequency(kSFreq);
    settings.setFFTSize(kSamples);
    settings.setWindowType("hanning");

    MatrixX3f positions(3, 3);
    positions << 0.0f, 0.0f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.05f, 0.0f;
    settings.setNodePositions(positions);
    //! [connectivity_settings_setup]

    //! [abstract_metric_band]
    // Spectral metrics evaluate the bins [start, start + amount); -1 means the full half spectrum.
    AbstractMetric::m_iNumberBinStart = -1;
    AbstractMetric::m_iNumberBinAmount = -1;
    AbstractMetric::m_bStorageModeIsActive = false;
    //! [abstract_metric_band]

    //! [connectivity_calculate]
    settings.setConnectivityMethods({"COH", "PLV", "WPLI"});
    const QList<Network> networks = Connectivity::calculate(settings);
    for (Network network : networks) {
        network.setFrequencyRange(8.0f, 12.0f); // average the per-bin weights over the alpha band
        qInfo().noquote() << network.getConnectivityMethod() << "0-1:" << edgeWeight(network, 0, 1);
    }
    //! [connectivity_calculate]
    ok &= expect(networks.size() == 3, "Connectivity::calculate returns one network per method");

    //! [coherence_calculate]
    Network coherence = Coherence::calculate(settings);
    coherence.setFrequencyRange(8.0f, 12.0f);
    const double coh01 = edgeWeight(coherence, 0, 1);
    const double coh02 = edgeWeight(coherence, 0, 2);
    //! [coherence_calculate]
    ok &= expect(coh01 > coh02 + 0.3, QString("COH 0-1 %1 > 0-2 %2 + 0.3").arg(coh01).arg(coh02));

    //! [imag_coherence_calculate]
    Network imagCoherence = ImagCoherence::calculate(settings);
    imagCoherence.setFrequencyRange(8.0f, 12.0f);
    const double icoh01 = std::fabs(edgeWeight(imagCoherence, 0, 1));
    //! [imag_coherence_calculate]
    ok &= expect(icoh01 > 0.2, QString("|ImCOH| 0-1 %1 > 0.2 (12 ms lag)").arg(icoh01));

    //! [coherency_calculate_abs]
    // Coherency fills a network whose nodes already exist; the metric classes above wrap this.
    settings.clearIntermediateData();
    Network coherency("COHY");
    coherency.setSamplingFrequency(kSFreq);
    coherency.setFFTSize(kSamples / 2 + 1);
    coherency.setUsedFreqBins(kSamples / 2 + 1);
    for (int i = 0; i < 3; ++i) {
        coherency.append(NetworkNode::SPtr(new NetworkNode(i, positions.row(i))));
    }
    Coherency::calculateAbs(coherency, settings);
    coherency.setFrequencyRange(8.0f, 12.0f);
    //! [coherency_calculate_abs]
    ok &= expect(std::fabs(edgeWeight(coherency, 0, 1) - coh01) < 1e-9, "Coherency::calculateAbs equals Coherence");

    //! [phase_lag_index_calculate]
    Network pli = PhaseLagIndex::calculate(settings);
    pli.setFrequencyRange(8.0f, 12.0f);
    //! [phase_lag_index_calculate]
    //! [weighted_phase_lag_index_calculate]
    Network wpli = WeightedPhaseLagIndex::calculate(settings);
    wpli.setFrequencyRange(8.0f, 12.0f);
    //! [weighted_phase_lag_index_calculate]
    //! [unbiased_squared_phase_lag_index_calculate]
    Network uspli = UnbiasedSquaredPhaseLagIndex::calculate(settings);
    uspli.setFrequencyRange(8.0f, 12.0f);
    //! [unbiased_squared_phase_lag_index_calculate]
    //! [debiased_squared_weighted_phase_lag_index_calculate]
    Network dswpli = DebiasedSquaredWeightedPhaseLagIndex::calculate(settings);
    dswpli.setFrequencyRange(8.0f, 12.0f);
    //! [debiased_squared_weighted_phase_lag_index_calculate]
    //! [phase_locking_value_calculate]
    Network plv = PhaseLockingValue::calculate(settings);
    plv.setFrequencyRange(8.0f, 12.0f);
    //! [phase_locking_value_calculate]
    for (const Network* network : {&pli, &wpli, &uspli, &dswpli, &plv}) {
        const double w01 = edgeWeight(*network, 0, 1);
        const double w02 = edgeWeight(*network, 0, 2);
        ok &= expect(w01 > w02 + 0.3,
                     QString("%1 0-1 %2 > 0-2 %3 + 0.3").arg(network->getConnectivityMethod()).arg(w01).arg(w02));
    }

    //! [correlation_calculate]
    Network correlation = Correlation::calculate(settings);
    const double cor01 = edgeWeight(correlation, 0, 1);
    //! [correlation_calculate]
    //! [cross_correlation_calculate]
    Network crossCorrelation = CrossCorrelation::calculate(settings);
    const double xcor01 = edgeWeight(crossCorrelation, 0, 1);
    //! [cross_correlation_calculate]
    // Zero-lag Pearson r of the model: 0.5 cos(2 pi 10 Hz 12 ms) / sqrt(0.75 * 1.0) = 0.42
    ok &= expect(std::fabs(cor01 - 0.42) < 0.05 && std::fabs(edgeWeight(correlation, 0, 2)) < 0.1,
                 QString("COR 0-1 %1 within 0.05 of 0.42, |0-2| < 0.1").arg(cor01));
    ok &= expect(xcor01 > 2.0 * std::fabs(edgeWeight(crossCorrelation, 0, 2)),
                 QString("XCOR 0-1 %1 > 2 x |0-2|").arg(xcor01));

    //! [mvar_model_fit]
    MatrixXd average = MatrixXd::Zero(3, kSamples);
    for (int i = 0; i < settings.size(); ++i) {
        average += settings.at(i).matData / settings.size();
    }
    MvarModel mvar;
    mvar.fit(average, 4);                                       // pass 0 to select the order by BIC
    const QVector<MatrixXd> coefficients = mvar.coefficients(); // A_1 .. A_p
    const QVector<MatrixXcd> transfer = mvar.transferFunction(VectorXd::LinSpaced(5, 0.0, 0.5));
    //! [mvar_model_fit]
    ok &= expect(mvar.order() == 4 && coefficients.size() == 4 && transfer.size() == 5,
                 "MvarModel stores order 4 and evaluates H(f) at 5 frequencies");
    ok &= expect(std::fabs(coefficients[kDelay - 1](1, 0)) > 0.5,
                 QString("|A_3(1,0)| %1 > 0.5: channel 1 depends on channel 0 three samples back")
                     .arg(std::fabs(coefficients[kDelay - 1](1, 0))));

    //! [granger_causality_calculate]
    Network granger = GrangerCausality::calculate(settings); // edge j -> i: j Granger-causes i
    //! [granger_causality_calculate]
    //! [directed_transfer_function_calculate]
    Network dtf = DirectedTransferFunction::calculate(settings);
    //! [directed_transfer_function_calculate]
    //! [partial_directed_coherence_calculate]
    Network pdc = PartialDirectedCoherence::calculate(settings);
    //! [partial_directed_coherence_calculate]
    for (const Network* network : {&granger, &dtf, &pdc}) {
        const double forward = edgeWeight(*network, 0, 1);
        const double backward = edgeWeight(*network, 1, 0);
        ok &= expect(forward > 2.0 * backward,
                     QString("%1 0->1 %2 > 2 x 1->0 %3").arg(network->getConnectivityMethod()).arg(forward).arg(backward));
    }

    //! [network_build]
    Network network("manual");
    for (int i = 0; i < 3; ++i) {
        network.append(NetworkNode::SPtr(new NetworkNode(i, positions.row(i))));
    }
    const QList<QPair<int, int>> pairs = {{0, 1}, {1, 2}};
    const QList<double> weights = {0.9, 0.2};
    for (int k = 0; k < pairs.size(); ++k) {
        MatrixXd perBin = MatrixXd::Constant(1, 1, weights[k]);
        NetworkEdge::SPtr edge(new NetworkEdge(pairs[k].first, pairs[k].second, perBin));
        network.getNodeAt(pairs[k].first)->append(edge);
        network.getNodeAt(pairs[k].second)->append(edge);
        network.append(edge);
    }
    //! [network_build]

    //! [network_threshold]
    network.setThreshold(0.5);
    const MatrixXd adjacency = network.getFullConnectivityMatrix(); // symmetric by default
    const int strongEdges = network.getThresholdedEdges().size();
    const NetworkNode::SPtr hub = network.getNodeAt(1);
    //! [network_threshold]
    ok &= expect(adjacency(1, 0) == 0.9 && adjacency(2, 1) == 0.2 && strongEdges == 1,
                 "manual network: mirrored adjacency, one edge above 0.5");
    ok &= expect(hub->getFullDegree() == 2 && hub->getThresholdedDegree() == 1 && std::fabs(hub->getFullStrength() - 1.1) < 1e-12,
                 "node 1: degree 2, thresholded degree 1, strength 1.1");

    qInfo() << (ok ? "All connectivity checks passed." : "Connectivity checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
