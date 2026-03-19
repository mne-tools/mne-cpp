//=============================================================================================================
/**
 * @file     inv_dipole_fit_data.h
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

#ifndef INV_DIPOLE_FIT_DATA_H
#define INV_DIPOLE_FIT_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

#include <fiff/fiff_types.h>
#include <fwd/fwd_types.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_bem_model.h>
#include "inv_dipole_forward.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <memory>

namespace FIFFLIB { class FiffCoordTrans; }
namespace MNELIB { class MNEMeasData; }

constexpr int COLUMN_NORM_NONE = 0;     /**< No column normalization requested. */
constexpr int COLUMN_NORM_COMP = 1;     /**< Componentwise normalization. */
constexpr int COLUMN_NORM_LOC  = 2;     /**< Dipole locationwise normalization. */

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

// (Replaces *dipoleFitFuncs,dipoleFitFuncsRec struct of MNE-C fit_types.h).

/**
 * @brief Forward field computation function pointers and client data for MEG and EEG dipole fitting.
 */
struct dipoleFitFuncsRec {
  ~dipoleFitFuncsRec() {
      if (meg_client_free && meg_client)
          meg_client_free(meg_client);
      if (eeg_client_free && eeg_client)
          eeg_client_free(eeg_client);
  }
  fwdFieldFunc    meg_field = nullptr;       /**< MEG forward calculation function. */
  fwdVecFieldFunc meg_vec_field = nullptr;   /**< MEG vectorized forward calculation function. */
  void            *meg_client = nullptr;     /**< Client data for MEG field computations. */
  MNELIB::mneUserFreeFunc meg_client_free = nullptr;  /**< Destructor for MEG client data. */

  fwdFieldFunc    eeg_pot = nullptr;         /**< EEG forward calculation function. */
  fwdVecFieldFunc eeg_vec_pot = nullptr;     /**< EEG vectorized forward calculation function. */
  void            *eeg_client = nullptr;     /**< Client data for EEG field computations. */
  MNELIB::mneUserFreeFunc eeg_client_free = nullptr;  /**< Destructor for EEG client data. */
};

/**
 * @brief Pointer alias for dipoleFitFuncsRec, used throughout the dipole fitting module.
 */
using dipoleFitFuncs = dipoleFitFuncsRec*;

/**
 * @brief Workspace for the dipole fitting objective function, holding forward model, measured field, and fit limits.
 */
struct FitDipUserRec {
    float          limit;
    int            report_dim;
    float          *B;
    double         B2;
    InvDipoleForward*  fwd;
};

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class InvGuessData;
class InvEcd;

//=============================================================================================================
/**
 * @brief Dipole fit workspace holding sensor geometry, forward model, noise
 *        covariance, and projection data.
 *
 * InvDipoleFitData aggregates everything needed to evaluate the dipole
 * fitting objective function: coordinate transforms, channel lists, coil/
 * electrode definitions, forward-model function pointers (sphere or BEM),
 * noise covariance, and SSP projection.  The static factory
 * setup_dipole_fit_data() reads all inputs from disk and returns a
 * fully initialised instance.
 *
 * Refactored from dipoleFitDataRec / fit_types.h (MNE-C).
 */
class INVSHARED_EXPORT InvDipoleFitData
{
public:
    typedef QSharedPointer<InvDipoleFitData> SPtr;             /**< Shared pointer type for InvDipoleFitData. */
    typedef QSharedPointer<const InvDipoleFitData> ConstSPtr;  /**< Const shared pointer type for InvDipoleFitData. */

    //=========================================================================================================
    /**
     * Default Constructor
     */
    explicit InvDipoleFitData();

    //=========================================================================================================
    /**
     * Destructs the Dipole Fit Data
     * Refactored: free_dipole_fit_data (dipole_fit_setup.c)
     */
    virtual ~InvDipoleFitData();

    //=========================================================================================================
    /**
     * @brief Set up the sphere-model and (optionally) BEM forward functions.
     *
     * Initialises sphere_funcs, bem_funcs, and mag_dipole_funcs, applies
     * CTF compensation if needed, and selects the active forward model.
     *
     * Refactored: setup_forward_model (dipole_fit_setup.c)
     *
     * @param[in,out] d           Dipole fit data to populate.
     * @param[in]     comp_data   CTF compensation data (may be nullptr).
     * @param[in]     comp_coils  Compensation coil set (may be nullptr).
     *
     * @return OK on success, FAIL on error.
     */
    static int setup_forward_model(InvDipoleFitData* d, MNELIB::MNECTFCompDataSet* comp_data, FWDLIB::FwdCoilSet* comp_coils);

    //=========================================================================================================
    /**
     * @brief Create an ad-hoc diagonal noise-covariance matrix.
     *
     * Builds a diagonal noise covariance from fixed standard deviations
     * for gradiometers, magnetometers, and EEG channels.
     *
     * Refactored: ad_hoc_noise (dipole_fit_setup.c)
     *
     * @param[in] meg       MEG coil set (used to classify grad vs. mag channels).
     * @param[in] eeg       EEG electrode set.
     * @param[in] grad_std  Standard deviation for planar gradiometers (T/m).
     * @param[in] mag_std   Standard deviation for magnetometers (T).
     * @param[in] eeg_std   Standard deviation for EEG channels (V).
     *
     * @return The noise-covariance matrix, or nullptr on error.
     */
    static std::unique_ptr<MNELIB::MNECovMatrix> ad_hoc_noise(FWDLIB::FwdCoilSet* meg,
                                     FWDLIB::FwdCoilSet* eeg,
                                     float      grad_std,
                                     float      mag_std,
                                     float      eeg_std);

    //=========================================================================================================
    /**
     * @brief Scale the noise-covariance matrix for a given number of averages.
     *
     * Re-decomposes the covariance after scaling by nave_old / nave_new.
     *
     * Refactored: scale_noise_cov (dipole_fit_setup.c)
     *
     * @param[in,out] f     Dipole fit data whose noise member is scaled.
     * @param[in]     nave  Number of averages in the current data.
     *
     * @return OK on success, FAIL on error.
     */
    static int scale_noise_cov(InvDipoleFitData* f, int nave);

    //=========================================================================================================
    /**
     * @brief Scale dipole-fit noise covariance for a given number of averages.
     *
     * Wrapper around scale_noise_cov() called from the dipole-fit pipeline.
     *
     * Refactored: scale_dipole_fit_noise_cov (dipole_fit_setup.c)
     *
     * @param[in,out] f     Dipole fit data whose noise member is scaled.
     * @param[in]     nave  Number of averages.
     *
     * @return OK on success, FAIL on error.
     */
    static int scale_dipole_fit_noise_cov(InvDipoleFitData* f, int nave);

    //=========================================================================================================
    /**
     * @brief Select and weight the noise-covariance for the active channel set.
     *
     * Channels present in the measurement data selection receive unit
     * weight; omitted channels receive a reduced weight.
     *
     * When @p meas is nullptr the function simply scales the noise
     * covariance for nave = 1 (initial setup case).
     *
     * Refactored: select_dipole_fit_noise_cov (dipole_fit_setup.c)
     *
     * @param[in,out] f     Dipole fit data whose noise_orig is used to
     *                       build the weighted noise member.
     * @param[in]     meas  Measurement data with channel info (may be nullptr).
     * @param[in]     nave  Number of averages override (< 0 to use meas->current->nave).
     * @param[in]     sels  Per-channel selection flags (size >= meas->nchan, may be nullptr).
     *
     * @return OK on success, FAIL on error.
     */
    static int select_dipole_fit_noise_cov(InvDipoleFitData* f,
                                           MNELIB::MNEMeasData* meas,
                                           int nave,
                                           const int* sels);

    //=========================================================================================================
    /**
     * @brief Master setup: read all inputs and build a ready-to-use fit workspace.
     *
     * Reads coordinate transforms, channel info, BEM/sphere model, noise
     * covariance, SSP projections, and compiles the forward model.
     *
     * Refactored: setup_dipole_fit_data (dipole_fit_setup.c)
     *
     * @param[in] mriname         MRI-to-head transform file.
     * @param[in] measname        Measurement file (provides MEG-to-head transform and channels).
     * @param[in] bemname         BEM model file (empty string to use sphere model only).
     * @param[in] r0              Sphere-model origin in head coordinates (may be nullptr).
     * @param[in] eeg_model       EEG sphere model definition (may be nullptr).
     * @param[in] accurate_coils  Use accurate coil definitions.
     * @param[in] badname         Bad-channel list file (empty to skip).
     * @param[in] noisename       Noise-covariance file (empty for ad-hoc noise).
     * @param[in] grad_std        Ad-hoc noise std for planar gradiometers (T/m).
     * @param[in] mag_std         Ad-hoc noise std for magnetometers (T).
     * @param[in] eeg_std         Ad-hoc noise std for EEG channels (V).
     * @param[in] mag_reg         Magnetometer noise-covariance regularization factor.
     * @param[in] grad_reg        Gradiometer noise-covariance regularization factor.
     * @param[in] eeg_reg         EEG noise-covariance regularization factor.
     * @param[in] diagnoise       Use only the diagonal of the noise covariance.
     * @param[in] projnames       SSP projection file paths.
     * @param[in] include_meg     Include MEG channels in the fit.
     * @param[in] include_eeg     Include EEG channels in the fit.
     *
     * @return Fully initialised fit data, or nullptr on error. Caller takes ownership.
     */
    static InvDipoleFitData* setup_dipole_fit_data(
                                            const QString& mriname,
                                            const QString& measname,
                                            const QString& bemname,
                                            Eigen::Vector3f *r0,
                                            FWDLIB::FwdEegSphereModel* eeg_model,
                                            int   accurate_coils,
                                            const QString& badname,
                                            const QString& noisename,
                                            float grad_std,
                                            float mag_std,
                                            float eeg_std,
                                            float mag_reg,
                                            float grad_reg,
                                            float eeg_reg,
                                            int   diagnoise,
                                            const QList<QString>& projnames,
                                            int   include_meg,
                                            int   include_eeg);

    //=========================================================================================================
    /**
     * @brief Fit a single dipole to the given data.
     *
     * Refactored: fit_one (fit_dipoles.c)
     *
     * @param[in]     fit        Precomputed fitting data.
     * @param[in]     guess      The initial guesses.
     * @param[in]     time       Time point (s).
     * @param[in,out] B          The field to fit (modified in-place by projection and whitening).
     * @param[in]     verbose    Verbose output flag.
     * @param[out]    res        The fitted dipole.
     *
     * @return true on success, false on fitting failure.
     */
    static bool fit_one(InvDipoleFitData* fit, InvGuessData* guess, float time, Eigen::Ref<Eigen::VectorXf> B, int verbose, InvEcd& res);

    //=========================================================================================================
    /**
     * @brief Compute the forward field for a dipole at the given location.
     *
     * Evaluates the MEG and/or EEG forward model at position @p rd and
     * optionally applies noise whitening.
     *
     * Refactored: compute_dipole_field (fit_dipoles.c)
     *
     * @param[in]     d        Dipole fit workspace.
     * @param[in]     rd       Dipole position in head coordinates (m).
     * @param[in]     whiten   If non-zero, whiten the result using the noise covariance.
     * @param[in,out] fwd      Forward field matrix (nchan x 3), filled on output.
     *
     * @return OK on success, FAIL on error.
     */
    static int compute_dipole_field(InvDipoleFitData& d, const Eigen::Vector3f& rd, int whiten, Eigen::Ref<Eigen::MatrixXf> fwd);

    //=========================================================================================================
    /**
     * @brief Compute the forward solution for a single dipole position.
     *
     * Returns a fully initialised InvDipoleForward with the forward
     * field, its SVD, and noise-normalised goodness-of-fit limit.  An
     * existing object may be recycled via @p old.
     *
     * Refactored: dipole_forward_one (fit_dipoles.c)
     *
     * @param[in]     d    Dipole fit workspace.
     * @param[in]     rd   Dipole position in head coordinates (m).
     * @param[in,out] old  Existing forward to recycle (may be nullptr).
     *
     * @return The populated forward object, or nullptr on error.
     */
    static InvDipoleForward* dipole_forward_one(InvDipoleFitData* d,
                                     const Eigen::Vector3f& rd,
                                     InvDipoleForward* old);

public:
      std::unique_ptr<FIFFLIB::FiffCoordTrans>    mri_head_t; /**< MRI <-> head coordinate transformation. */
      std::unique_ptr<FIFFLIB::FiffCoordTrans>    meg_head_t; /**< MEG <-> head coordinate transformation. */
      int               coord_frame;        /**< Common coordinate frame. */
      QList<FIFFLIB::FiffChInfo>        chs;       /**< Channels. */
      int               nmeg;               /**< How many MEG. */
      int               neeg;               /**< How many EEG. */
      QStringList       ch_names;           /**< List of all channel names. */
      std::unique_ptr<FIFFLIB::FiffSparseMatrix> pick;   /**< Matrix to pick data from the full data set which may contain channels we are not interested in (currently unused). */
      std::unique_ptr<FWDLIB::FwdCoilSet>        meg_coils;         /**< MEG coil definitions. */
      std::unique_ptr<FWDLIB::FwdCoilSet>        eeg_els;           /**< EEG electrode definitions. */
      Eigen::Vector3f     r0;                /**< Sphere model origin. */
      QString           bemname;           /**< Using a BEM?. */

      std::unique_ptr<FWDLIB::FwdEegSphereModel> eeg_model;         /**< EEG sphere model definition. */
      std::unique_ptr<FWDLIB::FwdBemModel>       bem_model;         /**< BEM model definition. */

      std::unique_ptr<dipoleFitFuncsRec>    sphere_funcs;       /**< These are the sphere model forward functions. */
      std::unique_ptr<dipoleFitFuncsRec>    bem_funcs;          /**< These are the BEM forward functions. */
      dipoleFitFuncsRec*    funcs = nullptr;    /**< Non-owning alias — points to one of the two above. */
      std::unique_ptr<dipoleFitFuncsRec>    mag_dipole_funcs;   /**< Functions to fit a magnetic dipole. */

      int               fixed_noise;        /**< Were fixed noise values used rather than a noise-covariance matrix read from a file. */
      std::unique_ptr<MNELIB::MNECovMatrix>      noise_orig;         /**< Noise covariance matrix (original, currently unused). */
      std::unique_ptr<MNELIB::MNECovMatrix>      noise;              /**< Noise covariance matrix (weighted to take the selection into account). */
      int               nave;               /**< How many averages does this correspond to?. */
      std::unique_ptr<MNELIB::MNEProjOp>        proj;               /**< The projection operator to use. */
      int               column_norm;        /**< What kind of column normalization to apply to the forward solution. */
      int               fit_mag_dipoles;    /**< Fit magnetic dipoles?. */
      FitDipUserRec     *user;              /**< Non-owning pointer to dipole fit workspace (set during fit_one). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // INV_DIPOLE_FIT_DATA_H
