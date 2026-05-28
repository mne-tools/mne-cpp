//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwd_bem_solution.cpp
 * @since March 2017
 * @brief FwdBemSolution implementation — storage for the dense @c ncoil × @c np Geselowitz projection that contracts BEM node potentials into MEG coil readings or EEG electrode voltages.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_bem_solution.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdBemSolution::FwdBemSolution()
:ncoil(0)
,np(0)
{
}

//=============================================================================================================

FwdBemSolution::~FwdBemSolution()
{
}
