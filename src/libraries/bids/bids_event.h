//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Reader/writer for the BIDS ``_events.tsv`` sidecar — discrete annotations on the time axis of a recording.
 *
 * BIDS events live in a @c _events.tsv sibling of every electrophysiology
 * or functional acquisition with REQUIRED columns @c onset and
 * @c duration (seconds) and a recommended @c trial_type label;
 * mne-cpp additionally tracks the original 0-based @c sample index and
 * the numeric trigger @c value so the record can round-trip back into
 * FIFF @c stim-channel land without re-deriving them. @ref BidsEvent is
 * the per-row value object and the static @ref BidsEvent::readTsv /
 * @ref BidsEvent::writeTsv functions perform the TSV round-trip via
 * @ref BidsTsv. Missing / @c n/a fields default to zero / empty so
 * partially-populated event files still round-trip.
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/modality-agnostic-files.html#tasks
 */

#ifndef BIDS_EVENT_H
#define BIDS_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Single event annotation, typically parsed from or written to *_events.tsv.
 *
 * Each event represents a discrete occurrence during the recording, such as a
 * stimulus presentation, button press, or seizure onset.
 */
struct BIDSSHARED_EXPORT BidsEvent
{
    float   onset{0.0f};        /**< Onset in seconds from the start of the recording. */
    float   duration{0.0f};     /**< Duration of the event in seconds. */
    int     sample{0};          /**< Sample index (0-based) corresponding to onset. */
    int     value{0};           /**< Numeric event value / trigger code. */
    QString trialType;          /**< Trial type or condition label (e.g. "stimulus", "response"). */

    /**
     * @brief Read a BIDS *_events.tsv file.
     *
     * Parses onset, duration, sample, value, and trial_type columns.
     * Missing or "n/a" fields default to zero / empty.
     *
     * @param[in] sFilePath  Path to the events.tsv file.
     * @return List of events.
     */
    static QList<BidsEvent> readTsv(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_events.tsv file.
     *
     * @param[in] sFilePath  Output path.
     * @param[in] events     List of events.
     * @return true on success.
     */
    static bool writeTsv(const QString& sFilePath,
                         const QList<BidsEvent>& events);
};

} // namespace BIDSLIB

#endif // BIDS_EVENT_H
