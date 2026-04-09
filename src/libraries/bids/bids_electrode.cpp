//=============================================================================================================
/**
 * @file     bids_electrode.cpp
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
 * @brief    BidsElectrode struct definition — *_electrodes.tsv I/O.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_electrode.h"
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

QList<BidsElectrode> BidsElectrode::readTsv(const QString& sFilePath)
{
    QStringList headers;
    QList<BidsTsvRow> rawRows = BidsTsv::readTsv(sFilePath, headers);

    QList<BidsElectrode> electrodes;
    electrodes.reserve(rawRows.size());

    for(const auto& row : rawRows) {
        BidsElectrode elec;
        elec.name       = row.value(QStringLiteral("name"));
        elec.x          = row.value(QStringLiteral("x"), NA);
        elec.y          = row.value(QStringLiteral("y"), NA);
        elec.z          = row.value(QStringLiteral("z"), NA);
        elec.size       = naToEmpty(row.value(QStringLiteral("size")));
        elec.type       = naToEmpty(row.value(QStringLiteral("type")));
        elec.material   = naToEmpty(row.value(QStringLiteral("material")));
        elec.impedance  = naToEmpty(row.value(QStringLiteral("impedance")));
        electrodes.append(elec);
    }

    return electrodes;
}

//=============================================================================================================

bool BidsElectrode::writeTsv(const QString& sFilePath,
                             const QList<BidsElectrode>& electrodes)
{
    QStringList headers = {
        QStringLiteral("name"),
        QStringLiteral("x"),
        QStringLiteral("y"),
        QStringLiteral("z"),
        QStringLiteral("size"),
        QStringLiteral("type"),
        QStringLiteral("material"),
        QStringLiteral("impedance"),
    };

    QList<BidsTsvRow> rows;
    rows.reserve(electrodes.size());

    for(const auto& elec : electrodes) {
        BidsTsvRow row;
        row[QStringLiteral("name")]      = elec.name;
        row[QStringLiteral("x")]         = elec.x;
        row[QStringLiteral("y")]         = elec.y;
        row[QStringLiteral("z")]         = elec.z;
        row[QStringLiteral("size")]      = elec.size;
        row[QStringLiteral("type")]      = elec.type;
        row[QStringLiteral("material")]  = elec.material;
        row[QStringLiteral("impedance")] = elec.impedance;
        rows.append(row);
    }

    return BidsTsv::writeTsv(sFilePath, headers, rows);
}
