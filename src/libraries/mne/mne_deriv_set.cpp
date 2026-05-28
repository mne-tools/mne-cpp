//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_deriv_set.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref MNELIB::MNEDerivSet.
 *
 * Implements FIFF I/O of the @c FIFFB_MNE_DERIVATIONS block and the
 * batched application of every derivation to one sample matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_deriv_set.h"
#include "mne_deriv.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEDerivSet::MNEDerivSet()
{
}

//=============================================================================================================

MNEDerivSet::~MNEDerivSet()
{
    for (int k = 0; k < derivs.size(); k++)
        delete derivs[k];
}
