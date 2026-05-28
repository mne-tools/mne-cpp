//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_guess_data.h
 * @since March 2026
 * @brief Initial-guess grid for the dipole-fit optimiser, with per-guess forward fields pre-computed.
 *
 * @ref INVLIB::InvGuessData generates and stores the regular grid of
 * candidate dipole positions that seeds the Nelder-Mead optimiser used
 * by @ref InvDipoleFit. For every guess location the corresponding
 * forward-field SVD is pre-computed into an @ref InvDipoleForward, so
 * the per-iteration evaluation cost reduces to a dense matmul. Replaces
 * the @c guessDataRec record of MNE-C and is built either from a
 * user-supplied guess source space or directly from an inner-skull BEM
 * surface inflated to a regular @c (mindist, exclude, grid) lattice.
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
