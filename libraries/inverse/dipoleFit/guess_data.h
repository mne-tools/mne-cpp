//=============================================================================================================
/**
 * @file     guess_data.h
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
 * @brief    GuessData class declaration.
 *
 */

#ifndef GUESSDATA_H
#define GUESSDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "dipole_forward.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

class DipoleFitData;

//=============================================================================================================
/**
 * Implements GuessData (Replaces *guessData,guessDataRec struct of MNE-C fit_types.h).
 *
 * @brief GuessData description
 */
class INVERSESHARED_EXPORT GuessData
{
public:
    typedef QSharedPointer<GuessData> SPtr;              /**< Shared pointer type for GuessData. */
    typedef QSharedPointer<const GuessData> ConstSPtr;   /**< Const shared pointer type for GuessData. */

    //=========================================================================================================
    /**
     * Constructs the Guess Data
     * Refactored: new_guess_data (dipole_fit_setup.c)
     */
    GuessData();

//    //=========================================================================================================
//    /**
//    * Copy constructor.
//    *
//    * @param[in] p_GuessData    GuessData which should be copied
//    */
//    GuessData(const GuessData& p_GuessData);

    //=========================================================================================================
    /**
     * Constructs the Guess Data from given Data
     * Refactored: make_guess_data (setup.c)
     *
     * @param[in] guessname.
     *
     */
    GuessData( const QString& guessname, const QString& guess_surfname, float mindist, float exclude, float grid, DipoleFitData* f);

    //=========================================================================================================
    /**
     * Constructs the Guess Data from given Data
     * Refactored: make_guess_data (dipole_fit_setup.c)
     *
     * @param[in] guessname.
     *
     */
    GuessData( const QString& guessname, const QString& guess_surfname, float mindist, float exclude, float grid, DipoleFitData* f, char *guess_save_name);

    //=========================================================================================================
    /**
     * Destroys the Guess Data description
     * Refactored: free_guess_data (dipole_fit_setup.c)
     */
    ~GuessData();

    //=========================================================================================================
    /**
     * Once the guess locations have been set up we can compute the fields
     * Refactored: compute_guess_fields (dipole_fit_setup.c)
     *
     * @param[in] f      Dipole Fit Data to the Compute Guess Fields.
     *
     * @return true when successful.
     */
    bool compute_guess_fields(DipoleFitData* f);

public:
    float          **rr;            /**< These are the guess dipole locations. */
    DipoleForward** guess_fwd;      /**< Forward solutions for the guesses. */
    int            nguess;          /**< How many sources. */

// ### OLD STRUCT ###
//    typedef struct {
//        float          **rr;                    /**< These are the guess dipole locations. */
//        DipoleForward** guess_fwd;              /**< Forward solutions for the guesses. */
//        int            nguess;                  /**< How many sources. */
//    } *guessData,guessDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVERSELIB

#endif // GUESSDATA_H
