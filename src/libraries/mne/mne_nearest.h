//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_nearest.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-source-space-vertex nearest-cortex-vertex mapping.
 *
 * @ref MNELIB::MNENearest records, for each vertex of an
 * @ref MNESourceSpace, the index of and distance to the closest vertex
 * of the full cortical surface. It is the structure stored under
 * @c FIFF_MNE_SOURCE_SPACE_NEAREST / @c _NEAREST_DIST and is used to
 * map decimated source-space values back onto the dense cortex for
 * rendering and morphing.
 */

#ifndef MNENEAREST_H
#define MNENEAREST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNEPatchInfo;

//=============================================================================================================
/**
 * Implements the MNE Nearest description (Replaces *mneNearest,mneNearestRec; struct of MNE-C mne_types.h).
 *
 * @brief This is used in the patch definitions
 */
class MNESHARED_EXPORT MNENearest
{
public:
    typedef QSharedPointer<MNENearest> SPtr;              /**< Shared pointer type for MNENearest. */
    typedef QSharedPointer<const MNENearest> ConstSPtr;   /**< Const shared pointer type for MNENearest. */

    //=========================================================================================================
    /**
     * Constructs the MNE Nearest
     */
    MNENearest();

    //=========================================================================================================
    /**
     * Destroys the MNE Nearest
     * Refactored:  (.c)
     */
    ~MNENearest();

public:
    int   vert;             /**< Vertex index in the full surface mesh. */
    int   nearest;          /**< Index of the nearest 'inuse' vertex. */
    float dist;             /**< Distance to the nearest 'inuse' vertex (meters). */
    MNEPatchInfo* patch;    /**< Non-owning pointer to the patch this vertex belongs to.
                             *   Owned by MNESourceSpace::patches (unique_ptr).
                             *   Multiple MNENearest objects share the same MNEPatchInfo. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNENEAREST_H
