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
 */

#ifndef FILTER_CHPI_DSP_H
#define FILTER_CHPI_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB { class FiffInfo; }

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Parameters for cHPI notch filtering.
 */
struct DSPSHARED_EXPORT FilterChpiParams
{
    double dNotchWidth = 2.0;  /**< Half-width of each notch filter in Hz (notch = freq +/- width). */
    int    iFilterOrder = 4;   /**< Butterworth order for each notch filter. */
    bool   bMegOnly = true;    /**< If true, only filter MEG channels. If false, filter all channels. */
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
                                  const FIFFLIB::FiffInfo& info,
                                  double sfreq,
                                  const QVector<double>& hpiFreqs,
                                  const FilterChpiParams& params = FilterChpiParams());

//=============================================================================================================
/**
 * @brief Overload that attempts to extract cHPI frequencies from FiffInfo.
 *
 * Tries to read HPI coil frequencies from the FiffInfo metadata. Because the FiffInfo
 * HPI frequency storage varies across file versions, this overload may not always succeed.
 * If frequencies cannot be determined, a warning is logged and no filtering is applied.
 * In that case, use the explicit-frequency overload instead.
 *
 * @param[in,out] data    Data matrix (n_channels x n_times). Modified in-place.
 * @param[in]     info    Measurement info.
 * @param[in]     sfreq   Sampling frequency in Hz.
 * @param[in]     params  Filtering parameters.
 */
DSPSHARED_EXPORT void filterChpi(Eigen::MatrixXd& data,
                                  const FIFFLIB::FiffInfo& info,
                                  double sfreq,
                                  const FilterChpiParams& params = FilterChpiParams());

} // namespace UTILSLIB

#endif // FILTER_CHPI_DSP_H
