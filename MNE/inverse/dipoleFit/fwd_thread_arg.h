//=============================================================================================================
/**
* @file     fwd_thread_arg.h
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
* @brief    Fwd Thread Argument (FwdThreadArg) class declaration.
*
*/

#ifndef FWDTHREADARG_H
#define FWDTHREADARG_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "fwd_types.h"


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
// Forward Declarations
//=============================================================================================================

class FwdCoilSet;
class MneSourceSpaceOld;


//=============================================================================================================
/**
* Implements a Forward Thread Argument (Replaces *fwdThreadArg,fwdThreadArgRec; struct of MNE-C compute_forward.c).
*
* @brief Filter Thread Argument Description
*/
class INVERSESHARED_EXPORT FwdThreadArg
{
public:
    typedef QSharedPointer<FwdThreadArg> SPtr;              /**< Shared pointer type for FwdThreadArg. */
    typedef QSharedPointer<const FwdThreadArg> ConstSPtr;   /**< Const shared pointer type for FwdThreadArg. */

    //=========================================================================================================
    /**
    * Constructs the Forward Thread Argument
    * Refactored: new_fwd_thread_arg (compute_forward.c)
    */
    FwdThreadArg();

    //=========================================================================================================
    /**
    * Destroys the Forward Thread Argument
    * Refactored: free_fwd_thread_arg (compute_forward.c)
    */
    ~FwdThreadArg();

public:
    float               **res;             /* Destination for the solution */
    float               **res_grad;        /* Gradient result */
    int                 off;               /* Offset within the result to the first source space vertex solution */
    fwdFieldFunc        field_pot;         /* Computes the field or potential for one dipole orientation */
    fwdVecFieldFunc     vec_field_pot;     /* Computes the field or potential for all dipole orientations */
    fwdFieldGradFunc    field_pot_grad;    /* Computes the gradient of field or potential for one dipole orientation */
    FwdCoilSet          *coils_els;        /* The coil definitions */
    void                *client;           /* Client data for the field computation function */
    MneSourceSpaceOld   *s;                 /* The source space to process */
    int                 fixed_ori;         /* Compute fixed orientation solution? */
    int                 comp;              /* Which component to compute for free orientations */
    int                 stat;

// ### OLD STRUCT ###
//typedef struct {
//    float            **res;             /* Destination for the solution */
//    float            **res_grad;        /* Gradient result */
//    int              off;               /* Offset within the result to the first source space vertex solution */
//    fwdFieldFunc     field_pot;         /* Computes the field or potential for one dipole orientation */
//    fwdVecFieldFunc  vec_field_pot;     /* Computes the field or potential for all dipole orientations */
//    fwdFieldGradFunc field_pot_grad;    /* Computes the gradient of field or potential for one dipole orientation */
//    fwdCoilSet       coils_els;         /* The coil definitions */
//    void             *client;           /* Client data for the field computation function */
//    mneSourceSpace   s;                 /* The source space to process */
//    int              fixed_ori;         /* Compute fixed orientation solution? */
//    int              comp;              /* Which component to compute for free orientations */
//    int              stat;
//} *fwdThreadArg,fwdThreadArgRec;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE INVERSELIB

#endif // FWDTHREADARG_H
