//=============================================================================================================
/**
 * @file     bids_channel.cpp
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
 * @brief    BidsChannel struct definition — *_channels.tsv I/O.
 *
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
