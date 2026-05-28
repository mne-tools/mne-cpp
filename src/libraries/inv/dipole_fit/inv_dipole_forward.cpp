//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_dipole_forward.cpp
 * @since March 2026
 * @brief Implementation of the @ref INVLIB::InvDipoleForward forward-field cache.
 *
 * Provides the trivial constructor / destructor for the cache; the
 * forward-matrix population, column normalisation and SVD that fill
 * the struct are driven from @ref InvDipoleFitData and the candidate-
 * generation code in @ref InvGuessData.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_dipole_forward.h"

// Constructor and destructor are defaulted in the header.
// Eigen members handle their own memory automatically.
