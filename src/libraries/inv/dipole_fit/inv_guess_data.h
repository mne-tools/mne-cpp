//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     inv_guess_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December 2016
 * @brief    Initial-guess grid for the dipole-fit optimiser, with per-guess forward fields pre-computed.
 *
 * @ref mne::InvGuessData generates and stores the regular grid of
 * candidate dipole positions that seeds the Nelder-Mead optimiser used
 * by @ref mne::InvDipoleFit. For every guess location the corresponding
 * forward-field SVD is pre-computed into an @ref mne::InvDipoleForward, so
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
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <filesystem>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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
 *
 * @snippet ex_inv_dipole_fit/main.cpp inv_dipole_fit_data_usage
 */
class INVSHARED_EXPORT InvGuessData
{
public:
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
     * Refactored: make_guess_data (dipole_fit_setup.c)
     *
     * @param[in] guessname File with a predefined guess point set (empty = build a grid).
     * @param[in] guess_surfname Surface file bounding the guess grid (empty = inner skull of the BEM, or a sphere).
     * @param[in] mindist Minimum distance of guess points from the bounding surface (m).
     * @param[in] exclude Exclude guess points closer than this to the sphere origin (m).
     * @param[in] grid Spacing of the guess grid (m).
     * @param[in, out] f Dipole fit data providing the forward model.
     * @param[in] guessrad Radius of the spherical guess volume when no surface bounds it (m).
     *
     */
    InvGuessData(const std::filesystem::path& guessname, const std::filesystem::path& guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData* f, float guessrad = 0.080f);

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
    std::vector<std::unique_ptr<InvDipoleForward>> guess_fwd;    /**< Forward solutions for the guesses. */
    int nguess = 0;                                              /**< How many sources. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_inv

#endif // INV_GUESS_DATA_H
