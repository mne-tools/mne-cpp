//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_explain.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref mne::FiffExplain from the compiled-in fiff_explanations.txt table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_explain.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <unordered_map>
#include <charconv>
#include <string_view>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace mne::detail
{
extern const unsigned char fiff_explanations_text[]; // generated from resources/fiff_explanations.txt
extern const std::size_t fiff_explanations_size;
} // namespace mne::detail

namespace
{

// Explanation classes of fiff_explanations.txt (MNE-C mne_show_fiff)
enum ExplanationClass
{
    TagClass = 1,
    BlockClass = 2,
    UnitClass = 3,
    UnitMultiplierClass = 4,
    ChannelKindClass = 5
};

std::uint64_t key(int exclass, int kind)
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(exclass)) << 32) | static_cast<std::uint32_t>(kind);
}

const std::unordered_map<std::uint64_t, std::string>& explanations()
{
    static const std::unordered_map<std::uint64_t, std::string> table = [] {
        std::unordered_map<std::uint64_t, std::string> entries;
        const std::string_view text(reinterpret_cast<const char*>(detail::fiff_explanations_text), detail::fiff_explanations_size);
        // <class> <kind> "<text>"; the first entry of a duplicated class/kind wins, inner spaces collapse as in MNE-C
        for (std::size_t start = 0; start < text.size();) {
            const std::size_t end = std::min(text.find('\n', start), text.size());
            const std::string_view line = text.substr(start, end - start);
            start = end + 1;
            int exclass = 0;
            int kind = 0;
            const char* at = line.data() + std::min(line.find_first_not_of(" \t\r"), line.size());
            const char* last = line.data() + line.size();
            auto [afterClass, classError] = std::from_chars(at, last, exclass);
            if (classError != std::errc() || *at == '-' || afterClass == last || (*afterClass != ' ' && *afterClass != '\t'))
                continue;
            at = afterClass;
            while (at != last && (*at == ' ' || *at == '\t'))
                ++at;
            auto [afterKind, kindError] = std::from_chars(at, last, kind);
            if (kindError != std::errc() || afterKind == last || (*afterKind != ' ' && *afterKind != '\t'))
                continue;
            at = afterKind;
            while (at != last && (*at == ' ' || *at == '\t'))
                ++at;
            const std::string_view rest(at, static_cast<std::size_t>(last - at));
            const std::size_t close = rest.find('"', 1);
            if (!rest.starts_with('"') || close == std::string_view::npos)
                continue;
            std::string explanation;
            for (const char c : rest.substr(1, close - 1)) {
                const bool space = c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
                if (!space)
                    explanation += c;
                else if (!explanation.empty() && explanation.back() != ' ')
                    explanation += ' ';
            }
            if (!explanation.empty() && explanation.back() == ' ')
                explanation.pop_back();
            entries.try_emplace(key(exclass, kind), std::move(explanation));
        }
        return entries;
    }();
    return table;
}

std::string lookup(int exclass, int kind)
{
    const auto& table = explanations();
    const auto it = table.find(key(exclass, kind));
    return it == table.end() ? std::string() : it->second;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::string FiffExplain::tag(int kind)
{
    return lookup(TagClass, kind);
}

//=============================================================================================================

std::string FiffExplain::block(int kind)
{
    return lookup(BlockClass, kind);
}

//=============================================================================================================

std::string FiffExplain::unit(int unit)
{
    return lookup(UnitClass, unit);
}

//=============================================================================================================

std::string FiffExplain::unitMultiplier(int unitMul)
{
    return lookup(UnitMultiplierClass, unitMul);
}

//=============================================================================================================

std::string FiffExplain::channelKind(int kind)
{
    return lookup(ChannelKindClass, kind);
}
