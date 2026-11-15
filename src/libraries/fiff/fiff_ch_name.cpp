//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_ch_name.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Matching of channel names across naming conventions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_ch_name.h"

#include <algorithm>
#include <cctype>

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

char folded(char c) noexcept
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

bool mne::same_channel(std::string_view a, std::string_view b) noexcept
{
    std::size_t i = 0;
    std::size_t j = 0;
    for (;;) {
        while (i < a.size() && a[i] == ' ') {
            ++i;
        }
        while (j < b.size() && b[j] == ' ') {
            ++j;
        }
        if (i == a.size() || j == b.size()) {
            return i == a.size() && j == b.size();
        }
        if (folded(a[i++]) != folded(b[j++])) {
            return false;
        }
    }
}

//=============================================================================================================

int mne::channel_index(const mne::StringList& names, std::string_view name) noexcept
{
    auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end()) {
        it = std::find_if(names.begin(), names.end(), [name](const std::string& n) { return same_channel(n, name); });
    }
    return it == names.end() ? -1 : static_cast<int>(it - names.begin());
}
