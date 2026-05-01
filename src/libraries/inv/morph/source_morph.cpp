//=============================================================================================================
/**
 * @file     source_morph.cpp
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
 * @brief    SourceMorph class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "source_morph.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SourceMorph::compute(const VectorXi& verticesFrom,
                           const VectorXi& verticesTo,
                           const SparseMatrix<double>& morphMap)
{
    if (verticesFrom.size() == 0 || verticesTo.size() == 0) {
        qWarning() << "[SourceMorph::compute] Empty vertex vectors.";
        return;
    }

    if (morphMap.rows() != verticesTo.size() || morphMap.cols() != verticesFrom.size()) {
        qWarning() << "[SourceMorph::compute] Morph map dimensions mismatch:"
                   << morphMap.rows() << "x" << morphMap.cols()
                   << "but expected" << verticesTo.size() << "x" << verticesFrom.size();
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
        qWarning() << "[SourceMorph::apply] Morph not computed. Call compute() first.";
        return InvSourceEstimate();
    }

    if (stcFrom.isEmpty()) {
        qWarning() << "[SourceMorph::apply] Source estimate is empty.";
        return InvSourceEstimate();
    }

    if (stcFrom.data.rows() != m_morphMatrix.cols()) {
        qWarning() << "[SourceMorph::apply] Source estimate rows" << stcFrom.data.rows()
                   << "!= morph matrix cols" << m_morphMatrix.cols();
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
