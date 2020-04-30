//=============================================================================================================
/**
 * @file     mne_surface_patch.h
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
 * @brief    MneSurfacePatch class declaration.
 *
 */

#ifndef MNESURFACEPATCH_H
#define MNESURFACEPATCH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

typedef void (*mneUserFreeFunc)(void *);  /* General purpose */

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

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

class MneSourceSpaceOld;

//=============================================================================================================
/**
 * Replaces *mneSurfacePatch,mneSurfacePatchRec struct (mne_types.c).
 *
 * @brief The MneSurfacePatch class.
 */
class MNESHARED_EXPORT MneSurfacePatch
{
public:
    typedef QSharedPointer<MneSurfacePatch> SPtr;              /**< Shared pointer type for MneSurfacePatch. */
    typedef QSharedPointer<const MneSurfacePatch> ConstSPtr;   /**< Const shared pointer type for MneSurfacePatch. */

    //=========================================================================================================
    /**
     * Constructs the MneSurfacePatch.
     */
    MneSurfacePatch(int np);

    //=========================================================================================================
    /**
     * Destroys the MneSurfacePatch.
     */
    ~MneSurfacePatch();

public:
    MneSourceSpaceOld       *s;		    /* Patch represented as a surface */
    int              *vert;	    /* Vertex numbers in the complete surface*/
    int              *surf_vert;	    /* Which vertex corresponds to each complete surface vertex here? */
    int              np_surf;	    /* How many points on the complete surface? */
    int              *tri;	    /* Which triangles in the complete surface correspond to our triangles? */
    int              *surf_tri;	    /* Which of our triangles corresponds to each triangle on the complete surface? */
    int              ntri_surf;	    /* How many triangles on the complete surface */
    int              *border;	    /* Is this vertex on the border? */
    int              flat;	    /* Is this a flat patch? */
    void             *user_data;      /* Anything else we want */
    mneUserFreeFunc  user_data_free;  /* Function to set the above free */

// ### OLD STRUCT ###
//    typedef struct {		    /* FreeSurfer patches */
//      mneSurface       s;		    /* Patch represented as a surface */
//      int              *vert;	    /* Vertex numbers in the complete surface*/
//      int              *surf_vert;	    /* Which vertex corresponds to each complete surface vertex here? */
//      int              np_surf;	    /* How many points on the complete surface? */
//      int              *tri;	    /* Which triangles in the complete surface correspond to our triangles? */
//      int              *surf_tri;	    /* Which of our triangles corresponds to each triangle on the complete surface? */
//      int              ntri_surf;	    /* How many triangles on the complete surface */
//      int              *border;	    /* Is this vertex on the border? */
//      int              flat;	    /* Is this a flat patch? */
//      void             *user_data;      /* Anything else we want */
//      mneUserFreeFunc  user_data_free;  /* Function to set the above free */
//    } *mneSurfacePatch,mneSurfacePatchRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNESURFACEPATCH_H
