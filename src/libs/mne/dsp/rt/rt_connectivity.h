//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     rt_connectivity.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2018
 * @brief    Streaming functional connectivity over a sliding window of trials.
 *
 * RtConnectivity estimates the connectivity networks of @ref mne::compute_connectivity (coherence,
 * imaginary coherence, PLI, wPLI, dwPLI, USPLI, PLV, correlation, ...) over the newest trials of a stream.
 * It keeps the tapered spectra and cross-spectra of every trial in the window together with their sums, so
 * a new trial costs its own spectra plus the final normalization, and the oldest trial is subtracted from
 * the sums instead of recomputing the rest. It is synchronous: call it from whichever thread receives the
 * trials.
 */

#ifndef RT_CONNECTIVITY_RTPROCESSING_H
#define RT_CONNECTIVITY_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <mne/connectivity/connectivitysettings.h>
#include <mne/connectivity/network/network.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Connectivity networks of the newest trials of a stream, updated trial by trial.
 *
 * The networks equal those @ref mne::compute_connectivity computes from the trials in the window.
 *
 * @snippet ex_dsp_rt/main.cpp rt_connectivity_usage
 */
class DSPSHARED_EXPORT RtConnectivity
{
public:
    //=========================================================================================================
    /**
     * @param[in] settings   Methods, sampling rate, FFT size, window type, frequency bins and node positions;
     *                       trials it already holds start the window.
     * @param[in] trials     Number of newest trials the networks are estimated from; 0 keeps every trial.
     */
    explicit RtConnectivity(ConnectivitySettings settings, int trials = 0);

    //=========================================================================================================
    /**
     * Adds a trial, drops the oldest ones beyond the window, and estimates the networks. A trial of another
     * size than those in the window starts the window over.
     *
     * @param[in] trial   Channels by samples.
     *
     * @return One network per method of the settings.
     */
    [[nodiscard]] std::vector<Network> update(const Eigen::MatrixXd& trial);

    //=========================================================================================================
    /**
     * @return The networks of the trials in the window, e.g. after the methods changed; none for an empty
     *         window.
     */
    [[nodiscard]] std::vector<Network> networks();

    //=========================================================================================================
    /**
     * @param[in] trials   Number of newest trials the networks are estimated from; 0 keeps every trial.
     */
    void set_trial_count(int trials);

    //=========================================================================================================
    /**
     * @return The settings with the trials of the window, to change the methods, frequency bins or window
     *         type; the kept spectra stay valid for a change of methods.
     */
    [[nodiscard]] ConnectivitySettings& settings();

    //=========================================================================================================
    /**
     * Forgets every trial.
     */
    void reset();

private:
    void trim();

    ConnectivitySettings m_settings; /**< Settings and the trials of the window. */
    int m_trials = 0;                /**< Window length in trials; 0 for every trial. */
};

} // namespace mne

#endif // RT_CONNECTIVITY_RTPROCESSING_H
