//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_channel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::BidsChannel — TSV read/write for the BIDS ``_channels.tsv`` sidecar.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_channel.h"
#include "bids_tsv.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{

std::string naToEmpty(std::string_view s)
{
    return s == "n/a" ? std::string() : std::string(s);
}

} // anonymous namespace

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

std::vector<BidsChannel> BidsChannel::read_tsv(const std::filesystem::path& path)
{
    std::vector<std::string> headers;
    const std::vector<BidsTsvRow> rawRows = BidsTsv::read_tsv(path, headers);

    std::vector<BidsChannel> channels;
    channels.reserve(rawRows.size());

    for (const BidsTsvRow& row : rawRows) {
        BidsChannel ch;
        ch.name = BidsTsv::value(row, "name");
        ch.type = BidsTsv::value(row, "type");
        ch.units = BidsTsv::value(row, "units");
        ch.samplingFreq = naToEmpty(BidsTsv::value(row, "sampling_frequency"));
        ch.lowCutoff = naToEmpty(BidsTsv::value(row, "low_cutoff"));
        ch.highCutoff = naToEmpty(BidsTsv::value(row, "high_cutoff"));
        ch.notch = naToEmpty(BidsTsv::value(row, "notch"));
        ch.status = naToEmpty(BidsTsv::value(row, "status"));
        ch.description = naToEmpty(BidsTsv::value(row, "description"));
        channels.push_back(std::move(ch));
    }

    return channels;
}

//=============================================================================================================

bool BidsChannel::write_tsv(const std::filesystem::path& path,
                            std::span<const BidsChannel> channels)
{
    const std::vector<std::string> headers{"name", "type", "units", "sampling_frequency", "low_cutoff",
                                           "high_cutoff", "notch", "status", "description"};

    std::vector<BidsTsvRow> rows;
    rows.reserve(channels.size());

    for (const BidsChannel& ch : channels) {
        rows.push_back({{"name", ch.name},
                        {"type", ch.type},
                        {"units", ch.units},
                        {"sampling_frequency", ch.samplingFreq},
                        {"low_cutoff", ch.lowCutoff},
                        {"high_cutoff", ch.highCutoff},
                        {"notch", ch.notch},
                        {"status", ch.status},
                        {"description", ch.description}});
    }

    return BidsTsv::write_tsv(path, headers, rows);
}
