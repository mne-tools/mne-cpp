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
 * @brief    MneNearest class declaration.
 *
 */

#ifndef MNENEAREST_H
#define MNENEAREST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

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

class MnePatchInfo;

//=============================================================================================================
/**
 * Implements the MNE Nearest description (Replaces *mneNearest,mneNearestRec; struct of MNE-C mne_types.h).
 *
 * @brief This is used in the patch definitions
 */
class MNESHARED_EXPORT MneNearest
{
public:
    typedef QSharedPointer<MneNearest> SPtr;              /**< Shared pointer type for MneNearest. */
    typedef QSharedPointer<const MneNearest> ConstSPtr;   /**< Const shared pointer type for MneNearest. */

    //=========================================================================================================
    /**
     * Constructs the MNE Nearest
     */
    MneNearest();

    //=========================================================================================================
    /**
     * Destroys the MNE Nearest
     * Refactored:  (.c)
     */
    ~MneNearest();

public:
    int   vert;             /* Number of this vertex (to enable sorting) */
    int   nearest;          /* Nearest 'inuse' vertex */
    float dist;             /* Distance to the nearest 'inuse' vertex */
    MnePatchInfo* patch;    /* The patch information record for the patch this vertex belongs to */

// ### OLD STRUCT ###
//typedef struct {                /* This is used in the patch definitions */
//    int   vert;                 /* Number of this vertex (to enable sorting) */
//    int   nearest;              /* Nearest 'inuse' vertex */
//    float dist;                 /* Distance to the nearest 'inuse' vertex */
//    MnePatchInfo* patch;        /* The patch information record for the patch this vertex belongs to */
//} *mneNearest,mneNearestRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNENEAREST_H
