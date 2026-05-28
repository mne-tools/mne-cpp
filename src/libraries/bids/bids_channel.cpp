//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_channel.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BidsChannel — TSV read/write for the BIDS ``_channels.tsv`` sidecar.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_channel.h"
#include "bids_tsv.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{
static const QString NA = QStringLiteral("n/a");

QString naToEmpty(const QString& s)
{
    return (s == NA) ? QString() : s;
}
} // anonymous namespace

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

QList<BidsChannel> BidsChannel::readTsv(const QString& sFilePath)
{
    QStringList headers;
    QList<BidsTsvRow> rawRows = BidsTsv::readTsv(sFilePath, headers);

    QList<BidsChannel> channels;
    channels.reserve(rawRows.size());

    for(const auto& row : rawRows) {
        BidsChannel ch;
        ch.name         = row.value(QStringLiteral("name"));
        ch.type         = row.value(QStringLiteral("type"));
        ch.units        = row.value(QStringLiteral("units"));
        ch.samplingFreq = naToEmpty(row.value(QStringLiteral("sampling_frequency")));
        ch.lowCutoff    = naToEmpty(row.value(QStringLiteral("low_cutoff")));
        ch.highCutoff   = naToEmpty(row.value(QStringLiteral("high_cutoff")));
        ch.notch        = naToEmpty(row.value(QStringLiteral("notch")));
        ch.status       = naToEmpty(row.value(QStringLiteral("status")));
        ch.description  = naToEmpty(row.value(QStringLiteral("description")));
        channels.append(ch);
    }

    return channels;
}

//=============================================================================================================

bool BidsChannel::writeTsv(const QString& sFilePath,
                           const QList<BidsChannel>& channels)
{
    QStringList headers = {
        QStringLiteral("name"),
        QStringLiteral("type"),
        QStringLiteral("units"),
        QStringLiteral("sampling_frequency"),
        QStringLiteral("low_cutoff"),
        QStringLiteral("high_cutoff"),
        QStringLiteral("notch"),
        QStringLiteral("status"),
        QStringLiteral("description"),
    };

    QList<BidsTsvRow> rows;
    rows.reserve(channels.size());

    for(const auto& ch : channels) {
        BidsTsvRow row;
        row[QStringLiteral("name")]               = ch.name;
        row[QStringLiteral("type")]                = ch.type;
        row[QStringLiteral("units")]               = ch.units;
        row[QStringLiteral("sampling_frequency")]  = ch.samplingFreq;
        row[QStringLiteral("low_cutoff")]          = ch.lowCutoff;
        row[QStringLiteral("high_cutoff")]         = ch.highCutoff;
        row[QStringLiteral("notch")]               = ch.notch;
        row[QStringLiteral("status")]              = ch.status;
        row[QStringLiteral("description")]         = ch.description;
        rows.append(row);
    }

    return BidsTsv::writeTsv(sFilePath, headers, rows);
}
