//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_epochs.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Static epoching utilities: cut continuous data into fixed-length epochs, concatenate and average them.
 *
 * Equivalent of @c mne.make_fixed_length_epochs (duration, overlap, optional short last epoch),
 * @c mne.concatenate_epochs and @c Epochs.average on plain data matrices. Event-locked epochs with
 * rejection are averaged by @ref FIFFLIB::FiffEvokedSet::computeAverages.
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
 * @brief Fixed-length epoching result: the (nepoch × nchan × nsamples) data stack plus the matching @ref FiffInfo.
 *
 * Returned by the static epoch-cutting helpers of @ref FiffEpochs.
 * Mirrors the @c mne.EpochsArray construction return value in
 * MNE-Python.
 */
struct FIFFSHARED_EXPORT FiffEpochData
{
    Eigen::MatrixXd data; /**< Epoch data (n_channels × n_times_per_epoch). */
    double tmin = 0.0;    /**< Start time of this epoch in seconds. */
    double tmax = 0.0;    /**< End time of this epoch in seconds. */
};

//=============================================================================================================
/**
 * @brief Static helpers that cut continuous data into fixed-length epochs, concatenate and average them.
 *
 * Stateless; operates on data matrices. Counterpart of @c mne.make_fixed_length_epochs and
 * @c mne.concatenate_epochs in MNE-Python.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_epochs_usage
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
