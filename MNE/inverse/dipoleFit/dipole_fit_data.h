//=============================================================================================================
/**
* @file     dipole_fit_data.h
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
* @brief    Dipole Fit Data class declaration.
*
*/

#ifndef DIPOLEFITDATA_H
#define DIPOLEFITDATA_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

#include "fwd_types.h"
#include "fwd_eeg_sphere_model.h"
#include "dipole_forward.h"


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

// ToDo move to cpp
#define COLUMN_NORM_NONE 0	    /* No column normalization requested */
#define COLUMN_NORM_COMP 1	    /* Componentwise normalization */
#define COLUMN_NORM_LOC  2	    /* Dipole locationwise normalization */


/*
 * These are the type definitions for dipole fitting
 */
typedef void (*fitUserFreeFunc)(void *);

// (Replaces *dipoleFitFuncs,dipoleFitFuncsRec struct of MNE-C fit_types.h).

typedef struct {
  fwdFieldFunc    meg_field;	    /* MEG forward calculation functions */
  fwdVecFieldFunc meg_vec_field;
  void            *meg_client;	    /* Client data for MEG field computations */
  mneUserFreeFunc meg_client_free;

  fwdFieldFunc    eeg_pot;	    /* EEG forward calculation functions */
  fwdVecFieldFunc eeg_vec_pot;
  void            *eeg_client;	    /* Client data for EEG field computations */
  mneUserFreeFunc eeg_client_free;
} *dipoleFitFuncs,dipoleFitFuncsRec;


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
* Implements the dipole fit data parser (Replaces *dipoleFitData,dipoleFitDataRec struct of MNE-C fit_types.h).
*
* @brief Dipole Fit Data implementation
*/
class INVERSESHARED_EXPORT DipoleFitData
{
public:
    typedef QSharedPointer<DipoleFitData> SPtr;             /**< Shared pointer type for DipoleFitData. */
    typedef QSharedPointer<const DipoleFitData> ConstSPtr;  /**< Const shared pointer type for DipoleFitData. */

    //=========================================================================================================
    /**
    * Default Constructor
    */
    explicit DipoleFitData();

    //=========================================================================================================
    /**
    * Destructs the Dipole Fit Data
    */
    virtual ~DipoleFitData();

public:
      fiffCoordTrans    mri_head_t;         /**< MRI <-> head coordinate transformation */
      fiffCoordTrans    meg_head_t;         /**< MEG <-> head coordinate transformation */
      int               coord_frame;        /**< Common coordinate frame */
      fiffChInfo        chs;                /**< Channels */
      int               nmeg;               /**< How many MEG */
      int               neeg;               /**< How many EEG */
      char              **ch_names;         /**< List of all channel names */
      mneSparseMatrix   pick;               /**< Matrix to pick data from the full data set which may contain channels we are not interested in */
      FwdCoilSet*        meg_coils;          /**< MEG coil definitions */
      FwdCoilSet*        eeg_els;            /**< EEG electrode definitions */
      float             r0[3];              /**< Sphere model origin */
      char              *bemname;           /**< Using a BEM? */

      FwdEegSphereModel *eeg_model;         /**< EEG sphere model definition */
      fwdBemModel       bem_model;          /**< BEM model definition */

      dipoleFitFuncs    sphere_funcs;       /**< These are the sphere model forward functions */
      dipoleFitFuncs    bem_funcs;          /**< These are the BEM forward functions */
      dipoleFitFuncs    funcs;              /**< Points to one of the two above */
      dipoleFitFuncs    mag_dipole_funcs;   /**< Functions to fit a magnetic dipole */

      int               fixed_noise;        /**< Were fixed noise values used rather than a noise-covariance matrix read from a file */
      mneCovMatrix      noise_orig;         /**< Noise covariance matrix (original) */
      mneCovMatrix      noise;              /**< Noise covariance matrix (weighted to take the selection into account) */
      int               nave;               /**< How many averages does this correspond to? */
      mneProjOp         proj;               /**< The projection operator to use */
      int               column_norm;        /**< What kind of column normalization to apply to the forward solution */
      int               fit_mag_dipoles;    /**< Fit magnetic dipoles? */
      void              *user;              /**< User data for anything we need */
      fitUserFreeFunc   user_free;          /**< Function to free the above */

// ### OLD STRUCT ###
//    typedef struct {		      /* This structure holds all fitting-related data */
//      fiffCoordTrans    mri_head_t;	      /* MRI <-> head coordinate transformation */
//      fiffCoordTrans    meg_head_t;	      /* MEG <-> head coordinate transformation */
//      int               coord_frame;      /* Common coordinate frame */
//      fiffChInfo        chs;              /* Channels */
//      int               nmeg;	      /* How many MEG */
//      int               neeg;	      /* How many EEG */
//      char              **ch_names;	      /* List of all channel names */
//      mneSparseMatrix   pick;	      /* Matrix to pick data from the
//                         full data set which may contain channels
//                         we are not interested in */
//      fwdCoilSet        meg_coils;	      /* MEG coil definitions */
//      fwdCoilSet        eeg_els;	      /* EEG electrode definitions */
//      float             r0[3];	      /* Sphere model origin */
//      char              *bemname;	      /* Using a BEM? */

//      FwdEegSphereModel *eeg_model;	      /* EEG sphere model definition */
//      fwdBemModel       bem_model;	      /* BEM model definition */

//      dipoleFitFuncs    sphere_funcs;     /* These are the sphere model forward functions */
//      dipoleFitFuncs    bem_funcs;	      /* These are the BEM forward functions */
//      dipoleFitFuncs    funcs;	      /* Points to one of the two above */
//      dipoleFitFuncs    mag_dipole_funcs; /* Functions to fit a magnetic dipole */

//      int               fixed_noise;      /* Were fixed noise values used rather than a noise-covariance
//                           * matrix read from a file */
//      mneCovMatrix      noise_orig;	      /* Noise covariance matrix (original) */
//      mneCovMatrix      noise;	      /* Noise covariance matrix (weighted to take the selection into account) */
//      int               nave;	      /* How many averages does this correspond to? */
//      mneProjOp         proj;	      /* The projection operator to use */
//      int               column_norm;      /* What kind of column normalization to apply to the forward solution */
//      int               fit_mag_dipoles;  /* Fit magnetic dipoles? */
//      void              *user;	      /* User data for anything we need */
//      fitUserFreeFunc   user_free;	      /* Function to free the above */
//    } *dipoleFitData,dipoleFitDataRec;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // DIPOLEFITDATA_H
