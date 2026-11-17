//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     imagcoherence.cpp
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Implementation of @ref mne::ImagCoherence - the volume-conduction-robust imaginary part of complex coherency of Nolte et al. (NeuroImage 2004).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
#include <base/log.h>
#include "imagcoherence.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

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

ImagCoherence::ImagCoherence()
{
}

//*******************************************************************************************************

Network ImagCoherence::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("IMAGCOH");

    if (connectivitySettings.empty()) {
        mne::Log::debug("ImagCoherence::calculate - Input data is empty");
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

    //Calculate all-to-all imaginary coherence matrix over epochs
    Coherency::calculateImag(finalNetwork,
                             connectivitySettings);

    return finalNetwork;
}
