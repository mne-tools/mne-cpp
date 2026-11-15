//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_tsv.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BidsTsv — generic UTF-8 / LF / @c n/a TSV reader and writer for BIDS sidecars.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_tsv.h"
#include <base/log.h>

#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <fstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<BidsTsvRow> BidsTsv::read_tsv(const std::filesystem::path& path,
                                          std::vector<std::string>& headers)
{
    std::vector<BidsTsvRow> rows;
    headers.clear();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        BASELIB::Log::warning("[BidsTsv::read_tsv] Cannot open {}", BASELIB::utf8FromPath(path));
        return rows;
    }

    bool firstLine = true;
    std::string line;
    while (std::getline(file, line)) {
        const std::string_view text = BASELIB::trimmed(line);
        if (text.empty())
            continue;

        BASELIB::StringList fields = BASELIB::split(text, '\t');

        if (firstLine) {
            headers = std::move(fields);
            firstLine = false;
            continue;
        }

        BidsTsvRow row;
        for (std::size_t i = 0; i < headers.size() && i < fields.size(); ++i) {
            row[headers[i]] = std::move(fields[i]);
        }
        rows.push_back(std::move(row));
    }

    return rows;
}

//=============================================================================================================

bool BidsTsv::write_tsv(const std::filesystem::path& path,
                        std::span<const std::string> headers,
                        std::span<const BidsTsvRow> rows)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        BASELIB::Log::warning("[BidsTsv::write_tsv] Cannot open {} for writing", BASELIB::utf8FromPath(path));
        return false;
    }

    for (std::size_t i = 0; i < headers.size(); ++i) {
        out << (i > 0 ? "\t" : "") << headers[i];
    }
    out << '\n';

    for (const BidsTsvRow& row : rows) {
        for (std::size_t i = 0; i < headers.size(); ++i) {
            const std::string_view field = value(row, headers[i]);
            out << (i > 0 ? "\t" : "") << (field.empty() ? std::string_view("n/a") : field);
        }
        out << '\n';
    }

    return static_cast<bool>(out);
}

//=============================================================================================================

std::string_view BidsTsv::value(const BidsTsvRow& row, std::string_view column, std::string_view fallback)
{
    const auto it = row.find(column);
    return it == row.end() ? fallback : std::string_view(it->second);
}
