//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_tsv.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Generic tab-separated-value reader/writer for BIDS sidecars (UTF-8, LF, ``n/a`` for missing values, mandatory header row).
 *
 * BIDS standardises a strict TSV dialect for every tabular sidecar: a
 * mandatory header row defines the column names, values are separated
 * by single literal tabs, missing values are encoded as the three-byte
 * string @c n/a, and the file MUST be UTF-8 encoded with LF (no CR)
 * line terminators. @ref mne::BidsTsv implements that dialect once via
 * @ref mne::BidsTsv::read_tsv "BidsTsv::read_tsv" / @ref mne::BidsTsv::write_tsv "BidsTsv::write_tsv" so the domain-specific
 * wrappers @ref mne::BidsChannel, @ref mne::BidsElectrode and @ref mne::BidsEvent do
 * not each re-derive it. Rows are returned as ordered
 * column-name → value maps (@ref mne::BidsTsvRow "BidsTsvRow"), preserving the header
 * ordering for round-trip-stable writes.
 */

#ifndef BIDS_TSV_H
#define BIDS_TSV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Represents a single row of a BIDS TSV file as a column-name → value map.
 */
using BidsTsvRow = std::map<std::string, std::string, std::less<>>;

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
 *
 * @snippet ex_bids/main.cpp bids_tsv_read
 */
class BIDSSHARED_EXPORT BidsTsv
{
public:
    /**
     * Reads a TSV file into an ordered list of row maps.
     *
     * @param[in] path      Path to the TSV file.
     * @param[out] headers  Column names from the header row.
     * @return List of rows, each row a map of column-name → value; empty if the file cannot be read.
     */
    static std::vector<BidsTsvRow> read_tsv(const std::filesystem::path& path,
                                            std::vector<std::string>& headers);

    /**
     * Writes rows to a TSV file, writing "n/a" for empty and missing values.
     *
     * @param[in] path     Output file path.
     * @param[in] headers  Column names (determines column order).
     * @param[in] rows     Data rows.
     * @return true on success, false on I/O error.
     */
    static bool write_tsv(const std::filesystem::path& path,
                          std::span<const std::string> headers,
                          std::span<const BidsTsvRow> rows);

    /**
     * @param[in] row        A row.
     * @param[in] column     Column name.
     * @param[in] fallback   Value if the row has no such column.
     * @return The value of @p column, valid while @p row is unchanged.
     */
    static std::string_view value(const BidsTsvRow& row, std::string_view column, std::string_view fallback = {});
};

} // namespace mne

#endif // BIDS_TSV_H
