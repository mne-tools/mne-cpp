//=============================================================================================================
/**
 * @file     bids_event.cpp
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
 * @brief    BidsEvent struct definition — *_events.tsv I/O.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_event.h"
#include "bids_tsv.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

QList<BidsEvent> BidsEvent::readTsv(const QString& sFilePath)
{
    QStringList headers;
    QList<BidsTsvRow> rawRows = BidsTsv::readTsv(sFilePath, headers);

    bool hasOnset     = headers.contains(QStringLiteral("onset"));
    bool hasDuration  = headers.contains(QStringLiteral("duration"));
    bool hasSample    = headers.contains(QStringLiteral("sample"));
    bool hasValue     = headers.contains(QStringLiteral("value"));
    bool hasTrialType = headers.contains(QStringLiteral("trial_type"));

    QList<BidsEvent> events;
    events.reserve(rawRows.size());

    for(const auto& row : rawRows) {
        BidsEvent ev;

        if(hasOnset)
            ev.onset = row.value(QStringLiteral("onset"), QStringLiteral("0")).toFloat();

        if(hasDuration) {
            QString dur = row.value(QStringLiteral("duration"), QStringLiteral("0"));
            ev.duration = (dur == QStringLiteral("n/a")) ? 0.0f : dur.toFloat();
        }

        if(hasSample)
            ev.sample = row.value(QStringLiteral("sample"), QStringLiteral("0")).toInt();

        if(hasValue) {
            QString val = row.value(QStringLiteral("value"), QStringLiteral("0"));
            ev.value = (val == QStringLiteral("n/a")) ? 0 : val.toInt();
        }

        if(hasTrialType) {
            ev.trialType = row.value(QStringLiteral("trial_type"));
            if(ev.trialType == QStringLiteral("n/a"))
                ev.trialType.clear();
        }

        events.append(ev);
    }

    return events;
}

//=============================================================================================================

bool BidsEvent::writeTsv(const QString& sFilePath,
                         const QList<BidsEvent>& events)
{
    QStringList headers = {
        QStringLiteral("onset"),
        QStringLiteral("duration"),
        QStringLiteral("trial_type"),
        QStringLiteral("value"),
        QStringLiteral("sample"),
    };

    QList<BidsTsvRow> rows;
    rows.reserve(events.size());

    for(const auto& ev : events) {
        BidsTsvRow row;
        row[QStringLiteral("onset")]      = QString::number(static_cast<double>(ev.onset), 'f', 6);
        row[QStringLiteral("duration")]   = QString::number(static_cast<double>(ev.duration), 'f', 6);
        row[QStringLiteral("trial_type")] = ev.trialType.isEmpty() ? QStringLiteral("n/a") : ev.trialType;
        row[QStringLiteral("value")]      = QString::number(ev.value);
        row[QStringLiteral("sample")]     = QString::number(ev.sample);
        rows.append(row);
    }

    return BidsTsv::writeTsv(sFilePath, headers, rows);
}
