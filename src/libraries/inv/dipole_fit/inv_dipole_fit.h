//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_dipole_fit.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    High-level driver for sequential equivalent-current-dipole (ECD) fitting at every time point of an evoked or raw recording.
 *
 * @ref INVLIB::InvDipoleFit orchestrates the full mne-c
 * @c mne_dipole_fit pipeline: it consumes a populated
 * @ref InvDipoleFitSettings, builds the forward-model workspace via
 * @ref InvDipoleFitData, generates the initial-guess grid via
 * @ref InvGuessData, and then fits one ECD per requested time bin
 * returning the result as an @ref InvEcdSet. Refactored from
 * @c fit_dipoles.c / @c dipole_fit_setup.c (MNE-C) while preserving the
 * sphere-model and BEM forward-model code paths used by the original
 * mne-c tooling.
 */

#ifndef INV_DIPOLE_FIT_H
#define INV_DIPOLE_FIT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_ecd_set.h"
#include "inv_dipole_fit_settings.h"
#include "inv_dipole_fit_data.h"
#include <mne/mne_meas_data.h>

#include <mne/mne_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief High-level driver for sequential dipole fitting.
 *
 * InvDipoleFit orchestrates the complete dipole-fit pipeline: it sets up
 * the forward model via InvDipoleFitData, computes initial guess grids
 * via InvGuessData, reads averaged or raw data, and fits an equivalent
 * current dipole (ECD) at each requested time point.
 *
 * Refactored from fit_dipoles.c / dipole_fit_setup.c (MNE-C).
 */
class INVSHARED_EXPORT InvDipoleFit
{
public:
    typedef QSharedPointer<InvDipoleFit> SPtr;             /**< Shared pointer type for InvDipoleFit. */
    typedef QSharedPointer<const InvDipoleFit> ConstSPtr;  /**< Const shared pointer type for InvDipoleFit. */

    //=========================================================================================================
    /**
     * Constructs Dipole Fit algorithm.
     *
     * @param[in] p_settings     The dipole fit settings.
     */
    explicit InvDipoleFit(InvDipoleFitSettings* p_settings);

    //=========================================================================================================
    /**
     * Destructs the Dipole Fit.
     */
    virtual ~InvDipoleFit() = default;

    //=========================================================================================================
    /**
     * Execute the dipole fit using the stored settings.
     *
     * @return The fitted dipole set.
     */
    InvEcdSet calculateFit() const;

public:

    //=========================================================================================================
    /**
     * Fit a single dipole to each time point of averaged data.
     *
     * Refactored: fit_dipoles (fit_dipoles.c)
     *
     * @param[in] dataname   Data file name.
     * @param[in] data       The measured data.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Fit start time (s).
     * @param[in] tmax       Fit end time (s).
     * @param[in] tstep      Time step to use (s).
     * @param[in] integ      Integration time (s).
     * @param[in] verbose    Verbose output.
     * @param[out] p_set     The fitted dipole set.
     *
     * @return true when successful.
     */
    static bool fit_dipoles(const QString& dataname, MNELIB::MNEMeasData* data, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set);

    //=========================================================================================================
    /**
     * Fit a single dipole to each time point of raw data.
     *
     * Reads data in overlapping segments of @c SEG_LEN seconds and
     * extracts values at each requested time point.
     *
     * Refactored: fit_dipoles_raw (fit_dipoles.c)
     *
     * @param[in] dataname   Data file name.
     * @param[in] raw        The raw data description.
     * @param[in] sel        Channel selection to use.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Fit start time (s).
     * @param[in] tmax       Fit end time (s).
     * @param[in] tstep      Time step to use (s).
     * @param[in] integ      Integration time (s).
     * @param[in] verbose    Verbose output.
     * @param[out] p_set     The fitted dipole set.
     *
     * @return true when successful.
     */
    static bool fit_dipoles_raw(const QString& dataname, MNELIB::MNERawData* raw, MNELIB::mneChSelection sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set);

    //=========================================================================================================
    /**
     * Fit dipoles to raw data (convenience overload without output set).
     *
     * Refactored: fit_dipoles_raw (fit_dipoles.c)
     *
     * @param[in] dataname   Data file name.
     * @param[in] raw        The raw data description.
     * @param[in] sel        Channel selection to use.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Fit start time (s).
     * @param[in] tmax       Fit end time (s).
     * @param[in] tstep      Time step to use (s).
     * @param[in] integ      Integration time (s).
     * @param[in] verbose    Verbose output.
     *
     * @return true when successful.
     */
    static bool fit_dipoles_raw(const QString& dataname, MNELIB::MNERawData* raw, MNELIB::mneChSelection sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose);

private:
    InvDipoleFitSettings* settings;     /**< Non-owning pointer to the dipole fit settings. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // INV_DIPOLE_FIT_H
