//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_ch_selection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of @ref mne::MNEChSelection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ch_selection.h"

#include "mne_deriv.h"
#include "mne_sparse_named_matrix.h"
#include "mne_types.h"
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEChSelection MNEChSelection::these(std::string selname, std::span<const std::string> names)
{
    MNEChSelection sel;
    sel.name = std::move(selname);
    sel.chdef.assign(names.begin(), names.end());
    sel.kind = MNE_CH_SELECTION_USER;
    return sel;
}

//=============================================================================================================

int MNEChSelection::assign_channels(std::span<const FiffChInfo> chs, const MNEDeriv* deriv)
{
    chspick = chdef;
    chspick_nospace = chdef;
    for (auto& channel : chspick_nospace)
        std::erase(channel, ' ');
    const int nchan = this->nchan();

    pick.setConstant(nchan, -1);
    pick_deriv.setConstant(nchan, -1);
    ch_kind.setConstant(nchan, -1);

    const auto matches = [this](int c, std::string_view ch_name) {
        return mne::iequals(chspick[c], ch_name) || mne::iequals(chspick_nospace[c], ch_name);
    };
    const auto assign = [this, &chs](int c, std::size_t rc) {
        pick[c] = static_cast<int>(rc);
        ch_kind[c] = chs[rc].kind;
    };

    for (int c = 0; c < nchan; c++) {
        for (std::size_t rc = 0; rc < chs.size(); rc++) {
            if (matches(c, chs[rc].ch_name)) {
                assign(c, rc);
                break;
            }
        }
    }
    /*
     * Maybe the derivations will help
     */
    if (deriv && deriv->deriv_data) {
        const StringList& deriv_names = deriv->deriv_data->rowlist;
        for (int c = 0; c < nchan; c++) {
            if (pick[c] != -1)
                continue;
            for (std::size_t d = 0; d < deriv_names.size(); d++) {
                if (mne::iequals(chspick[c], deriv_names[d]) && d < static_cast<std::size_t>(deriv->valid.size()) && deriv->valid[static_cast<Eigen::Index>(d)]) {
                    pick_deriv[c] = static_cast<int>(d);
                    ch_kind[c] = deriv->chs[d].kind;
                    break;
                }
            }
        }
    }
    /*
     * Try simple channels again without the part after dashes
     */
    for (int c = 0; c < nchan; c++) {
        if (pick[c] != -1 || pick_deriv[c] != -1)
            continue;
        for (std::size_t rc = 0; rc < chs.size(); rc++) {
            const std::string_view ch_name = chs[rc].ch_name;
            const auto dash = ch_name.find('-');
            if (dash != std::string_view::npos && matches(c, ch_name.substr(0, dash))) {
                assign(c, rc);
                break;
            }
        }
    }
    if (const int matched = nderiv(); matched > 0)
        mne::Log::info("Selection \"{}\" has {} matched derived channels.", name, matched);
    return static_cast<int>(std::ranges::count_if(pick, [](int p) { return p >= 0; }));
}
