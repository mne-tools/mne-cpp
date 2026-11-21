//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     compute_connectivity.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Runs the requested connectivity metrics over a @ref mne::ConnectivitySettings batch.
 *
 * The caller fills a @ref mne::ConnectivitySettings object with the trial data, sampling frequency,
 * FFT length, taper window, node positions and the method names ("COH", "IMAGCOH", "PLI", "WPLI", "DSWPLI",
 * "USPLI", "PLV", "COR", "XCOR", "GC", "DTF", "PDC"); @ref mne::compute_connectivity returns one
 * @ref mne::Network per method. The DPSS-tapered FFTs and spectral sums are cached on the settings,
 * so several spectral methods cost one FFT pass.
 */

#ifndef COMPUTE_CONNECTIVITY_H
#define COMPUTE_CONNECTIVITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"
#include "network/network.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class ConnectivitySettings;

//=============================================================================================================
/**
 * Computes every connectivity method the settings ask for, as mne_connectivity.spectral_connectivity_epochs does
 * for its spectral methods.
 *
 * @param[in, out] settings   Trials, sampling and method names; the spectral intermediate sums are cached on it.
 *
 * @return One network per requested method, in the order WPLI, USPLI, COR, XCOR, PLI, COH, IMAGCOH, PLV, DSWPLI, GC,
 *         DTF, PDC (methods not requested are skipped).
 */
CONNECTIVITYSHARED_EXPORT std::vector<Network> compute_connectivity(ConnectivitySettings& settings);

} // namespace mne

#endif // COMPUTE_CONNECTIVITY_H
