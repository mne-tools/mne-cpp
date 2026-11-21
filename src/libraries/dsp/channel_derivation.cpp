//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_derivation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    ChannelDerivation class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channel_derivation.h"

#include <base/log.h>
#include <mne/mne_deriv_set.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <utility>
#include <cmath>
#include <format>
#include <fstream>
#include <string_view>
#include <unordered_map>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

/**
 * @brief Extract the shaft prefix from a channel name.
 *
 * Collects all leading characters before the first digit.
 * E.g. "LH1" → "LH", "RA12" → "RA", "A'1" → "A'".
 */
static std::string_view extractShaftPrefix(std::string_view name)
{
    const std::size_t digit = name.find_first_of("0123456789");
    return name.substr(0, digit);
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<DerivationRule> ChannelDerivation::buildBipolar(const StringList& channelNames)
{
    // Group channels by shaft prefix, keeping the first-seen group order and the channel order within groups
    std::vector<std::vector<const std::string*>> groups;
    std::unordered_map<std::string_view, std::size_t> groupIndex;
    for (const std::string& name : channelNames) {
        const auto [it, inserted] = groupIndex.try_emplace(extractShaftPrefix(name), groups.size());
        if (inserted) {
            groups.emplace_back();
        }
        groups[it->second].push_back(&name);
    }

    // Build bipolar pairs within each group
    std::vector<DerivationRule> rules;
    for (const auto& group : groups) {
        for (std::size_t i = 0; i + 1 < group.size(); ++i) {
            const std::string& a = *group[i];
            const std::string& b = *group[i + 1];
            rules.push_back({std::format("{}-{}", a, b), {{a, 1.0}, {b, -1.0}}});
        }
    }

    return rules;
}

//=============================================================================================================

std::vector<DerivationRule> ChannelDerivation::buildCommonAverage(const StringList& channelNames)
{
    if (channelNames.empty()) {
        return {};
    }

    const double invN = 1.0 / static_cast<double>(channelNames.size());

    std::map<std::string, double> average;
    for (const std::string& ch : channelNames) {
        average.emplace(ch, -invN);
    }

    std::vector<DerivationRule> rules;
    rules.reserve(channelNames.size());
    for (const std::string& target : channelNames) {
        DerivationRule rule{target, average};
        // Override target channel: weight = 1.0 - 1/N
        rule.inputWeights[target] = 1.0 - invN;
        rules.push_back(std::move(rule));
    }

    return rules;
}

//=============================================================================================================

std::pair<MatrixXd, StringList> ChannelDerivation::apply(
    const MatrixXd& matData,
    const StringList& channelNames,
    const std::vector<DerivationRule>& rules)
{
    // Build channel name → row index lookup
    std::unordered_map<std::string_view, Eigen::Index> chIndex;
    chIndex.reserve(channelNames.size());
    for (std::size_t i = 0; i < channelNames.size(); ++i) {
        chIndex.emplace(channelNames[i], static_cast<Eigen::Index>(i));
    }

    MatrixXd matResult = MatrixXd::Zero(static_cast<Eigen::Index>(rules.size()), matData.cols());
    StringList outputNames;
    outputNames.reserve(rules.size());

    for (std::size_t r = 0; r < rules.size(); ++r) {
        const DerivationRule& rule = rules[r];
        outputNames.push_back(rule.outputName);

        for (const auto& [input, weight] : rule.inputWeights) {
            const auto idxIt = chIndex.find(input);
            if (idxIt == chIndex.end()) {
                Log::warning("ChannelDerivation::apply - channel not found: {} in rule: {}", input, rule.outputName);
                continue;
            }
            matResult.row(static_cast<Eigen::Index>(r)) += weight * matData.row(idxIt->second);
        }
    }

    return {std::move(matResult), std::move(outputNames)};
}

//=============================================================================================================

std::vector<DerivationRule> ChannelDerivation::readDefinitionFile(const std::filesystem::path& path)
{
    const std::optional<MNEDerivSet> set = MNEDerivSet::readText(path);
    if (!set) {
        return {};
    }
    std::vector<DerivationRule> rules;
    for (MNEDerivSet::Definition& definition : set->definitions()) {
        rules.push_back({std::move(definition.first), std::move(definition.second)});
    }
    return rules;
}

//=============================================================================================================

bool ChannelDerivation::writeDefinitionFile(const std::filesystem::path& path, const std::vector<DerivationRule>& rules)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        Log::warning("ChannelDerivation::writeDefinitionFile - cannot open: {}", utf8_from_path(path));
        return false;
    }
    // MNE-C mne_make_derivations syntax; names are quoted because they may contain spaces.
    out << "# Channel derivations: \"name\" = weight * \"input\" + ...\n";
    for (const DerivationRule& rule : rules) {
        out << '"' << rule.outputName << "\" =";
        for (const auto& [input, weight] : rule.inputWeights) {
            out << std::format(" {} {:.17g} * \"{}\"", weight < 0.0 ? '-' : '+', std::fabs(weight), input);
        }
        out << '\n';
    }
    return static_cast<bool>(out);
}
