//=============================================================================================================
/**
 * @file     filter_thread_arg.h
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
 * @brief    Filter Thread Argument (FilterThreadArg) class declaration.
 *
 */

#ifndef FILTERTHREADARG_H
#define FILTERTHREADARG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

#include <fiff/c/fiff_coord_trans_old.h>

#include "mne_source_space_old.h"
#include "mne_surface_old.h"

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
/**
 * Implements a Filter Thread Argument (Replaces *filterThreadArg,filterThreadArgRec; struct of MNE-C filter_source_space.c).
 *
 * @brief Filter Thread Argument Description
 */
class MNESHARED_EXPORT FilterThreadArg
{
public:
    typedef QSharedPointer<FilterThreadArg> SPtr;              /**< Shared pointer type for FilterThreadArg. */
    typedef QSharedPointer<const FilterThreadArg> ConstSPtr;   /**< Const shared pointer type for FilterThreadArg. */

    //=========================================================================================================
    /**
     * Constructs the Filter Thread Argument
     * Refactored: new_filter_thread_arg (filter_source_space.c)
     */
    FilterThreadArg();

    //=========================================================================================================
    /**
     * Destroys the Filter Thread Argument
     * Refactored: free_filter_thread_arg (filter_source_space.c)
     */
    ~FilterThreadArg();

public:
    MneSourceSpaceOld* s;           /* The source space to process */
    FIFFLIB::FiffCoordTransOld* mri_head_t;  /* Coordinate transformation */
    MneSurfaceOld*   surf;          /* The inner skull surface */
    float          limit;           /* Distance limit */
    FILE           *filtered;       /* Log omitted point locations here */
    int            stat;            /* How was it? */

// ### OLD STRUCT ###
//typedef struct {
//    MneSourceSpaceOld* s;           /* The source space to process */
//    FiffCoordTransOld* mri_head_t;  /* Coordinate transformation */
//    MneSurfaceOld*   surf;          /* The inner skull surface */
//    float          limit;           /* Distance limit */
//    FILE           *filtered;       /* Log omitted point locations here */
//    int            stat;            /* How was it? */
//} *filterThreadArg,filterThreadArgRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // FILTERTHREADARG_H
