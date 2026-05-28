//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_morph_map.h
 * @since March 2026
 * @brief Sparse linear operator that morphs a source estimate between two subjects.
 *
 * @ref MNELIB::MNEMorphMap composes a left and a right @ref MNECorticalMap
 * into the rectangular sparse matrix used by @ref INVERSELIB::MneMorph
 * to project a @ref MNESourceEstimate from a subject's native cortex to
 * a target (typically @c fsaverage) cortex. Backed by
 * @ref MNELIB::MNESparseNamedMatrix so that the row/column hemisphere
 * layout is preserved.
 */

#ifndef MNEMORPHMAP_H
#define MNEMORPHMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "fiff/fiff_sparse_matrix.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *morphMap,morphMapRec struct (analyze_types.c).
 *
 * @brief Vertex-to-vertex mapping between two FreeSurfer surface meshes for morphing source estimates.
 */
class MNESHARED_EXPORT MNEMorphMap
{
public:
    typedef QSharedPointer<MNEMorphMap> SPtr;              /**< Shared pointer type for MNEMorphMap. */
    typedef QSharedPointer<const MNEMorphMap> ConstSPtr;   /**< Const shared pointer type for MNEMorphMap. */

    //=========================================================================================================
    /**
     * Constructs the MNEMorphMap.
     */
    MNEMorphMap() = default;

    //=========================================================================================================
    /**
     * Destroys the MNEMorphMap.
     */
    ~MNEMorphMap() = default;

public:
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> map;  /**< Sparse interpolation matrix: multiply source surface data
                                                          by this to obtain values on the target ('this') surface. */
    Eigen::VectorXi best;                            /**< Index of the closest source surface vertex for each target vertex. */
    int from_kind = -1;                              /**< FsSurface kind identifier (e.g., hemisphere) of the source surface (-1 = unknown). */
    QString from_subj;                               /**< Subject name of the source surface. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMORPHMAP_H
