//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_event.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BidsEvent — TSV I/O for the BIDS ``_events.tsv`` annotation sidecar.
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
