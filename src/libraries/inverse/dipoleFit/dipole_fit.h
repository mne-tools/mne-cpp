//=============================================================================================================
/**
 * @file     dipole_fit.h
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

#ifndef DIPOLEFIT_H
#define DIPOLEFIT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "ecd_set.h"
#include "dipole_fit_settings.h"
#include "dipole_fit_data.h"
#include "../c/mne_meas_data.h"

#include <mne/c/mne_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//class DipoleFitData;
//class GuessData;
//class MneMeasData

//=============================================================================================================
/**
 * Implements all required dipole fitting routines
 *
 * @brief Dipole Fit implementation
 */
class INVERSESHARED_EXPORT DipoleFit
{
public:
    typedef QSharedPointer<DipoleFit> SPtr;             /**< Shared pointer type for DipoleFit. */
    typedef QSharedPointer<const DipoleFit> ConstSPtr;  /**< Const shared pointer type for DipoleFit. */

    //=========================================================================================================
    /**
     * Constructs Dipole Fit algorithm
     */
    explicit DipoleFit(DipoleFitSettings* p_settings);

    virtual ~DipoleFit(){}

    //ToDo split this function into init (with settings as parameter) and the actual fit function
    ECDSet calculateFit() const;
//    virtual const char* getName() const;

public:

    //=========================================================================================================
    /**
     *
     * Fit a single dipole to each time point of the data
     * Refactored: fit_dipoles (fit_dipoles.c)
     *
     * @param[in] dataname.
     * @param[in] data       The measured data.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Time range.
     * @param[in] tmax.
     * @param[in] tstep      Time step to use.
     * @param[in] integ      Integration time.
     * @param[in] verbose    Verbose output?.
     * @param[out] p_set     the fitted ECD Set.
     *
     * @return true when successful.
     */
    static int fit_dipoles( const QString& dataname, MneMeasData* data, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set);

    //=========================================================================================================
    /**
     *
     * Fit a single dipole to each time point of the data
     * Refactored: fit_dipoles_raw (fit_dipoles.c)
     *
     * @param[in] dataname.
     * @param[in] raw        The raw data description.
     * @param[in] sel        Channel selection to use.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Time range.
     * @param[in] tmax.
     * @param[in] tstep      Time step to use.
     * @param[in] integ      Integration time.
     * @param[in] verbose    Verbose output?.
     * @param[out] p_set     Return all results here. Warning: for large data files this may take a lot of memory.
     *
     * @return true when successful.
     */
    static int fit_dipoles_raw(const QString& dataname, MNELIB::MneRawData* raw, MNELIB::mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set);

    //=========================================================================================================
    /**
     *
     * Fit a single dipole to each time point of the data
     * Refactored: fit_dipoles_raw (fit_dipoles.c)
     *
     * @param[in] dataname.
     * @param[in] raw        The raw data description.
     * @param[in] sel        Channel selection to use.
     * @param[in] fit        Precomputed fitting data.
     * @param[in] guess      The initial guesses.
     * @param[in] tmin       Time range.
     * @param[in] tmax.
     * @param[in] tstep      Time step to use.
     * @param[in] integ      Integration time.
     * @param[in] verbose    Verbose output?.
     *
     * @return true when successful.
     */
    static int fit_dipoles_raw(const QString& dataname, MNELIB::MneRawData* raw, MNELIB::mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose);

private:
    DipoleFitSettings* settings;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // DIPOLEFIT_H
