//=============================================================================================================
/**
 * @file     bids_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 * @brief    BidsEvent struct declaration — a single BIDS event annotation.
 *
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
