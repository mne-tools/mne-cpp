//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_layer.h
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
 * @brief    FwdEegSphereLayer class declaration.
 *
 */

#ifndef FWD_EEG_SPHERE_LAYER_H
#define FWD_EEG_SPHERE_LAYER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements FwdEegSphereLayer (Replaces *fwdEegSphereLayer,fwdEegSphereLayerRec struct of MNE-C fwd_types.h).
 *
 * @brief Single concentric sphere layer with conductivity ratio for the EEG forward model.
 */
class FWDSHARED_EXPORT FwdEegSphereLayer
{
public:

    //=========================================================================================================
    /**
     * Constructs the Electric Current Dipole
     */
    FwdEegSphereLayer();

    //=========================================================================================================
    /**
     * Destroys the Forward EEG Sphere Layer description
     */
    ~FwdEegSphereLayer();

    //=========================================================================================================
    /**
     * Compare two sphere layers by radius for sorting.
     *
     * @param[in] v1   First layer.
     * @param[in] v2   Second layer.
     *
     * @return True if v1.rad < v2.rad.
     */
    static bool comp_layers(const FwdEegSphereLayer& v1, const FwdEegSphereLayer& v2)
    {
        return v1.rad < v2.rad;
    }

public:
    float rad;          /**< The actual rads. */
    float rel_rad;      /**< Relative rads. */
    float sigma;        /**< Conductivity. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_EEG_SPHERE_LAYER_H
