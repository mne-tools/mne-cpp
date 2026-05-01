//=============================================================================================================
/**
 * @file     source_morph.h
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
 * @brief    SourceMorph class for morphing source estimates between subjects.
 *
 * Equivalent to MNE-Python's mne.compute_source_morph() + SourceMorph.apply().
 * Uses nearest-neighbor interpolation via MNEMorphMap.
 */

#ifndef SOURCE_MORPH_H
#define SOURCE_MORPH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Morphs source estimates from one subject's source space to another.
 *
 * Usage:
 * @code
 *   SourceMorph morph;
 *   morph.compute(srcVerticesFrom, srcVerticesTo, morphMapLh, morphMapRh);
 *   InvSourceEstimate morphed = morph.apply(stc);
 * @endcode
 */
class INVSHARED_EXPORT SourceMorph
{
public:
    SourceMorph() = default;

    //=========================================================================================================
    /**
     * @brief Compute the morphing transformation.
     *
     * Builds a sparse interpolation matrix that maps source estimate data
     * from one vertex set to another using nearest-neighbor interpolation.
     *
     * @param[in] verticesFrom  Source vertices in the "from" subject's source space.
     * @param[in] verticesTo    Target vertices in the "to" subject's source space.
     * @param[in] morphMap      Sparse interpolation matrix (nTo x nFrom) — e.g. from MNEMorphMap::map.
     */
    void compute(const Eigen::VectorXi& verticesFrom,
                 const Eigen::VectorXi& verticesTo,
                 const Eigen::SparseMatrix<double>& morphMap);

    //=========================================================================================================
    /**
     * @brief Apply the morphing to a source estimate.
     *
     * @param[in] stcFrom  Source estimate in the "from" subject's space.
     *
     * @return Morphed source estimate in the "to" subject's space.
     */
    InvSourceEstimate apply(const InvSourceEstimate& stcFrom) const;

    //=========================================================================================================
    /**
     * @brief Check if the morph has been computed.
     */
    bool isComputed() const { return m_bComputed; }

    //=========================================================================================================
    /**
     * @brief Get the number of target vertices.
     */
    int nVerticesTo() const { return static_cast<int>(m_verticesTo.size()); }

private:
    bool m_bComputed = false;
    Eigen::VectorXi m_verticesFrom;
    Eigen::VectorXi m_verticesTo;
    Eigen::SparseMatrix<double> m_morphMatrix;  /**< (nTo x nFrom) interpolation matrix. */
};

} // namespace INVLIB

#endif // SOURCE_MORPH_H
