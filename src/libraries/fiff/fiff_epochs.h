//=============================================================================================================
/**
 * @file     fiff_epochs.h
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
 * @brief    Epoch convenience utilities.
 *
 * Equivalent to MNE-Python's mne.make_fixed_length_epochs and mne.concatenate_epochs.
 */

#ifndef FIFF_EPOCHS_H
#define FIFF_EPOCHS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_evoked.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QPair>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Result structure for fixed-length epoching.
 */
struct FIFFSHARED_EXPORT FiffEpochData
{
    Eigen::MatrixXd data;           /**< Epoch data (n_channels × n_times_per_epoch). */
    double tmin = 0.0;              /**< Start time of this epoch in seconds. */
    double tmax = 0.0;              /**< End time of this epoch in seconds. */
};

//=============================================================================================================
/**
 * @brief Static utilities for epoch creation and manipulation.
 *
 * Provides:
 * - makeFixedLengthEpochs: Segment continuous data into fixed-length epochs
 * - concatenateEpochs: Concatenate multiple epoch sets into one
 * - averageEpochs: Compute average across epochs
 */
class FIFFSHARED_EXPORT FiffEpochs
{
public:
    //=========================================================================================================
    /**
     * @brief Create fixed-length epochs from continuous data.
     *
     * Segments the data matrix into non-overlapping (or overlapping) epochs
     * of the specified duration.
     *
     * @param[in] matData       Continuous data (n_channels × n_times).
     * @param[in] dSFreq        Sampling frequency in Hz.
     * @param[in] dDuration     Epoch duration in seconds.
     * @param[in] dOverlap      Overlap between epochs in seconds (default 0.0).
     * @param[in] bDropLast     Drop the last epoch if it's shorter than duration (default true).
     *
     * @return List of epoch data structures.
     */
    static QList<FiffEpochData> makeFixedLengthEpochs(const Eigen::MatrixXd& matData,
                                                        double dSFreq,
                                                        double dDuration,
                                                        double dOverlap = 0.0,
                                                        bool bDropLast = true);

    //=========================================================================================================
    /**
     * @brief Concatenate multiple epoch sets into a single list.
     *
     * @param[in] epochSets     List of epoch sets to concatenate.
     *
     * @return Combined list of all epochs.
     */
    static QList<FiffEpochData> concatenateEpochs(const QList<QList<FiffEpochData>>& epochSets);

    //=========================================================================================================
    /**
     * @brief Compute the average (evoked response) across epochs.
     *
     * All epochs must have the same dimensions.
     * Returns a FiffEvoked with data, nave, times, and aspect_kind populated.
     *
     * @param[in] epochs        List of epochs.
     * @param[in] dSFreq        Sampling frequency in Hz (used to compute times vector).
     * @param[in] comment       Comment string for the evoked (default "Average").
     *
     * @return FiffEvoked with averaged data and metadata.
     */
    static FiffEvoked averageEpochs(const QList<FiffEpochData>& epochs,
                                    double dSFreq,
                                    const QString& comment = "Average");

    //=========================================================================================================
    /**
     * @brief Extract data matrices from epoch structures.
     *
     * Convenience method to get a list of data matrices from epoch structures,
     * suitable for use with CSP, SPoC, SSD, etc.
     *
     * @param[in] epochs        List of epoch data.
     *
     * @return List of data matrices (n_channels × n_times).
     */
    static QList<Eigen::MatrixXd> toMatrixList(const QList<FiffEpochData>& epochs);
};

} // namespace FIFFLIB

#endif // FIFF_EPOCHS_H
