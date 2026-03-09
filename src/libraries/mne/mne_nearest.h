//=============================================================================================================
/**
 * @file     mne_nearest.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNENearest class declaration.
 *
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
