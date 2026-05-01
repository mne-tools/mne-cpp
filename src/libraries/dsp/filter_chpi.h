//=============================================================================================================
/**
 * @file     filter_chpi.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
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
