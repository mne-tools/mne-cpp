//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_tsv.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BidsTsv — generic UTF-8 / LF / @c n/a TSV reader and writer for BIDS sidecars.
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
