//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_chpi.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Declaration of filterChpi — cHPI signal removal by notch filtering.
 *
 * Removes continuous head-position indicator (cHPI) excitation signals from MEG data by
 * applying zero-phase Butterworth band-stop (notch) filters at each cHPI frequency.
 * This mirrors MNE-Python's mne.chpi.filter_chpi().
 *
 * @snippet ex_dsp_maxwell/main.cpp filter_chpi_usage
 */

#ifndef FILTER_CHPI_DSP_H
#define FILTER_CHPI_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Parameters for cHPI notch filtering.
 *
 * @snippet ex_dsp_maxwell/main.cpp filter_chpi_usage
 */
struct DSPSHARED_EXPORT FilterChpiParams
{
    double dNotchWidth = 2.0; /**< Half-width of each notch filter in Hz (notch = freq +/- width). */
    int iFilterOrder = 4;     /**< Butterworth order for each notch filter. */
    bool bMegOnly = true;     /**< If true, only filter MEG channels. If false, filter all channels. */
};

//=============================================================================================================
/**
 * @brief Remove cHPI excitation signals from MEG data by notch filtering.
 *
 * Applies a zero-phase Butterworth band-stop (notch) filter at each specified
 * cHPI frequency. Only MEG channels are filtered by default.
 *
 * @param[in,out] data         Data matrix (n_channels x n_times). Modified in-place.
 * @param[in]     info         Measurement info (used to identify MEG channels if bMegOnly is true).
 * @param[in]     sfreq        Sampling frequency in Hz.
 * @param[in]     hpiFreqs     List of cHPI excitation frequencies in Hz.
 *                             Typical Elekta/MEGIN system: {83, 143, 203, 263, 293}.
 * @param[in]     params       Filtering parameters.
 */
DSPSHARED_EXPORT void filterChpi(Eigen::MatrixXd& data,
                                 const mne::FiffInfo& info,
                                 double sfreq,
                                 const std::vector<double>& hpiFreqs,
                                 const FilterChpiParams& params = FilterChpiParams());

//=============================================================================================================
/**
 * @brief Removes the excitation frequencies of the HPI coils in the measurement info.
 *
 * Without HPI coil frequencies in the info, a warning is logged and the data stays unchanged.
 *
 * @param[in,out] data    Data matrix (n_channels x n_times). Modified in-place.
 * @param[in]     info    Measurement info with the HPI coil frequencies.
 * @param[in]     sfreq   Sampling frequency in Hz.
 * @param[in]     params  Filtering parameters.
 */
DSPSHARED_EXPORT void filterChpi(Eigen::MatrixXd& data,
                                 const mne::FiffInfo& info,
                                 double sfreq,
                                 const FilterChpiParams& params = FilterChpiParams());

} // namespace mne

#endif // FILTER_CHPI_DSP_H
