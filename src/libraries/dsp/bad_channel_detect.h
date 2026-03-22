//=============================================================================================================
/**
 * @file     bad_channel_detect.h
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
 * @brief    Declaration of BadChannelDetect — automated detection of bad MEG/EEG channels.
 *
 * Three complementary detection strategies are provided:
 *
 *  1. **Flat signal** — channels whose peak-to-peak amplitude is below a minimum
 *     threshold over the whole segment (disconnected electrode, saturated ADC stuck at rail, etc.).
 *
 *  2. **High variance** — channels whose standard deviation is more than @p dZThresh standard
 *     deviations above the median across channels (muscle artefact, loose connection, large drift).
 *
 *  3. **Low correlation** — channels that correlate poorly with their neighbours in the channel
 *     list.  A bad channel appears isolated: its Pearson correlation with the surrounding
 *     ±@p iNeighbours channels is below @p dCorrThresh.  This catches channels with
 *     coherent but spurious signals that pass the variance test.
 *
 * The three detectors can be run independently or combined via detect() which applies all three
 * and returns the union of bad channel row indices.
 */

#ifndef BAD_CHANNEL_DETECT_DSP_H
#define BAD_CHANNEL_DETECT_DSP_H

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
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Automated detection of bad MEG/EEG channels using flat, variance, and correlation criteria.
 *
 * @code
 *   // Run all three detectors with default parameters
 *   BadChannelDetect::Params p;
 *   QVector<int> bad = BadChannelDetect::detect(matData, p);
 *
 *   // Or run individual detectors
 *   QVector<int> flat  = BadChannelDetect::detectFlat(matData);
 *   QVector<int> noisy = BadChannelDetect::detectHighVariance(matData);
 *   QVector<int> weird = BadChannelDetect::detectLowCorrelation(matData);
 * @endcode
 */
class DSPSHARED_EXPORT BadChannelDetect
{
public:
    //=========================================================================================================
    /**
     * @brief Combined detection parameters.
     */
    struct Params
    {
        // Flat signal
        double dFlatThreshold = 1e-13;  /**< Peak-to-peak below this → flat (SI units; ~0.1 fT for MEG). */

        // High variance
        double dVarZThresh    = 4.0;    /**< Z-score of std-dev above this → noisy. */

        // Low correlation
        double dCorrThresh    = 0.4;    /**< Mean absolute neighbour correlation below this → isolated. */
        int    iNeighbours    = 5;      /**< Number of channels on each side to use as neighbours. */
    };

    //=========================================================================================================
    /**
     * Run all three detectors and return the union of bad channel row indices.
     *
     * The order of the returned indices is ascending and each index appears at most once.
     *
     * @param[in] matData  Data matrix (n_channels × n_samples), calibrated (SI units).
     * @param[in] params   Detection parameters.
     *
     * @return Sorted list of bad channel row indices (0-based).
     */
    static QVector<int> detect(const Eigen::MatrixXd& matData,
                                const Params&          params = Params());

    //=========================================================================================================
    /**
     * Detect flat (dead) channels.
     *
     * A channel is flat if its peak-to-peak amplitude over the whole segment is < @p dThreshold.
     *
     * @param[in] matData    Data matrix (n_channels × n_samples).
     * @param[in] dThreshold Minimum peak-to-peak amplitude (SI units). Default 1e-13 T (0.1 fT).
     *
     * @return Row indices of flat channels.
     */
    static QVector<int> detectFlat(const Eigen::MatrixXd& matData,
                                    double                 dThreshold = 1e-13);

    //=========================================================================================================
    /**
     * Detect high-variance (noisy) channels.
     *
     * Computes the per-channel standard deviation, then flags any channel whose z-score
     * (relative to the median and MAD across channels) exceeds @p dZThresh.
     * Using the median/MAD makes the test robust to a few very bad channels.
     *
     * @param[in] matData   Data matrix (n_channels × n_samples).
     * @param[in] dZThresh  Z-score threshold (default 4.0).
     *
     * @return Row indices of high-variance channels.
     */
    static QVector<int> detectHighVariance(const Eigen::MatrixXd& matData,
                                            double                 dZThresh = 4.0);

    //=========================================================================================================
    /**
     * Detect low-correlation (isolated) channels.
     *
     * For each channel, computes the mean absolute Pearson correlation with its
     * @p iNeighbours nearest channels on each side of the channel list.
     * A channel is flagged if this mean falls below @p dCorrThresh.
     *
     * This detector is most useful for sensor arrays where physically close channels
     * share common signal; it is less meaningful for widely-spaced EEG reference
     * montages.
     *
     * @param[in] matData      Data matrix (n_channels × n_samples).
     * @param[in] dCorrThresh  Minimum acceptable mean absolute correlation (default 0.4).
     * @param[in] iNeighbours  Channels on each side (default 5).
     *
     * @return Row indices of low-correlation channels.
     */
    static QVector<int> detectLowCorrelation(const Eigen::MatrixXd& matData,
                                              double                 dCorrThresh = 0.4,
                                              int                    iNeighbours = 5);

private:
    //=========================================================================================================
    /**
     * Compute the Pearson correlation coefficient between two zero-mean row vectors.
     * Returns 0 if either vector has zero norm.
     */
    static double pearsonCorr(const Eigen::RowVectorXd& a, const Eigen::RowVectorXd& b);

    //=========================================================================================================
    /**
     * Compute the median of a vector of doubles.
     */
    static double median(QVector<double> values);
};

} // namespace UTILSLIB

#endif // BAD_CHANNEL_DETECT_DSP_H
