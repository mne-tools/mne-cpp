//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_adjacency.h
 * @since April 2026
 * @brief Construction of the sensor- and source-space neighbourhood graphs that define the cluster support for permutation testing.
 *
 * Cluster-based inference (@ref STSLIB::StatsCluster) reduces the
 * multiple-comparison problem by grouping supra-threshold samples into
 * spatially - and optionally temporally - connected clusters before
 * computing a max-statistic null. Doing this rigorously requires an
 * explicit adjacency graph whose edges encode which (channel, time) or
 * (vertex, time) pairs are allowed to merge.
 *
 * @ref STSLIB::StatsAdjacency provides three constructors that cover the
 * standard M/EEG analysis cases: a sensor graph built from the 3D
 * channel positions in a @ref FIFFLIB::FiffInfo using a heuristic of
 * three times the median nearest-neighbour distance, a cortical graph
 * built from the triangulation of a source space, and a spatio-temporal
 * extension of the cortical graph that links each vertex to itself at
 * the previous and next time sample (linear index @c vertex*nTimes+time).
 * The output is always a symmetric sparse integer matrix consumed
 * unchanged by @ref STSLIB::StatsCluster.
 *
 * Reference: Maris & Oostenveld (2007), J. Neurosci. Methods 164(1).
 */

#ifndef STS_ADJACENCY_H
#define STS_ADJACENCY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB
{

//=============================================================================================================
/**
 * Adjacency matrix construction for spatial clustering.
 *
 * @brief Builds the sparse spatial and spatio-temporal neighbourhood graphs that define cluster support for permutation tests.
 */
class STSSHARED_EXPORT StatsAdjacency
{
public:
    //=========================================================================================================
    /**
     * Build a spatial adjacency matrix from channel positions in FiffInfo.
     * Uses a distance threshold of 3x the median nearest-neighbor distance.
     *
     * @param[in] info   FiffInfo with channel positions.
     * @param[in] picks  Optional list of channel names to include. If empty, all channels are used.
     *
     * @return Sparse adjacency matrix (symmetric, nChannels x nChannels).
     */
    static Eigen::SparseMatrix<int> fromChannelPositions(
        const FIFFLIB::FiffInfo& info,
        const QStringList& picks = QStringList());

    //=========================================================================================================
    /**
     * Build a spatial adjacency matrix from a triangulated source space.
     *
     * @param[in] tris       Triangle definitions (nTris x 3, vertex indices).
     * @param[in] nVertices  Total number of vertices.
     *
     * @return Sparse adjacency matrix (symmetric, nVertices x nVertices).
     */
    static Eigen::SparseMatrix<int> fromSourceSpace(
        const Eigen::MatrixX3i& tris,
        int nVertices);

    //=========================================================================================================
    /**
     * Build a spatio-temporal adjacency matrix from a triangulated source space.
     *
     * Constructs a (nVertices*nTimes) x (nVertices*nTimes) adjacency matrix where spatial
     * neighbors come from the triangle mesh and temporal neighbors connect each vertex to
     * itself at t-1 and t+1. The linear index is vertex * nTimes + time.
     *
     * @param[in] tris       Triangle definitions (nTris x 3, vertex indices).
     * @param[in] nVertices  Total number of vertices.
     * @param[in] nTimes     Number of time points.
     *
     * @return Sparse adjacency matrix (symmetric, nVertices*nTimes x nVertices*nTimes).
     *
     * @since 2.2.0
     */
    static Eigen::SparseMatrix<int> fromSourceSpaceTemporal(
        const Eigen::MatrixX3i& tris,
        int nVertices,
        int nTimes);
};

} // namespace STSLIB

#endif // STS_ADJACENCY_H
