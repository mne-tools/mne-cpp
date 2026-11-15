//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_reference.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Bipolar derivations along sEEG / ECoG leads and their montage matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_reference.h"
#include "ieeg_implantation.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<IeegBipolarPair> IeegReference::bipolarPairs(const IeegImplantation& implantation)
{
    const std::vector<IeegContact>& contacts = implantation.contacts();
    std::vector<IeegBipolarPair> pairs;
    auto add = [&](int anode, int cathode) {
        const Eigen::Vector3f& a = contacts[std::size_t(anode)].position;
        const Eigen::Vector3f& c = contacts[std::size_t(cathode)].position;
        if ((a - c).norm() > kMaxNeighbourDistance)
            return;
        pairs.push_back({anode, cathode, contacts[std::size_t(anode)].name + '-' + contacts[std::size_t(cathode)].name, (a + c) * 0.5f});
    };

    for (const IeegLead& lead : implantation.leads()) {
        const std::vector<int>& order = lead.contacts;
        const int count = static_cast<int>(order.size());
        const bool grid = lead.type == IeegLeadType::Grid && std::int64_t{lead.rows} * lead.cols == count && lead.cols > 0;
        const int cols = grid ? lead.cols : count;
        // Along each row (the whole lead for depth leads and strips)
        for (int k = 0; k + 1 < count; ++k) {
            if ((k + 1) % cols != 0)
                add(order[std::size_t(k)], order[std::size_t(k) + 1]);
        }
        // Down each column of a grid
        if (grid) {
            for (int k = 0; k + cols < count; ++k)
                add(order[std::size_t(k)], order[std::size_t(k) + std::size_t(cols)]);
        }
    }
    return pairs;
}

//=============================================================================================================

Eigen::SparseMatrix<double> IeegReference::bipolarMatrix(const std::vector<IeegBipolarPair>& pairs, int contactCount)
{
    std::vector<Eigen::Triplet<double>> entries;
    entries.reserve(pairs.size() * 2);
    for (std::size_t p = 0; p < pairs.size(); ++p) {
        entries.emplace_back(int(p), pairs[p].anode, 1.0);
        entries.emplace_back(int(p), pairs[p].cathode, -1.0);
    }
    Eigen::SparseMatrix<double> montage(Eigen::Index(pairs.size()), contactCount);
    montage.setFromTriplets(entries.begin(), entries.end());
    return montage;
}
