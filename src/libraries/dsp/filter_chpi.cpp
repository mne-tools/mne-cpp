//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filter_chpi.cpp
 * @since May 2026
 * @brief Implementation of filterChpi — cHPI signal removal by notch filtering.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_chpi.h"
#include "iirfilter.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Collect indices of MEG channels from info.
 */
static QVector<int> megChannelIndices(const FiffInfo& info)
{
    QVector<int> indices;
    for (int i = 0; i < info.chs.size(); ++i) {
        if (info.chs[i].kind == FIFFV_MEG_CH) {
            indices.append(i);
        }
    }
    return indices;
}

//=============================================================================================================
// DEFINITIONS
//=============================================================================================================

void UTILSLIB::filterChpi(MatrixXd& data,
                           const FiffInfo& info,
                           double sfreq,
                           const QVector<double>& hpiFreqs,
                           const FilterChpiParams& params)
{
    if (hpiFreqs.isEmpty()) {
        return;
    }

    if (sfreq <= 0.0) {
        qWarning() << "filterChpi: Invalid sampling frequency" << sfreq << "Hz. Skipping.";
        return;
    }

    const double dNyquist = sfreq / 2.0;

    // Collect valid frequencies
    QVector<double> validFreqs;
    for (const double freq : hpiFreqs) {
        const double fLow = freq - params.dNotchWidth;
        const double fHigh = freq + params.dNotchWidth;
        if (freq <= 0.0 || fHigh >= dNyquist) {
            qWarning() << "filterChpi: Skipping invalid cHPI frequency" << freq
                        << "Hz (Nyquist =" << dNyquist << "Hz).";
            continue;
        }
        if (fLow <= 0.0) {
            qWarning() << "filterChpi: Skipping cHPI frequency" << freq
                        << "Hz (notch lower edge <= 0 Hz).";
            continue;
        }
        validFreqs.append(freq);
    }

    if (validFreqs.isEmpty()) {
        return;
    }

    if (params.bMegOnly) {
        // Filter only MEG channels
        const QVector<int> megIdx = megChannelIndices(info);
        if (megIdx.isEmpty()) {
            qWarning() << "filterChpi: No MEG channels found in FiffInfo. Nothing to filter.";
            return;
        }

        // Extract MEG submatrix
        MatrixXd megData(megIdx.size(), data.cols());
        for (int i = 0; i < megIdx.size(); ++i) {
            megData.row(i) = data.row(megIdx[i]);
        }

        // Apply notch filters sequentially
        for (const double freq : validFreqs) {
            const double fLow = freq - params.dNotchWidth;
            const double fHigh = freq + params.dNotchWidth;

            QVector<IirBiquad> sos = IirFilter::designButterworth(
                params.iFilterOrder, IirFilter::BandStop, fLow, fHigh, sfreq);

            megData = IirFilter::applyZeroPhaseMatrix(megData, sos);
        }

        // Write back
        for (int i = 0; i < megIdx.size(); ++i) {
            data.row(megIdx[i]) = megData.row(i);
        }
    } else {
        // Filter all channels
        for (const double freq : validFreqs) {
            const double fLow = freq - params.dNotchWidth;
            const double fHigh = freq + params.dNotchWidth;

            QVector<IirBiquad> sos = IirFilter::designButterworth(
                params.iFilterOrder, IirFilter::BandStop, fLow, fHigh, sfreq);

            data = IirFilter::applyZeroPhaseMatrix(data, sos);
        }
    }
}

//=============================================================================================================

void UTILSLIB::filterChpi(MatrixXd& data,
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

    qWarning() << "filterChpi: Automatic cHPI frequency extraction from FiffInfo is not yet "
                  "implemented. Please provide frequencies explicitly via the hpiFreqs parameter.";
}
