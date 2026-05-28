//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file source_morph.h
 * @since 2026
 * @date  May 2026
 * @brief Sparse morph operator that re-samples an @ref INVLIB::InvSourceEstimate from one subject's source space onto another.
 *
 * @ref INVLIB::SourceMorph is the C++ peer of mne-python's
 * @c mne.compute_source_morph + @c SourceMorph.apply. It consumes the
 * sparse vertex-to-vertex interpolation matrix produced by
 * @c FSLIB::MNEMorphMap (nearest-neighbour on the subject sphere)
 * and stores it together with the from/to vertex lists so a precomputed
 * morph can be re-applied to many source estimates from the same
 * subject pair without re-doing the surface interpolation. The
 * resulting morphed estimate carries the @em to subject's vertex list
 * and time axis untouched, making it directly comparable to estimates
 * that were inverse-solved natively on the target subject.
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
