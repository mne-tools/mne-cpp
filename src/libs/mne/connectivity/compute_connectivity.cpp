//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     compute_connectivity.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           mfarisyahya <mfarisyahya@gmail.com>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Implementation of @ref mne::compute_connectivity, the dispatcher that runs the requested functional-connectivity metrics over a @ref mne::ConnectivitySettings batch.
 *
 * The single entry point @ref mne::compute_connectivity inspects the
 * list of method names on @ref mne::ConnectivitySettings and routes
 * each request to the corresponding estimator (Coherence, Imaginary
 * Coherence, PLI / wPLI / dwPLI / USPLI, PLV, Pearson and cross
 * correlation, Granger Causality, DTF, PDC). All spectral metrics share
 * the DPSS tapered FFT and CSD accumulator cached on the settings object,
 * so requesting several methods at once costs only one FFT pass.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compute_connectivity.h"

#include "connectivitysettings.h"
#include "network/network.h"
#include "metrics/correlation.h"
#include "metrics/crosscorrelation.h"
#include "metrics/coherence.h"
#include "metrics/imagcoherence.h"
#include "metrics/phaselagindex.h"
#include "metrics/phaselockingvalue.h"
#include "metrics/weightedphaselagindex.h"
#include "metrics/unbiasedsquaredphaselagindex.h"
#include "metrics/debiasedsquaredweightedphaselagindex.h"
#include "metrics/granger_causality.h"
#include "metrics/directed_transfer_function.h"
#include "metrics/partial_directed_coherence.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <string_view>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;


//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::vector<Network> mne::compute_connectivity(ConnectivitySettings& settings)
{
    using Method = Network (*)(ConnectivitySettings&);
    static constexpr std::pair<std::string_view, Method> methods[] = {
        {"WPLI", &WeightedPhaseLagIndex::calculate},
        {"USPLI", &UnbiasedSquaredPhaseLagIndex::calculate},
        {"COR", &Correlation::calculate},
        {"XCOR", &CrossCorrelation::calculate},
        {"PLI", &PhaseLagIndex::calculate},
        {"COH", &Coherence::calculate},
        {"IMAGCOH", &ImagCoherence::calculate},
        {"PLV", &PhaseLockingValue::calculate},
        {"DSWPLI", &DebiasedSquaredWeightedPhaseLagIndex::calculate},
        {"GC", &GrangerCausality::calculate},
        {"DTF", &DirectedTransferFunction::calculate},
        {"PDC", &PartialDirectedCoherence::calculate},
    };

    const std::vector<std::string>& requested = settings.getConnectivityMethods();
    std::vector<Network> networks;
    networks.reserve(requested.size());
    for (const auto& [name, method] : methods) {
        if (std::ranges::find(requested, name) != requested.end()) {
            networks.push_back(method(settings));
        }
    }
    return networks;
}
