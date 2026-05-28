//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_surface_patch.h
 * @since 2026
 * @date  March 2026
 * @brief Local surface patch (neighbours-of-neighbours) used by patch-based source priors.
 *
 * @ref MNELIB::MNESurfacePatch stores the vertex set, triangle subset
 * and geodesic distances of a cortical patch grown from a seed vertex.
 * Patches feed the cortical-orientation prior of the inverse operator
 * (@ref MNEPatchInfo) and are also used for source-space decimation
 * quality checks.
 */

#ifndef MNESURFACEPATCH_H
#define MNESURFACEPATCH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

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

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNESourceSpace;

//=============================================================================================================
/**
 * @brief Cortical surface patch linking a set of vertices to a single source space point.
 *
 * Each patch represents a contiguous region of a FreeSurfer cortical surface.
 * It stores the mapping between patch-local and full-surface vertex/triangle
 * indices and marks which vertices lie on the patch border.
 */
class MNESHARED_EXPORT MNESurfacePatch
{
public:
    typedef QSharedPointer<MNESurfacePatch> SPtr;              /**< Shared pointer type for MNESurfacePatch. */
    typedef QSharedPointer<const MNESurfacePatch> ConstSPtr;   /**< Const shared pointer type for MNESurfacePatch. */

    //=========================================================================================================
    /**
     * Constructs a patch with @p np vertices.
     *
     * @param[in] np  Number of vertices in the patch (0 for an empty patch).
     */
    MNESurfacePatch(int np);

    //=========================================================================================================
    /**
     * Destroys the MNESurfacePatch.
     */
    ~MNESurfacePatch();

public:
    std::unique_ptr<MNESourceSpace> s;		    /**< Patch represented as a source space surface. */
    Eigen::VectorXi  vert;	    /**< Vertex numbers in the complete surface (size np). */
    Eigen::VectorXi  surf_vert;	    /**< Map from complete-surface vertex index to patch vertex index (-1 if absent). */
    Eigen::VectorXi  tri;	    /**< Map from patch triangle index to complete-surface triangle index. */
    Eigen::VectorXi  surf_tri;	    /**< Map from complete-surface triangle index to patch triangle index (-1 if absent). */
    Eigen::VectorXi  border;	    /**< Per-vertex flag: non-zero if the vertex lies on the patch border (size np). */
    int              flat = 0;	    /**< Non-zero if the patch has been flattened. */
};

} // NAMESPACE MNELIB

#endif // MNESURFACEPATCH_H
