//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_annotation_event_utils.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Conversion helpers between FIFF annotations and the integer stim-channel event list used by epoching / averaging.
 *
 * Two representations coexist in MNE workflows: free-text @ref
 * FiffAnnotation entries (onset / duration / description) and integer
 * (sample, prev, code) event triples extracted from a stim channel or
 * fed to @c mne.Epochs. These free functions translate between the two:
 * mapping annotation descriptions to event codes via a description→code
 * table, exploding annotation durations into start / stop event pairs,
 * and the reverse aggregation. They mirror
 * @c mne.events_from_annotations and @c mne.annotations_from_events in
 * MNE-Python.
 */

#ifndef FIFF_ANNOTATION_EVENT_UTILS_H
#define FIFF_ANNOTATION_EVENT_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_annotations.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Convert a FIFF event matrix to FiffAnnotations.
 *
 * Each event becomes an annotation with onset = (sample - firstSample) / sfreq
 * and duration = 0.  The description is looked up from eventDescriptions
 * (event_id -> label).  If not found, the event id is used as a string.
 *
 * @param[in] events              FIFF events (Nx3: [sample, before, after]).
 * @param[in] sfreq               Sampling frequency in Hz.
 * @param[in] eventDescriptions   Map from event id (the "after" column) to description string.
 * @param[in] firstSample         First sample offset (default 0).
 * @return FiffAnnotations containing one annotation per event.
 */
FIFFSHARED_EXPORT FiffAnnotations annotationsFromEvents(
    const Eigen::MatrixXi& events,
    double sfreq,
    const QMap<int, QString>& eventDescriptions = QMap<int, QString>(),
    int firstSample = 0);

//=============================================================================================================
/**
 * Convert FiffAnnotations to a FIFF event matrix.
 *
 * Each annotation becomes an event row [sample, 0, event_id].
 * The event_id is looked up from eventIds (description -> id).
 * If not found, the description is parsed as an integer; if that also
 * fails, event_id is set to 0.
 *
 * @param[in] annotations   The annotations to convert.
 * @param[in] sfreq         Sampling frequency in Hz.
 * @param[in] eventIds      Map from description string to event id integer.
 * @param[in] firstSample   First sample offset (default 0).
 * @return Eigen::MatrixXi  Nx3 event matrix [sample, 0, event_id].
 */
FIFFSHARED_EXPORT Eigen::MatrixXi eventsFromAnnotations(
    const FiffAnnotations& annotations,
    double sfreq,
    const QMap<QString, int>& eventIds = QMap<QString, int>(),
    int firstSample = 0);

//=============================================================================================================
/**
 * Count annotations by description.
 *
 * @param[in] annotations   The annotations to count.
 * @return QMap<QString, int>  Map from description to count.
 */
FIFFSHARED_EXPORT QMap<QString, int> countAnnotations(const FiffAnnotations& annotations);

} // namespace FIFFLIB

#endif // FIFF_ANNOTATION_EVENT_UTILS_H
