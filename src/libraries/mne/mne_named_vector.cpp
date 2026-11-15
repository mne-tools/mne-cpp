//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_named_vector.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNENamedVector.
 *
 * Implements constructors and the name-based lookup helpers; FIFF I/O is
 * handled by the surrounding matrix readers because named vectors are
 * almost always written as a degenerate named matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_named_vector.h"
#include <base/log.h>

#include <algorithm>
#include <iterator>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MNENamedVector::pick(std::span<const std::string> pick_names, bool require_all, Eigen::Ref<Eigen::VectorXf> res) const
{
    if (names.empty()) {
        mne::Log::critical("No names present in vector. Cannot pick.");
        return false;
    }
    if (static_cast<std::size_t>(res.size()) != pick_names.size()) {
        mne::Log::critical("Cannot pick {} names into a vector of {} values.", pick_names.size(), res.size());
        return false;
    }

    res.setZero();
    for (std::size_t k = 0; k < pick_names.size(); ++k) {
        const auto it = std::ranges::find(names, pick_names[k]);
        if (it != names.end())
            res[static_cast<Eigen::Index>(k)] = data[std::distance(names.begin(), it)];
        else if (require_all) {
            mne::Log::critical("All required elements not found in named vector.");
            return false;
        }
    }
    return true;
}
