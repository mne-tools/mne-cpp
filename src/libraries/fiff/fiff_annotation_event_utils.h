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
 * mne::FiffAnnotation entries (onset / duration / description) and integer
 * (sample, prev, code) event triples extracted from a stim channel or
 * fed to @c mne.Epochs. These free functions translate between the two:
 * mapping annotation descriptions to event codes via a description→code
 * table, exploding annotation durations into start / stop event pairs,
 * and the reverse aggregation. They mirror
 * @c mne.events_from_annotations and @c mne.annotations_from_events in
 * MNE-Python.
 *
 * @snippet ex_fiff_api/main.cpp fiff_annotations_usage
 */

#ifndef FIFF_ANNOTATION_EVENT_UTILS_H
#define FIFF_ANNOTATION_EVENT_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_annotations.h"

#include <functional>
#include <map>
#include <string>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Converts events to annotations, like mne.annotations_from_events.
 *
 * Each event becomes a point annotation at (sample - first_sample) / sfreq, described by the label of
 * its id (the third column) or, without a label, by the id itself.
 *
 * @param[in] events          Events, one row [sample, before, id] each.
 * @param[in] sfreq           Sampling frequency in Hz.
 * @param[in] event_desc      Label of each event id.
 * @param[in] first_sample    First sample of the recording.
 *
 * @return One annotation per event; none if @p sfreq is not positive.
 */
FIFFSHARED_EXPORT FiffAnnotations annotations_from_events(const Eigen::MatrixXi& events,
                                                          double sfreq,
                                                          const std::map<int, std::string>& event_desc = {},
                                                          int first_sample = 0);

//=============================================================================================================
/**
 * Converts annotations to events, like mne.events_from_annotations.
 *
 * Each annotation becomes a row [sample, 0, id], with the id of its description in @p event_id or,
 * failing that, the description read as an integer, or 0.
 *
 * @param[in] annotations    The annotations.
 * @param[in] sfreq          Sampling frequency in Hz.
 * @param[in] event_id       Id of each description.
 * @param[in] first_sample   First sample of the recording.
 *
 * @return One event row per annotation; none if @p sfreq is not positive.
 */
FIFFSHARED_EXPORT Eigen::MatrixXi events_from_annotations(const FiffAnnotations& annotations,
                                                          double sfreq,
                                                          const std::map<std::string, int, std::less<>>& event_id = {},
                                                          int first_sample = 0);

//=============================================================================================================
/**
 * Counts annotations by description, like mne.count_annotations.
 *
 * @param[in] annotations   The annotations.
 *
 * @return The number of annotations with each description.
 */
FIFFSHARED_EXPORT std::map<std::string, int, std::less<>> count_annotations(const FiffAnnotations& annotations);

} // namespace mne

#endif // FIFF_ANNOTATION_EVENT_UTILS_H
