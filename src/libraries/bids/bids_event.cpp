//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_event.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::BidsEvent — TSV I/O for the BIDS ``_events.tsv`` annotation sidecar.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_event.h"
#include "bids_tsv.h"

#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

std::vector<BidsEvent> BidsEvent::read_tsv(const std::filesystem::path& path)
{
    std::vector<std::string> headers;
    const std::vector<BidsTsvRow> rawRows = BidsTsv::read_tsv(path, headers);

    const bool hasOnset = mne::contains(headers, "onset");
    const bool hasDuration = mne::contains(headers, "duration");
    const bool hasSample = mne::contains(headers, "sample");
    const bool hasValue = mne::contains(headers, "value");
    const bool hasTrialType = mne::contains(headers, "trial_type");

    std::vector<BidsEvent> events;
    events.reserve(rawRows.size());

    for (const BidsTsvRow& row : rawRows) {
        BidsEvent ev;

        // "n/a" and other non-numbers read as 0
        if (hasOnset)
            ev.onset = static_cast<float>(mne::toDouble(BidsTsv::value(row, "onset", "0")).value_or(0.0));

        if (hasDuration)
            ev.duration = static_cast<float>(mne::toDouble(BidsTsv::value(row, "duration", "0")).value_or(0.0));

        if (hasSample)
            ev.sample = mne::toInt(BidsTsv::value(row, "sample", "0")).value_or(0);

        if (hasValue)
            ev.value = mne::toInt(BidsTsv::value(row, "value", "0")).value_or(0);

        if (hasTrialType) {
            const std::string_view trialType = BidsTsv::value(row, "trial_type");
            if (trialType != "n/a")
                ev.trialType = trialType;
        }

        events.push_back(std::move(ev));
    }

    return events;
}

//=============================================================================================================

bool BidsEvent::write_tsv(const std::filesystem::path& path,
                          std::span<const BidsEvent> events)
{
    const std::vector<std::string> headers{"onset", "duration", "trial_type", "value", "sample"};

    std::vector<BidsTsvRow> rows;
    rows.reserve(events.size());

    for (const BidsEvent& ev : events) {
        rows.push_back({{"onset", std::format("{:.6f}", static_cast<double>(ev.onset))},
                        {"duration", std::format("{:.6f}", static_cast<double>(ev.duration))},
                        {"trial_type", ev.trialType.empty() ? std::string("n/a") : ev.trialType},
                        {"value", std::to_string(ev.value)},
                        {"sample", std::to_string(ev.sample)}});
    }

    return BidsTsv::write_tsv(path, headers, rows);
}
