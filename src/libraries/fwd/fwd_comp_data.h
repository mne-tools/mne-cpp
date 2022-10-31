//=============================================================================================================
/**
 * @file     fwd_comp_data.h
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
 * @brief    FwdCompData class declaration.
 *
 */

#ifndef FWDCOMPDATA_H
#define FWDCOMPDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_types.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MneCTFCompDataSet;
}

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FWDLIB FORWARD DECLARATIONS
//=============================================================================================================

class FwdCoilSet;

//=============================================================================================================
/**
 * Implements the Forward Compensation Data description (Replaces *fwdCompData,fwdCompDataRec; struct of MNE-C fwd_comp_data.h).
 *
 * @brief This structure is used in the compensated field calculations
 */
class FWDSHARED_EXPORT FwdCompData
{
public:
    typedef QSharedPointer<FwdCompData> SPtr;              /**< Shared pointer type for FwdCompData. */
    typedef QSharedPointer<const FwdCompData> ConstSPtr;   /**< Const shared pointer type for FwdCompData. */

    //=========================================================================================================
    /**
     * Constructs the Forward Compensation Data
     * Refactored: fwd_new_comp_data (fwd_comp.c)
     */
    FwdCompData();

    //=========================================================================================================
    /**
     * Destroys the Forward Compensation Data
     * Refactored: fwd_free_comp_data (fwd_comp.c)
     */
    ~FwdCompData();

    //============================= fwd_comp.c =============================

    static int fwd_comp_field(float *rd,float *Q, FwdCoilSet* coils, float *res, void *client);

    /*
     * Routines to implement the reference channel compensation in field computations
     */

    static void fwd_free_comp_data(void *d);

    static int fwd_make_ctf_comp_coils(MNELIB::MneCTFCompDataSet* set,          /* The available compensation data */
                                       FwdCoilSet*        coils,        /* The main coil set */
                                       FwdCoilSet*        comp_coils);

    static FwdCompData* fwd_make_comp_data(MNELIB::MneCTFCompDataSet* set,           /* The CTF compensation data read from the file */
                                   FwdCoilSet*        coils,         /* The principal set of coils */
                                   FwdCoilSet*        comp_coils,    /* The compensation coils */
                                   fwdFieldFunc      field,	        /* The field computation functions */
                                   fwdVecFieldFunc   vec_field,
                                   fwdFieldGradFunc  field_grad,    /* The field and gradient computation function */
                                   void              *client,       /* Client data to be passed to the above */
                                   fwdUserFreeFunc   client_free);

    static int fwd_comp_field_vec(float *rd, FwdCoilSet* coils, float **res, void *client);

    static int fwd_comp_field_grad(float *rd,float *Q, FwdCoilSet* coils,
                float *res, float *xgrad, float *ygrad, float *zgrad,
                void *client);

public:
    MNELIB::MneCTFCompDataSet*  set;        /* The compensation data set */
    FwdCoilSet*         comp_coils; /* The compensation coil definitions */
    fwdFieldFunc        field;      /* Computes the field of given direction dipole */
    fwdVecFieldFunc     vec_field;  /* Computes the fields of all three dipole components  */
    fwdFieldGradFunc    field_grad; /* Computes the field and gradient of one dipole direction */
    void                *client;    /* Client data to pass to the above functions */
    fwdUserFreeFunc     client_free;
    float               *work;      /* The work areas */
    float               **vec_work;

// ### OLD STRUCT ###
//typedef struct {
//    FWDLIB::MneCTFCompDataSet* set; /* The compensation data set */
//    FWDLIB::FwdCoilSet* comp_coils; /* The compensation coil definitions */
//    fwdFieldFunc      field;            /* Computes the field of given direction dipole */
//    fwdVecFieldFunc   vec_field;        /* Computes the fields of all three dipole components  */
//    fwdFieldGradFunc  field_grad;       /* Computes the field and gradient of one dipole direction */
//    void              *client;          /* Client data to pass to the above functions */
//    fwdUserFreeFunc   client_free;
//    float             *work;            /* The work areas */
//    float             **vec_work;
//} *fwdCompData,fwdCompDataRec;          /* This structure is used in the compensated field calculations */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWDCOMPDATA_H
