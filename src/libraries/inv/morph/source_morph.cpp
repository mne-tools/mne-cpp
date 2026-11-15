//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     source_morph.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::SourceMorph (matrix caching and apply step).
 *
 * Implements @c compute (caches the from/to vertex lists and the
 * sparse @c (nTo × nFrom) interpolation matrix) and @c apply, which is
 * a single sparse-matrix × dense-matrix product against the data block
 * of the input @ref InvSourceEstimate. Vertex lists, time origin and
 * time step are propagated from the input so the morphed estimate is
 * indistinguishable in metadata layout from one solved natively on the
 * target subject.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "source_morph.h"
#include <base/log.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SourceMorph::compute(const VectorXi& verticesFrom,
                          const VectorXi& verticesTo,
                          const SparseMatrix<double>& morphMap)
{
    if (verticesFrom.size() == 0 || verticesTo.size() == 0) {
        mne::Log::warning("[SourceMorph::compute] Empty vertex vectors.");
        return;
    }

    if (morphMap.rows() != verticesTo.size() || morphMap.cols() != verticesFrom.size()) {
        mne::Log::warning("[SourceMorph::compute] Morph map dimensions mismatch: {} x {} but expected {} x {}", morphMap.rows(), morphMap.cols(), verticesTo.size(), verticesFrom.size());
        return;
    }

    m_verticesFrom = verticesFrom;
    m_verticesTo = verticesTo;
    m_morphMatrix = morphMap;
    m_bComputed = true;
}

//=============================================================================================================

InvSourceEstimate SourceMorph::apply(const InvSourceEstimate& stcFrom) const
{
    if (!m_bComputed) {
        mne::Log::warning("[SourceMorph::apply] Morph not computed. Call compute() first.");
        return InvSourceEstimate();
    }

    if (stcFrom.empty()) {
        mne::Log::warning("[SourceMorph::apply] Source estimate is empty.");
        return InvSourceEstimate();
    }

    if (stcFrom.data.rows() != m_morphMatrix.cols()) {
        mne::Log::warning("[SourceMorph::apply] Source estimate rows {} != morph matrix cols {}", stcFrom.data.rows(), m_morphMatrix.cols());
        return InvSourceEstimate();
    }

    // Apply morph: data_to = morphMatrix * data_from
    MatrixXd morphedData = m_morphMatrix * stcFrom.data;

    InvSourceEstimate stcTo(morphedData, m_verticesTo, stcFrom.tmin, stcFrom.tstep);
    stcTo.method = stcFrom.method;
    stcTo.sourceSpaceType = stcFrom.sourceSpaceType;
    stcTo.orientationType = stcFrom.orientationType;

    return stcTo;
}
