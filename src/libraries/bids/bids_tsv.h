//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_tsv.h
 * @since 2026
 * @date  April 2026
 * @brief Generic tab-separated-value reader/writer for BIDS sidecars (UTF-8, LF, ``n/a`` for missing values, mandatory header row).
 *
 * BIDS standardises a strict TSV dialect for every tabular sidecar: a
 * mandatory header row defines the column names, values are separated
 * by single literal tabs, missing values are encoded as the three-byte
 * string @c n/a, and the file MUST be UTF-8 encoded with LF (no CR)
 * line terminators. @ref BidsTsv implements that dialect once via
 * @ref BidsTsv::readTsv / @ref BidsTsv::writeTsv so the domain-specific
 * wrappers @ref BidsChannel, @ref BidsElectrode and @ref BidsEvent do
 * not each re-derive it. Rows are returned as ordered
 * column-name → value maps (@ref BidsTsvRow), preserving the header
 * ordering for round-trip-stable writes.
 */

#ifndef BIDS_TSV_H
#define BIDS_TSV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * Represents a single row of a BIDS TSV file as a column-name → value map.
 */
using BidsTsvRow = QMap<QString, QString>;

//=============================================================================================================
/**
 * Generic BIDS TSV file reader/writer.
 *
 * BIDS TSV files are tab-separated value files with:
 * - A mandatory header row defining column names.
 * - "n/a" for missing values.
 * - UTF-8 encoding, LF line endings.
 *
 * Domain-specific TSV I/O (channels, electrodes, events) is handled by
 * BidsChannel, BidsElectrode, and BidsEvent respectively.
 *
 * @brief Generic BIDS TSV file I/O.
 */
class BIDSSHARED_EXPORT BidsTsv
{
public:
    using SPtr = QSharedPointer<BidsTsv>;

    BidsTsv();
    ~BidsTsv();

    /**
     * Reads a TSV file into an ordered list of row maps.
     *
     * @param[in] sFilePath  Path to the TSV file.
     * @param[out] headers   Column names from the header row.
     * @return List of rows, each row a map of column-name → value.
     */
    static QList<BidsTsvRow> readTsv(const QString& sFilePath,
                                     QStringList& headers);

    /**
     * Writes rows to a TSV file.
     *
     * @param[in] sFilePath  Output file path.
     * @param[in] headers    Column names (determines column order).
     * @param[in] rows       Data rows.
     * @return true on success, false on I/O error.
     */
    static bool writeTsv(const QString& sFilePath,
                         const QStringList& headers,
                         const QList<BidsTsvRow>& rows);
};

} // namespace BIDSLIB

#endif // BIDS_TSV_H
