//=============================================================================================================
/**
 * @file     bids_tsv.cpp
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
 * @brief    BidsTsv class definition — generic TSV I/O.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_tsv.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

static const QString NA = QStringLiteral("n/a");

static QString valOrNA(const QString& s)
{
    return s.isEmpty() ? NA : s;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BidsTsv::BidsTsv()
{
}

//=============================================================================================================

BidsTsv::~BidsTsv()
{
}

//=============================================================================================================

QList<BidsTsvRow> BidsTsv::readTsv(const QString& sFilePath,
                                    QStringList& headers)
{
    QList<BidsTsvRow> rows;
    headers.clear();

    QFile file(sFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BidsTsv::readTsv] Cannot open" << sFilePath;
        return rows;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while(!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.isEmpty())
            continue;

        QStringList fields = line.split(QLatin1Char('\t'));

        if(firstLine) {
            headers = fields;
            firstLine = false;
            continue;
        }

        BidsTsvRow row;
        for(int i = 0; i < headers.size() && i < fields.size(); ++i) {
            row[headers[i]] = fields[i];
        }
        rows.append(row);
    }

    file.close();
    return rows;
}

//=============================================================================================================

bool BidsTsv::writeTsv(const QString& sFilePath,
                        const QStringList& headers,
                        const QList<BidsTsvRow>& rows)
{
    QFile file(sFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BidsTsv::writeTsv] Cannot open" << sFilePath << "for writing";
        return false;
    }

    QTextStream out(&file);

    // Header row
    out << headers.join(QLatin1Char('\t')) << QLatin1Char('\n');

    // Data rows
    for(const auto& row : rows) {
        QStringList fields;
        fields.reserve(headers.size());
        for(const auto& header : headers) {
            fields << valOrNA(row.value(header));
        }
        out << fields.join(QLatin1Char('\t')) << QLatin1Char('\n');
    }

    file.close();
    return true;
}
