//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     inv_dipole_forward.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December 2016
 * @brief    Implementation of the @ref mne::InvDipoleForward forward-field cache.
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
