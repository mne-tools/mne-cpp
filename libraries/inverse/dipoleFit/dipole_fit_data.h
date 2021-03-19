//=============================================================================================================
/**
 * @file     dipole_fit_data.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

#include <fiff/fiff_types.h>
#include "analyze_types.h"
#include <fwd/fwd_types.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_bem_model.h>
#include "dipole_forward.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

// ToDo move to cpp
#define COLUMN_NORM_NONE 0	    /* No column normalization requested */
#define COLUMN_NORM_COMP 1	    /* Componentwise normalization */
#define COLUMN_NORM_LOC  2	    /* Dipole locationwise normalization */

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

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

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GuessData;
class ECD;

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
     * Refactored: free_dipole_fit_data (dipole_fit_setup.c)
     */
    virtual ~DipoleFitData();

    //============================= dipole_fit_setup.c =============================

    static int setup_forward_model(DipoleFitData* d, MNELIB::MneCTFCompDataSet* comp_data, FWDLIB::FwdCoilSet* comp_coils);

    static MNELIB::MneCovMatrix* ad_hoc_noise(FWDLIB::FwdCoilSet* meg,          /* Channel name lists to define which channels are gradiometers */
                                     FWDLIB::FwdCoilSet* eeg,
                                     float      grad_std,
                                     float      mag_std,
                                     float      eeg_std);

    //ToDo  move to mneProjOp class
    static int make_projection(const QList<QString>& projnames,
                               const QList<FIFFLIB::FiffChInfo>& chs,
                               int        nch,
                               MNELIB::MneProjOp*  *res);

    static int scale_noise_cov(DipoleFitData* f,int nave);

    static int scale_dipole_fit_noise_cov(DipoleFitData* f,int nave);

    static int select_dipole_fit_noise_cov(DipoleFitData* f, mshMegEegData d);

    static DipoleFitData* setup_dipole_fit_data(   const QString& mriname,         /**< This gives the MRI/head transform. */
                                            const QString& measname,        /**< This gives the MEG/head transform and sensor locations. */
                                            const QString& bemname,         /**< BEM model. */
                                            Eigen::Vector3f *r0,            /**< Sphere model origin in head coordinates (optional). */
                                            FWDLIB::FwdEegSphereModel* eeg_model,   /**< EEG sphere model definition. */
                                            int   accurate_coils,           /**< Use accurate coil definitions?. */
                                            const QString& badname,         /**< Bad channels list. */
                                            const QString& noisename,               /**< Noise covariance matrix. */
                                            float grad_std,                 /**< Standard deviations for the ad-hoc noise cov (planar gradiometers). */
                                            float mag_std,                  /**< Ditto for magnetometers. */
                                            float eeg_std,                  /**< Ditto for EEG. */
                                            float mag_reg,                  /**< Noise-covariance regularization factors. */
                                            float grad_reg,
                                            float eeg_reg,
                                            int   diagnoise,                /**< Use only the diagonal elements of the noise-covariance matrix. */
                                            const QList<QString>& projnames,/**< SSP file names. */
                                            int   include_meg,              /**< Include MEG in the fitting?. */
                                            int   include_eeg);

    //=========================================================================================================
    /**
     * Fit a single dipole to the given data
     * Refactored: fit_one (fit_dipoles.c)
     *
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] time       Which time is it?.
     * @param[in] B          The field to fit.
     * @param[in] verbose.
     * @param[in] res        The fitted dipole.
     */
    static bool fit_one(DipoleFitData* fit, GuessData* guess, float time, float *B, int verbose, ECD& res);

//============================= dipole_forward.c

    static int compute_dipole_field(DipoleFitData* d, float *rd, int whiten, float **fwd);

    //============================= dipole_forward.c

    static DipoleForward* dipole_forward_one(DipoleFitData* d,
                                     float         *rd,
                                     DipoleForward* old);

public:
      FIFFLIB::FiffCoordTransOld*    mri_head_t; /**< MRI <-> head coordinate transformation. */
      FIFFLIB::FiffCoordTransOld*    meg_head_t; /**< MEG <-> head coordinate transformation. */
      int               coord_frame;        /**< Common coordinate frame. */
      QList<FIFFLIB::FiffChInfo>        chs;       /**< Channels. */
      int               nmeg;               /**< How many MEG. */
      int               neeg;               /**< How many EEG. */
      QStringList       ch_names;           /**< List of all channel names. */
      FIFFLIB::FiffSparseMatrix* pick;   /**< Matrix to pick data from the full data set which may contain channels we are not interested in. */
      FWDLIB::FwdCoilSet*        meg_coils;         /**< MEG coil definitions. */
      FWDLIB::FwdCoilSet*        eeg_els;           /**< EEG electrode definitions. */
      float             r0[3];              /**< Sphere model origin. */
      QString           bemname;           /**< Using a BEM?. */

      FWDLIB::FwdEegSphereModel *eeg_model;         /**< EEG sphere model definition. */
      FWDLIB::FwdBemModel       *bem_model;         /**< BEM model definition. */

      dipoleFitFuncs    sphere_funcs;       /**< These are the sphere model forward functions. */
      dipoleFitFuncs    bem_funcs;          /**< These are the BEM forward functions. */
      dipoleFitFuncs    funcs;              /**< Points to one of the two above. */
      dipoleFitFuncs    mag_dipole_funcs;   /**< Functions to fit a magnetic dipole. */

      int               fixed_noise;        /**< Were fixed noise values used rather than a noise-covariance matrix read from a file. */
      MNELIB::MneCovMatrix*      noise_orig;         /**< Noise covariance matrix (original). */
      MNELIB::MneCovMatrix*      noise;              /**< Noise covariance matrix (weighted to take the selection into account). */
      int               nave;               /**< How many averages does this correspond to?. */
      MNELIB::MneProjOp*        proj;               /**< The projection operator to use. */
      int               column_norm;        /**< What kind of column normalization to apply to the forward solution. */
      int               fit_mag_dipoles;    /**< Fit magnetic dipoles?. */
      void              *user;              /**< User data for anything we need. */
      fitUserFreeFunc   user_free;          /**< Function to free the above. */

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
//      MneCovMatrix*      noise_orig;	      /* Noise covariance matrix (original) */
//      MneCovMatrix*      noise;	      /* Noise covariance matrix (weighted to take the selection into account) */
//      int               nave;	      /* How many averages does this correspond to? */
//      mneProjOp         proj;	      /* The projection operator to use */
//      int               column_norm;      /* What kind of column normalization to apply to the forward solution */
//      int               fit_mag_dipoles;  /* Fit magnetic dipoles? */
//      void              *user;	      /* User data for anything we need */
//      fitUserFreeFunc   user_free;	      /* Function to free the above */
//    } *dipoleFitData,dipoleFitDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // DIPOLEFITDATA_H
