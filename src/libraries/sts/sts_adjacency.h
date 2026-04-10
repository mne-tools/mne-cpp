//=============================================================================================================
/**
 * @file     sts_adjacency.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    StatsAdjacency class declaration.
 *
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
 * @brief Adjacency matrices from channel positions or source spaces.
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
};

} // namespace STSLIB

#endif // STS_ADJACENCY_H
