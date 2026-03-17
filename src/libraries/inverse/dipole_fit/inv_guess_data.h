//=============================================================================================================
/**
 * @file     inv_guess_data.h
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
 * @brief    InvGuessData class declaration.
 *
 */

#ifndef INV_GUESS_DATA_H
#define INV_GUESS_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_dipole_forward.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class InvDipoleFitData;

//=============================================================================================================
/**
 * Implements InvGuessData (Replaces *guessData,guessDataRec struct of MNE-C fit_types.h).
 *
 * @brief Precomputed guess point grid with forward fields for initial dipole position candidates.
 */
class INVSHARED_EXPORT InvGuessData
{
public:
    typedef QSharedPointer<InvGuessData> SPtr;              /**< Shared pointer type for InvGuessData. */
    typedef QSharedPointer<const InvGuessData> ConstSPtr;   /**< Const shared pointer type for InvGuessData. */

    //=========================================================================================================
    /**
     * Constructs the Guess Data
     * Refactored: new_guess_data (dipole_fit_setup.c)
     */
    InvGuessData();

    /** Deleted — non-copyable due to unique_ptr members. */
    InvGuessData(const InvGuessData&) = delete;
    InvGuessData& operator=(const InvGuessData&) = delete;

    //=========================================================================================================
    /**
     * Constructs the Guess Data from given Data
     * Refactored: make_guess_data (setup.c)
     *
     * @param[in] guessname.
     *
     */
    InvGuessData( const QString& guessname, const QString& guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData* f);

    //=========================================================================================================
    /**
     * Constructs the Guess Data from given Data
     * Refactored: make_guess_data (dipole_fit_setup.c)
     *
     * @param[in] guessname.
     *
     */
    InvGuessData( const QString& guessname, const QString& guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData* f, char *guess_save_name);

    //=========================================================================================================
    /**
     * Destroys the Guess Data description
     * Refactored: free_guess_data (dipole_fit_setup.c)
     */
    ~InvGuessData();

    //=========================================================================================================
    /**
     * Once the guess locations have been set up we can compute the fields
     * Refactored: compute_guess_fields (dipole_fit_setup.c)
     *
     * @param[in] f      Dipole Fit Data to the Compute Guess Fields.
     *
     * @return true when successful.
     */
    bool compute_guess_fields(InvDipoleFitData* f);

public:
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> rr; /**< Guess dipole locations (nguess x 3, row-major). */
    std::vector<InvDipoleForward::UPtr> guess_fwd; /**< Forward solutions for the guesses. */
    int            nguess;          /**< How many sources. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVLIB

#endif // INV_GUESS_DATA_H
