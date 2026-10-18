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

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IEEGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QVector<IeegBipolarPair> IeegReference::bipolarPairs(const IeegImplantation& implantation)
{
    const QVector<IeegContact>& contacts = implantation.contacts();
    QVector<IeegBipolarPair> pairs;
    auto add = [&](int anode, int cathode) {
        const Eigen::Vector3f& a = contacts[anode].position;
        const Eigen::Vector3f& c = contacts[cathode].position;
        if ((a - c).norm() > kMaxNeighbourDistance)
            return;
        pairs.append({anode, cathode, contacts[anode].name + QLatin1Char('-') + contacts[cathode].name, (a + c) * 0.5f});
    };

    for (const IeegLead& lead : implantation.leads()) {
        const QVector<int>& order = lead.contacts;
        const bool grid = lead.type == IeegLeadType::Grid && lead.rows * lead.cols == order.size() && lead.cols > 0;
        const int cols = grid ? lead.cols : int(order.size());
        // Along each row (the whole lead for depth leads and strips)
        for (int k = 0; k + 1 < order.size(); ++k) {
            if ((k + 1) % cols != 0)
                add(order[k], order[k + 1]);
        }
        // Down each column of a grid
        if (grid) {
            for (int k = 0; k + cols < order.size(); ++k)
                add(order[k], order[k + cols]);
        }
    }
    return pairs;
}

//=============================================================================================================

Eigen::SparseMatrix<double> IeegReference::bipolarMatrix(const QVector<IeegBipolarPair>& pairs, int contactCount)
{
    std::vector<Eigen::Triplet<double>> entries;
    entries.reserve(size_t(pairs.size()) * 2);
    for (int p = 0; p < pairs.size(); ++p) {
        entries.emplace_back(p, pairs[p].anode, 1.0);
        entries.emplace_back(p, pairs[p].cathode, -1.0);
    }
    Eigen::SparseMatrix<double> montage(pairs.size(), contactCount);
    montage.setFromTriplets(entries.begin(), entries.end());
    return montage;
}
