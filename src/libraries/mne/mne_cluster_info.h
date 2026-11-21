//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     mne_cluster_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Bookkeeping for a label-restricted clustering of source-space leadfield columns.
 *
 * @ref mne::MNEClusterInfo stores, for every cluster produced by
 * @ref mne::MNEForwardSolution::cluster_forward_solution "MNEForwardSolution::cluster_forward_solution", the source-space
 * vertices it contains, the representative dipole position and the
 * mean orientation. Used to compress the gain matrix into a much
 * smaller per-cluster leadfield and to expand the clustered solution
 * back onto the cortex for visualisation.
 */

#ifndef MNE_CLUSTER_INFO_H
#define MNE_CLUSTER_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Per-cluster bookkeeping for a label-restricted clustering of a forward
 * solution: contained vertices, representative dipole and mean orientation.
 *
 * @brief Cluster table used to compress and reconstruct a clustered leadfield.
 *
 * @snippet ex_inv_api/main.cpp mne_cluster_info_usage
 */
class MNESHARED_EXPORT MNEClusterInfo
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEClusterInfo();

    //=========================================================================================================
    /**
     * Initializes the cluster information.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns true if MNE cluster information contains no data.
     *
     * @return true if MNE cluster information is empty.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Returns the number of clusters
     *
     * @return number of clusters.
     */
    inline std::int32_t numClust() const;

    //=========================================================================================================
    /**
     * Writes the cluster info as text, and the centroid vertex numbers to
     * @c centroids_<name> in the same directory.
     *
     * @param[in] p_sFileName    FileName to write to.
     *
     * @return True if both files were written.
     */
    [[nodiscard]] bool write(const std::filesystem::path& p_sFileName) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand cluster info.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const MNEClusterInfo& a, const MNEClusterInfo& b);

public:
    std::vector<std::string> clusterLabelNames;     /**< FsLabel name of the cluster. Entries can be non unique, since some FsLabel consist of more than one cluster.*/
    std::vector<std::int32_t> clusterLabelIds;      /**< Id (FsLabel/ROI id) of the cluster. Entries can be non unique, since some FsLabel/ROI consist of more than one cluster.*/
    std::vector<std::int32_t> centroidVertno;       /**< Id (FsLabel/ROI id) of the centroid. */
    std::vector<Eigen::Vector3f> centroidSource_rr; /**< Centroid location. */
    std::vector<Eigen::VectorXi> clusterVertnos;    /**< Vertnos which belong to corresponding cluster. */
    std::vector<Eigen::MatrixX3f> clusterSource_rr; /**< Cluster source locations. */
    std::vector<Eigen::VectorXd> clusterDistances;  /**< Distances to clusters centroid. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEClusterInfo::empty() const
{
    return !(this->clusterVertnos.size() > 0);
}

//=============================================================================================================

inline std::int32_t MNEClusterInfo::numClust() const
{
    return static_cast<int>(this->clusterVertnos.size());
}

//=============================================================================================================

inline bool operator==(const MNEClusterInfo& a, const MNEClusterInfo& b)
{
    if (a.centroidSource_rr.size() == b.centroidSource_rr.size()) {
        for (int i = 0; i < std::ssize(a.centroidSource_rr); ++i) {
            if (!a.centroidSource_rr.at(i).isApprox(b.centroidSource_rr.at(i), 0.0001f)) {
                return false;
            }
        }
    } else {
        return false;
    }

    if (a.clusterVertnos.size() == b.clusterVertnos.size()) {
        for (int i = 0; i < std::ssize(a.clusterVertnos); ++i) {
            if (!a.clusterVertnos.at(i).isApprox(b.clusterVertnos.at(i))) {
                return false;
            }
        }
    } else {
        return false;
    }

    if (a.clusterSource_rr.size() == b.clusterSource_rr.size()) {
        for (int i = 0; i < std::ssize(a.clusterSource_rr); ++i) {
            if (!a.clusterSource_rr.at(i).isApprox(b.clusterSource_rr.at(i), 0.0001f)) {
                return false;
            }
        }
    } else {
        return false;
    }

    if (a.clusterDistances.size() == b.clusterDistances.size()) {
        for (int i = 0; i < std::ssize(a.clusterDistances); ++i) {
            if (!a.clusterDistances.at(i).isApprox(b.clusterDistances.at(i))) {
                return false;
            }
        }
    } else {
        return false;
    }

    return (a.clusterLabelNames == b.clusterLabelNames &&
            a.clusterLabelIds == b.clusterLabelIds &&
            a.centroidVertno == b.centroidVertno);
}
} // NAMESPACE

#endif // MNE_CLUSTER_INFO_H
