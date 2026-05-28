//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_label_utils.h
 * @since May 2026
 * @brief Surface-mesh label manipulation: grow, split into connected components, STC ↔ label conversion.
 *
 * Algorithmic counterpart to @ref FsLabel I/O. Operates on labels in the
 * frame of an accompanying @ref FsSurface, using the surface’s triangle
 * connectivity as the vertex adjacency graph (one breadth-first hop per
 * @c step in @c growLabel; connectivity-component flood-fill in
 * @c splitLabel).
 *
 * The @c stcToLabel / @c labelsToStc pair converts between source-time-course
 * matrices indexed on a sparse subset of cortical vertices and the explicit
 * vertex-list label form. This is the mne-cpp equivalent of
 * @c mne.grow_labels, @c mne.split_label, @c mne.stc_to_label and
 * @c mne.labels_to_stc in MNE-Python; the surface metric and indexing
 * conventions are kept identical so round-tripping data between the two
 * stacks does not require resampling.
 */

#ifndef FS_LABEL_UTILS_H
#define FS_LABEL_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "fs_label.h"
#include "fs_surface.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QSet>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
/**
 * @brief Stateless utilities that grow, split and convert @ref FsLabel objects on a FreeSurfer triangular surface.
 *
 * All operations take an @ref FsSurface as the topology source and treat
 * its triangle faces as the vertex adjacency graph. The helpers are static
 * because labels carry their own state and the surface is supplied
 * per call, mirroring the function-based API used by MNE-Python.
 */
class FSSHARED_EXPORT FsLabelUtils
{
public:
    //=========================================================================================================
    /**
     * @brief Grow a label by expanding along the surface mesh.
     *
     * Starting from the seed vertices in the label, expands outward by
     * nSteps along surface edges (breadth-first).
     *
     * @param[in] label     The seed label to grow from.
     * @param[in] surface   The surface providing vertex adjacency.
     * @param[in] nSteps    Number of expansion steps.
     *
     * @return Grown label containing original + expanded vertices.
     */
    static FsLabel growLabel(const FsLabel& label,
                              const FsSurface& surface,
                              int nSteps);

    //=========================================================================================================
    /**
     * @brief Split a label into connected components.
     *
     * Uses the surface mesh to identify connected sub-labels.
     *
     * @param[in] label     The label to split.
     * @param[in] surface   The surface providing vertex adjacency.
     *
     * @return List of sub-labels (connected components).
     */
    static QList<FsLabel> splitLabel(const FsLabel& label,
                                      const FsSurface& surface);

    //=========================================================================================================
    /**
     * @brief Convert a source estimate to labels by thresholding.
     *
     * Vertices whose absolute value exceeds the threshold at any time point
     * are grouped into connected labels on the surface.
     *
     * @param[in] stcData       Source estimate data (n_vertices × n_times).
     * @param[in] vertices      Vertex indices in the source estimate.
     * @param[in] surface       The surface providing positions and adjacency.
     * @param[in] dThreshold    Absolute threshold for inclusion (default 0.0 = include all).
     * @param[in] iHemi         Hemisphere id (0=lh, 1=rh).
     *
     * @return List of labels (connected components above threshold).
     */
    static QList<FsLabel> stcToLabel(const Eigen::MatrixXd& stcData,
                                      const Eigen::VectorXi& vertices,
                                      const FsSurface& surface,
                                      double dThreshold = 0.0,
                                      int iHemi = 0);

    //=========================================================================================================
    /**
     * @brief Convert labels to a binary source estimate mask.
     *
     * Creates a matrix of ones for vertices inside the labels, zeros otherwise.
     *
     * @param[in] labels         Labels to convert.
     * @param[in] stcVertices    Vertex indices of the target STC space.
     * @param[in] nTimes         Number of time points.
     *
     * @return Binary mask (n_vertices × n_times).
     */
    static Eigen::MatrixXd labelsToStc(const QList<FsLabel>& labels,
                                        const Eigen::VectorXi& stcVertices,
                                        int nTimes);

    //=========================================================================================================
    /**
     * @brief Build adjacency list from surface triangle mesh.
     *
     * @param[in] tris   Triangle matrix (n_tris × 3).
     * @param[in] nVerts Total number of vertices.
     *
     * @return Adjacency list: for each vertex, the set of neighbor vertices.
     */
    static QList<QSet<int>> buildAdjacency(const Eigen::MatrixX3i& tris,
                                            int nVerts);
};

} // namespace FSLIB

#endif // FS_LABEL_UTILS_H
