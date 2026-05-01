//=============================================================================================================
/**
 * @file     fs_label_utils.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Label manipulation utilities: grow, split, STC↔label conversion.
 *
 * Equivalent to MNE-Python's mne.grow_labels, mne.split_label,
 * mne.stc_to_label, mne.labels_to_stc.
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
 * @brief Static utilities for label manipulation on FreeSurfer surfaces.
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
