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
 * @ref MNELIB::MNEMshPicked stores the indices selected by the user in
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
// EIGEN INCLUDES
//=============================================================================================================

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

//=============================================================================================================
/**
 * Replaces *mshPicked, mshPickedRec struct (analyze_types.c).
 *
 * @brief Picked point on a displayed surface storing vertex index, coordinates, and source value.
 */
class MNESHARED_EXPORT MNEMshPicked
{
public:
    typedef QSharedPointer<MNEMshPicked> SPtr;              /**< Shared pointer type for MNEMshPicked. */
    typedef QSharedPointer<const MNEMshPicked> ConstSPtr;   /**< Const shared pointer type for MNEMshPicked. */

    //=========================================================================================================
    /**
     * Constructs the MNEMshPicked.
     */
    MNEMshPicked();

    //=========================================================================================================
    /**
     * Destroys the MNEMshPicked.
     */
    ~MNEMshPicked();

public:
    int   vert;			/* Vertex # */
    bool  sparse;			/* Is this a isolated point? */

// ### OLD STRUCT ###
//    typedef struct {
//      int   vert;			/* Vertex # */
//      int   sparse;			/* Is this a isolated point? */
//    } *mshPicked,mshPickedRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHPICKED_H
