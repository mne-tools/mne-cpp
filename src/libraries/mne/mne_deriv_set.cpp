//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEDerivSet.
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
