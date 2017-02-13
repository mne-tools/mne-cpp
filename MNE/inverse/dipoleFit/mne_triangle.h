//=============================================================================================================
/**
* @file     mne_triangle.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    MneTriangle class declaration.
*
*/

#ifndef MNETRIANGLE_H
#define MNETRIANGLE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Implements the MNE Triangle description (Replaces *mneTriangle,mneTriangleRec; struct of MNE-C mne_types.h).
*
* @brief Triangle data
*/
class INVERSESHARED_EXPORT MneTriangle
{
public:
    typedef QSharedPointer<MneTriangle> SPtr;              /**< Shared pointer type for MneTriangle. */
    typedef QSharedPointer<const MneTriangle> ConstSPtr;   /**< Const shared pointer type for MneTriangle. */

    //=========================================================================================================
    /**
    * Constructs the MNE Triangle
    */
    MneTriangle();

    //=========================================================================================================
    /**
    * Destroys the MNE Triangle
    * Refactored:  (.c)
    */
    ~MneTriangle();

public:
    int   *vert;            /* Triangle vertices (pointers to the itris member of the associated mneSurface) */
    float *r1,*r2,*r3;      /* Triangle vertex locations (pointers to the rr member of the associated mneSurface) */
    float r12[3],r13[3];    /* Vectors along the sides */
    float nn[3];            /* Normal vector */
    float area;             /* Area */
    float cent[3];          /* Centroid */
    float ex[3],ey[3];      /* Other unit vectors (used by BEM calculations) */

// ### OLD STRUCT ###
//typedef struct {
//    int   *vert;            /* Triangle vertices (pointers to the itris member of the associated mneSurface) */
//    float *r1,*r2,*r3;      /* Triangle vertex locations (pointers to the rr member of the associated mneSurface) */
//    float r12[3],r13[3];    /* Vectors along the sides */
//    float nn[3];            /* Normal vector */
//    float area;             /* Area */
//    float cent[3];          /* Centroid */
//    float ex[3],ey[3];      /* Other unit vectors (used by BEM calculations) */
//} *mneTriangle,mneTriangleRec;  /* Triangle data */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE INVERSELIB

#endif // MNETRIANGLE_H
