//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_chpi.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of filterChpi — cHPI signal removal by notch filtering.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_chpi.h"
#include <base/log.h>

#include <vector>
#include "iirfilter.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Collect indices of MEG channels from info.
 */
static std::vector<int> megChannelIndices(const FiffInfo& info)
{
    std::vector<int> indices;
    for (int i = 0; i < std::ssize(info.chs); ++i) {
        if (info.chs[i].kind == FIFFV_MEG_CH) {
            indices.push_back(i);
        }
    }
    return indices;
}

//=============================================================================================================
// DEFINITIONS
//=============================================================================================================

void mne::filterChpi(MatrixXd& data,
                     const FiffInfo& info,
                     double sfreq,
                     const std::vector<double>& hpiFreqs,
                     const FilterChpiParams& params)
{
    if (hpiFreqs.empty()) {
        return;
    }

    if (sfreq <= 0.0) {
        Log::warning("filterChpi: Invalid sampling frequency {} Hz. Skipping.", sfreq);
        return;
    }

    const double dNyquist = sfreq / 2.0;

    // Collect valid frequencies
    std::vector<double> validFreqs;
    for (const double freq : hpiFreqs) {
        const double fLow = freq - params.dNotchWidth;
        const double fHigh = freq + params.dNotchWidth;
        if (freq <= 0.0 || fHigh >= dNyquist) {
            Log::warning("filterChpi: Skipping invalid cHPI frequency {} Hz (Nyquist = {} Hz).", freq, dNyquist);
            continue;
        }
        if (fLow <= 0.0) {
            Log::warning("filterChpi: Skipping cHPI frequency {} Hz (notch lower edge <= 0 Hz).", freq);
            continue;
        }
        validFreqs.push_back(freq);
    }

    if (validFreqs.empty()) {
        return;
    }

    if (params.bMegOnly) {
        // Filter only MEG channels
        const std::vector<int> megIdx = megChannelIndices(info);
        if (megIdx.empty()) {
            Log::warning("filterChpi: No MEG channels found in FiffInfo. Nothing to filter.");
            return;
        }

        // Extract MEG submatrix
        MatrixXd megData(megIdx.size(), data.cols());
        for (int i = 0; i < std::ssize(megIdx); ++i) {
            megData.row(i) = data.row(megIdx[i]);
        }

        // Apply notch filters sequentially
        for (const double freq : validFreqs) {
            const double fLow = freq - params.dNotchWidth;
            const double fHigh = freq + params.dNotchWidth;

            std::vector<IirBiquad> sos = IirFilter::designButterworth(
                params.iFilterOrder, IirFilter::BandStop, fLow, fHigh, sfreq);

            megData = IirFilter::applyZeroPhaseMatrix(megData, sos);
        }

        // Write back
        for (int i = 0; i < std::ssize(megIdx); ++i) {
            data.row(megIdx[i]) = megData.row(i);
        }
    } else {
        // Filter all channels
        for (const double freq : validFreqs) {
            const double fLow = freq - params.dNotchWidth;
            const double fHigh = freq + params.dNotchWidth;

            std::vector<IirBiquad> sos = IirFilter::designButterworth(
                params.iFilterOrder, IirFilter::BandStop, fLow, fHigh, sfreq);

            data = IirFilter::applyZeroPhaseMatrix(data, sos);
        }
    }
}

//=============================================================================================================

void mne::filterChpi(MatrixXd& data,
                     const FiffInfo& info,
                     double sfreq,
                     const FilterChpiParams& params)
{
    // Attempt to extract HPI coil frequencies from FiffInfo.
    // The HPI frequency information may be stored in various FIFF blocks
    // (FIFFB_HPI_MEAS, FIFFB_HPI_SUBSYSTEM), but the FiffInfo C++ struct
    // does not currently expose a parsed list of HPI excitation frequencies.
    // This overload is provided for API completeness; users should prefer
    // passing frequencies explicitly.
    Q_UNUSED(data)
    Q_UNUSED(info)
    Q_UNUSED(sfreq)
    Q_UNUSED(params)

    Log::warning("filterChpi: Automatic cHPI frequency extraction from FiffInfo is not yet implemented. Please provide frequencies explicitly via the hpiFreqs parameter.");
}
