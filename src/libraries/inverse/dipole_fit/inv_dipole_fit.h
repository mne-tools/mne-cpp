//=============================================================================================================
/**
 * @file     inv_dipole_fit.h
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
 * @brief    Dipole Fit class declaration.
 *
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
#include "../inv_meas_data.h"

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
    static bool fit_dipoles(const QString& dataname, INVLIB::InvMeasData* data, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set);

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
