//=============================================================================================================
/**
 * @file     epoch_extractor.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Declaration of EpochExtractor — segments continuous MEG/EEG data into trials.
 *
 * EpochExtractor turns a continuous raw recording into a set of fixed-length trials
 * (epochs) locked to stimulus or response events.  Each epoch can optionally have
 * baseline correction applied and can be rejected if any channel exceeds a
 * peak-to-peak amplitude threshold.
 *
 * The output epochs are stored as MNEEpochData objects (data matrix + event code + tmin/tmax
 * + rejection flag) which integrate with the rest of the MNE-CPP analysis pipeline.
 */

#ifndef EPOCH_EXTRACTOR_DSP_H
#define EPOCH_EXTRACTOR_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <mne/mne_epoch_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Segments continuous raw data into fixed-length epochs locked to events.
 *
 * @code
 *   EpochExtractor::Params p;
 *   p.dTmin      = -0.2;     // 200 ms pre-stimulus
 *   p.dTmax      =  0.8;     // 800 ms post-stimulus
 *   p.dBaseMin   = -0.2;
 *   p.dBaseMax   =  0.0;
 *   p.dThreshold = 150e-6;   // 150 µV peak-to-peak rejection
 *
 *   QVector<MNELIB::MNEEpochData> epochs =
 *       EpochExtractor::extract(matRawData, eventSamples, sFreq, p);
 *
 *   // Good epochs only
 *   QVector<MNELIB::MNEEpochData> clean =
 *       EpochExtractor::rejectMarked(epochs);
 * @endcode
 */
class DSPSHARED_EXPORT EpochExtractor
{
public:
    //=========================================================================================================
    /**
     * @brief Parameters controlling epoch extraction, baseline correction, and rejection.
     */
    struct DSPSHARED_EXPORT Params
    {
        double dTmin      = -0.2;   /**< Epoch start relative to event in seconds (negative = pre-stimulus). */
        double dTmax      =  0.5;   /**< Epoch end relative to event in seconds. */
        double dBaseMin   = -0.2;   /**< Baseline start in seconds (relative to event). */
        double dBaseMax   =  0.0;   /**< Baseline end in seconds (relative to event). Set both to 0 to skip. */
        double dThreshold =  0.0;   /**< Peak-to-peak amplitude rejection threshold (SI units, e.g. V or T).
                                         0 = no rejection. Applied per-channel across all channels. */
        bool   bApplyBaseline = true; /**< Whether to apply baseline correction. */
    };

    //=========================================================================================================
    /**
     * Extract epochs from a continuous raw data matrix.
     *
     * For each event sample index the function:
     *  1. Cuts a window [tmin, tmax] around the event.
     *  2. Applies mean baseline correction over [baseMin, baseMax] (if bApplyBaseline and the window is valid).
     *  3. Marks the epoch for rejection if any channel has peak-to-peak > dThreshold (and dThreshold > 0).
     *
     * Epochs whose window extends outside the data boundaries are silently skipped.
     *
     * @param[in] matData       Continuous raw data (n_channels × n_samples), calibrated (SI units).
     * @param[in] eventSamples  0-based sample indices of events.
     * @param[in] dSFreq        Sampling frequency in Hz.
     * @param[in] params        Extraction parameters.
     * @param[in] eventCodes    Optional per-event integer codes stored in MNEEpochData::event.
     *                          Must be empty or the same length as eventSamples.
     *
     * @return Vector of MNEEpochData (one per valid event).  Rejected epochs are included with
     *         MNEEpochData::bReject == true so callers can choose whether to exclude them.
     */
    static QVector<MNELIB::MNEEpochData> extract(const Eigen::MatrixXd&  matData,
                                                   const QVector<int>&     eventSamples,
                                                   double                  dSFreq,
                                                   const Params&           params     = Params(),
                                                   const QVector<int>&     eventCodes = QVector<int>());

    //=========================================================================================================
    /**
     * Compute the grand average (ERP/ERF) across all non-rejected epochs.
     *
     * @param[in] epochs  Vector of extracted epochs (output of extract()).
     *
     * @return Mean data matrix (n_channels × n_epoch_samples), or empty matrix if no good epochs.
     */
    static Eigen::MatrixXd average(const QVector<MNELIB::MNEEpochData>& epochs);

    //=========================================================================================================
    /**
     * Return only the epochs that are NOT marked for rejection.
     *
     * @param[in] epochs  Input epoch vector.
     *
     * @return Subset of epochs with bReject == false.
     */
    static QVector<MNELIB::MNEEpochData> rejectMarked(const QVector<MNELIB::MNEEpochData>& epochs);

private:
    //=========================================================================================================
    /**
     * Apply mean baseline correction in-place.
     *
     * Subtracts the per-channel mean computed over the baseline window [iBase0, iBase1] (inclusive,
     * 0-based column indices into the epoch matrix) from every sample.
     *
     * @param[in,out] matEpoch  Epoch data matrix (n_channels × n_samples), modified in place.
     * @param[in]     iBase0    First baseline sample column.
     * @param[in]     iBase1    Last baseline sample column (inclusive).
     */
    static void applyBaseline(Eigen::MatrixXd& matEpoch, int iBase0, int iBase1);
};

} // namespace UTILSLIB

#endif // EPOCH_EXTRACTOR_DSP_H
