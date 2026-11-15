//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     mne_cluster_info.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Implementation of @ref mne::MNEClusterInfo.
 *
 * Implements constructors and the helpers that group source vertices
 * into clusters / undo the grouping when expanding back to the original
 * source space.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_cluster_info.h"
#include <base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <fstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEClusterInfo::MNEClusterInfo()
{
}

//=============================================================================================================

void MNEClusterInfo::clear()
{
    clusterLabelNames.clear();
    clusterLabelIds.clear();
    centroidVertno.clear();
    centroidSource_rr.clear();
    clusterVertnos.clear();
    clusterSource_rr.clear();
    clusterDistances.clear();
}

//=============================================================================================================

bool MNEClusterInfo::write(const std::filesystem::path& p_sFileName) const
{
    std::ofstream out(p_sFileName);
    if (!out) {
        mne::Log::warning("MNEClusterInfo::write - Cannot open {}", mne::utf8_from_path(p_sFileName));
        return false;
    }
    out << "MNE Cluster Info\n";

    for (std::size_t i = 0; i < clusterLabelIds.size(); ++i) {
        out << "\nLabel : " << clusterLabelNames[i] << "\n";
        out << "FsLabel ID : " << clusterLabelIds[i] << "\n";
        out << "Centroid Vertno : " << centroidVertno[i] << "\n";
        out << "Centroid rr : " << centroidSource_rr[i](0) << ", " << centroidSource_rr[i](1) << ", " << centroidSource_rr[i](2) << "\n";
        out << "Vertnos :\n";
        for (const int vertno : clusterVertnos[i])
            out << vertno << ", ";
        out << "\nDistances :\n";
        for (const double distance : clusterDistances[i])
            out << distance << ", ";
        out << "\nrr :\n";
        for (Eigen::Index j = 0; j < clusterSource_rr[i].rows(); ++j)
            out << clusterSource_rr[i](j, 0) << ", " << clusterSource_rr[i](j, 1) << ", " << clusterSource_rr[i](j, 2) << "\n";

        out << "\n";
    }
    if (!out.flush()) {
        return false;
    }

    // Next to the main file: <dir>/centroids_<name>
    std::filesystem::path centroidsPath = p_sFileName;
    centroidsPath.replace_filename("centroids_" + p_sFileName.filename().string());
    std::ofstream centroids(centroidsPath);
    for (const std::int32_t vertno : centroidVertno)
        centroids << vertno << ", ";
    return static_cast<bool>(centroids.flush());
}
