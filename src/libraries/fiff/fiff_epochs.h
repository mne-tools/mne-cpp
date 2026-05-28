//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_epochs.h
 * @since 2026
 * @date  May 2026
 * @brief Static epoching utilities: cut a FiffRawData stream into fixed-length, event-aligned epochs.
 *
 * Implements the equivalent of @c mne.Epochs construction from a
 * continuous recording: given a @ref FiffRawData, an event list (or a
 * fixed step), and a time window (@c tmin, @c tmax), it returns a
 * 3D (nepoch × nchan × nsamples) stack along with the associated
 * @ref FiffInfo. Bad-segment rejection (via @ref FiffAnnotation
 * ``BAD_*'' entries) and peak-to-peak / flatness rejection
 * (via @ref RejectionParams in @ref fiff_evoked_set.h) are applied as
 * the epochs are cut.
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
 * Returned by the static epoch-cutting helpers in @ref FiffEpochsUtils.
 * Mirrors the @c mne.EpochsArray construction return value in
 * MNE-Python.
 */
struct FIFFSHARED_EXPORT FiffEpochData
{
    Eigen::MatrixXd data;           /**< Epoch data (n_channels × n_times_per_epoch). */
    double tmin = 0.0;              /**< Start time of this epoch in seconds. */
    double tmax = 0.0;              /**< End time of this epoch in seconds. */
};

//=============================================================================================================
/**
 * @brief Free / static helpers that turn a @ref FiffRawData plus an event list into fixed-length epochs.
 *
 * Stateless — operates on the @ref FiffRawData and event arguments
 * directly. Used by the source-reconstruction pipeline and by the offline
 * averaging tooling when building epochs for an @ref FiffEvokedSet.
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
