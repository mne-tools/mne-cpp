//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_picked.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Picked vertices / triangles on a mesh display surface.
 *
 * @ref mne::MNEMshPicked stores the indices selected by the user in
 * the legacy mesh viewer together with the kind of selection (vertex,
 * label, ROI). Used to round-trip @c tksurfer selection state into
 * mne-cpp.
 */

#ifndef MNEMSHPICKED_H
#define MNEMSHPICKED_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshPicked, mshPickedRec struct (analyze_types.c).
 *
 * @brief Picked point on a displayed surface storing vertex index, coordinates, and source value.
 */
class MNESHARED_EXPORT MNEMshPicked
{
public:
    int vert = 0;        /**< Index of the picked vertex. */
    bool sparse = false; /**< Whether this is an isolated point. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMSHPICKED_H
