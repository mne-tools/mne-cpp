//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     coherence.cpp
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Implementation of @ref mne::Coherence - magnitude-squared coherence between every channel pair, computed from the shared DPSS-tapered cross-spectral cache.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
#include <base/log.h>
#include "coherence.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include "../connectivitysettings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Coherence::Coherence()
{
}

//*******************************************************************************************************

Network Coherence::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("COH");

    if (connectivitySettings.empty()) {
        mne::Log::debug("Coherence::calculate - Input data is empty");
        return finalNetwork;
    }

    if (!connectivitySettings.keeps_trial_data()) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

    int iNfft = connectivitySettings.getFFTSize();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;


    // Pass information about the FFT length. Use iNFreqs because we only use the half spectrum
    finalNetwork.set_fft_size(iNFreqs);
    finalNetwork.set_used_freq_bins(connectivitySettings.frequency_bins().second);

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    add_nodes(finalNetwork, rows, connectivitySettings.getNodePositions());

    //Calculate all-to-all coherence matrix over epochs
    Coherency::calculateAbs(finalNetwork,
                            connectivitySettings);
    ;

    return finalNetwork;
}
