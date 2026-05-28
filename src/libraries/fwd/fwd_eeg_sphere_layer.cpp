//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_eeg_sphere_layer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    FwdEegSphereLayer implementation — trivial storage for one concentric shell (@c rad, @c sigma) of the layered de Munck / Berg-Scherg EEG head model, plus the inner-to-outer radius comparator used by the model setup.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_layer.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereLayer::FwdEegSphereLayer()
: rad(0.0f)
, rel_rad(0.0f)
, sigma(0.0f)
{
}

//=============================================================================================================

//FwdEegSphereLayer::FwdEegSphereLayer(const FwdEegSphereLayer& p_FwdEegSphereLayer)
//: rad(p_FwdEegSphereLayer.rad)
//, rel_rad(p_FwdEegSphereLayer.rel_rad)
//, sigma(p_FwdEegSphereLayer.sigma)
//{
//}

//=============================================================================================================

FwdEegSphereLayer::~FwdEegSphereLayer()
{
}
