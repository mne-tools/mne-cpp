//=============================================================================================================
/**
* @file     fwd_coil_set.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    FwdCoilSet class declaration.
*
*/

#ifndef FWDCOILSET_H
#define FWDCOILSET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "fwd_coil.h"
#include <fiff/fiff_types.h>


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



typedef void (*fwdUserFreeFunc)(void *);  /* General purpose */


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{


//=============================================================================================================
/**
* Implements FwdCoilSet (Replaces *fwdCoilSet,fwdCoilSetRec; struct of MNE-C fwd_types.h).
*
* @brief FwdCoilSet description
*/
class INVERSESHARED_EXPORT FwdCoilSet
{
public:
    typedef QSharedPointer<FwdCoilSet> SPtr;              /**< Shared pointer type for FwdCoilSet. */
    typedef QSharedPointer<const FwdCoilSet> ConstSPtr;   /**< Const shared pointer type for FwdCoilSet. */

    //=========================================================================================================
    /**
    * Constructs the Forward Coil Set description
    * Refactored: fwd_new_coil_set
    */
    FwdCoilSet();

//    //=========================================================================================================
//    /**
//    * Copy constructor.
//    *
//    * @param[in] p_FwdCoilSet      FwdCoilSet which should be copied
//    */
//    FwdCoilSet(const FwdCoilSet& p_FwdCoilSet);

    //=========================================================================================================
    /**
    * Destroys the Forward Coil Set description
    * Refactored: fwd_free_coil_set, fwd_free_coil_set_user_data
    */
    ~FwdCoilSet();


    void fwd_free_coil_set_user_data()
    {
        if (user_data_free && user_data)
            user_data_free(user_data);
        user_data = NULL;
    }

    //=========================================================================================================
    /**
    * Create a MEG coil definition using a database of templates
    * Change the coordinate frame if so desired
    * Refactored: fwd_create_meg_coil (fwd_coil_def.c)
    *
    * @param[in] ch     Channel information to use
    * @param[in] acc    Required accuracy
    * @param[in] t      Transform the points using this
    *
    * @return   The created meg coil.
    */
    FwdCoil* create_meg_coil( FIFFLIB::fiffChInfo ch, int acc, FIFFLIB::fiffCoordTrans t);

    //=========================================================================================================
    /**
    * Create a MEG coil set definition using a database of templates
    * Change the coordinate frame if so desired
    * Refactored: fwd_create_meg_coils (fwd_coil_def.c)
    *
    * @param[in] ch     Channel information to use
    * @param[in] nch    Number of channels
    * @param[in] acc    Required accuracy
    * @param[in] t      Transform the points using this
    *
    * @return   The created meg coil set.
    */
    FwdCoilSet* create_meg_coils(FIFFLIB::fiffChInfo chs,  int nch, int acc, FIFFLIB::fiffCoordTrans t);


public:
    FwdCoil **coils;                 /* The coil or electrode positions */
    int     ncoil;
    int     coord_frame;            /* Common coordinate frame */
    void    *user_data;             /* We can put whatever in here */
    fwdUserFreeFunc user_data_free;

// ### OLD STRUCT ###
//    typedef struct {
//      fwdCoil *coils;		/* The coil or electrode positions */
//      int     ncoil;
//      int     coord_frame;		/* Common coordinate frame */
//      void    *user_data;		/* We can put whatever in here */
//      fwdUserFreeFunc user_data_free;
//    } *fwdCoilSet,fwdCoilSetRec;	/* A collection of the above */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE INVERSELIB

#endif // FWDCOILSET_H
