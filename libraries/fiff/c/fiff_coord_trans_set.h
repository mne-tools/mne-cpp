//=============================================================================================================
/**
 * @file     fiff_coord_trans_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    FiffCoordTransSet class declaration.
 *
 */

#ifndef FIFFCOORDTRANSSET_H
#define FIFFCOORDTRANSSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffCoordTransOld;

//=============================================================================================================
/**
 * Implements an Fiff Coordinate Descriptor Set (Replaces *coordTransSet,coordTransSetRec; analyze_types.h).
 *
 * @brief Coordinate transformation descriptor
 */
class FIFFSHARED_EXPORT FiffCoordTransSet
{
public:
    typedef QSharedPointer<FiffCoordTransSet> SPtr;              /**< Shared pointer type for FiffCoordTransSet. */
    typedef QSharedPointer<const FiffCoordTransSet> ConstSPtr;   /**< Const shared pointer type for FiffCoordTransSet. */

    //=========================================================================================================
    /**
     * Constructs the FiffCoordTransSet
     */
    FiffCoordTransSet();

    //=========================================================================================================
    /**
     * Destroys the FiffCoordTransSet
     * Refactored:  (.c)
     */
    ~FiffCoordTransSet();

public:
    FIFFLIB::FiffCoordTransOld*    head_surf_RAS_t;   /* Transform from MEG head coordinates to surface RAS */
    FIFFLIB::FiffCoordTransOld*    surf_RAS_RAS_t;    /* Transform from surface RAS to RAS (nonzero origin) coordinates */
    FIFFLIB::FiffCoordTransOld*    RAS_MNI_tal_t;     /* Transform from RAS (nonzero origin) to MNI Talairach coordinates */
    FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_gtz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0) */
    FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_ltz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0) */

    // ### OLD STRUCT ###
//    typedef struct {
//      FIFFLIB::FiffCoordTransOld*    head_surf_RAS_t;   /* Transform from MEG head coordinates to surface RAS */
//      FIFFLIB::FiffCoordTransOld*    surf_RAS_RAS_t;    /* Transform from surface RAS to RAS (nonzero origin) coordinates */
//      FIFFLIB::FiffCoordTransOld*    RAS_MNI_tal_t;     /* Transform from RAS (nonzero origin) to MNI Talairach coordinates */
//      FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_gtz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0) */
//      FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_ltz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0) */
//    } *coordTransSet,coordTransSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FIFFLIB

#endif // FIFFCOORDTRANSSET_H
