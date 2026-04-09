//=============================================================================================================
/**
 * @file     bids_tsv.h
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
 * @brief    BidsTsv class declaration — generic BIDS TSV file reading and writing.
 *
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
