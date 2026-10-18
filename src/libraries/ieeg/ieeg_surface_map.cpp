//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_surface_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Distance-weighted mapping of contact values onto surface vertices.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_surface_map.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IEEGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Eigen::SparseMatrix<double> IeegSurfaceMap::weights(const Eigen::MatrixX3f& contacts,
                                                    const Eigen::MatrixX3f& vertices,
                                                    double distance,
                                                    IeegSurfaceMapMode mode,
                                                    bool project)
{
    const Eigen::Index nVert = vertices.rows();
    const Eigen::Index nContacts = contacts.rows();
    Eigen::SparseMatrix<double> w(nVert, nContacts);
    if (nVert == 0 || nContacts == 0 || distance <= 0.0)
        return w;

    // Linear fall-off within reach, per vertex: (contact, weight)
    const Eigen::MatrixX3d rr = vertices.cast<double>();
    std::vector<std::vector<std::pair<Eigen::Index, double>>> near(static_cast<size_t>(nVert));
    const double reach2 = distance * distance;
    for (Eigen::Index c = 0; c < nContacts; ++c) {
        Eigen::RowVector3d pos = contacts.row(c).cast<double>();
        if (!pos.allFinite())
            continue;
        Eigen::VectorXd d2 = (rr.rowwise() - pos).rowwise().squaredNorm();
        if (project) {
            Eigen::Index nearest = 0;
            d2.minCoeff(&nearest);
            pos = rr.row(nearest);
            d2 = (rr.rowwise() - pos).rowwise().squaredNorm();
        }
        for (Eigen::Index v = 0; v < nVert; ++v) {
            if (d2(v) <= reach2)
                near[size_t(v)].emplace_back(c, 1.0 - std::sqrt(d2(v)) / distance);
        }
    }

    std::vector<Eigen::Triplet<double>> entries;
    for (Eigen::Index v = 0; v < nVert; ++v) {
        auto& list = near[size_t(v)];
        if (list.empty())
            continue;
        if (mode == IeegSurfaceMapMode::Single || mode == IeegSurfaceMapMode::Nearest) {
            // The nearest contact has the largest weight; the first one wins a tie, as numpy's argmax
            auto best = list.begin();
            for (auto it = list.begin(); it != list.end(); ++it) {
                if (it->second > best->second)
                    best = it;
            }
            entries.emplace_back(v, best->first, mode == IeegSurfaceMapMode::Single ? best->second : 1.0);
            continue;
        }
        double norm = 1.0;
        if (mode == IeegSurfaceMapMode::Weighted) {
            double sum = 0.0;
            for (const auto& [c, weight] : list)
                sum += weight;
            norm = sum > 0.0 ? sum : 1.0;
        }
        for (const auto& [c, weight] : list) {
            if (weight > 0.0)
                entries.emplace_back(v, c, weight / norm);
        }
    }
    w.setFromTriplets(entries.begin(), entries.end());
    return w;
}
