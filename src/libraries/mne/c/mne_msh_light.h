//=============================================================================================================
/**
 * @file     mne_msh_light.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MneMshLight class declaration.
 *
 */

#ifndef MNEMSHLIGHT_H
#define MNEMSHLIGHT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshLight,mshLightRec struct (analyze_types.c).
 *
 * @brief The MneMshLight class.
 */
class MNESHARED_EXPORT MneMshLight
{
public:
    typedef QSharedPointer<MneMshLight> SPtr;              /**< Shared pointer type for MneMshLight. */
    typedef QSharedPointer<const MneMshLight> ConstSPtr;   /**< Const shared pointer type for MneMshLight. */

    //=========================================================================================================
    /**
     * Constructs the MneMshLight.
     */
    MneMshLight();

    //=========================================================================================================
    /**
     * Copy Constructs of the MneMshLight.
     */
    MneMshLight(const MneMshLight &p_mneMshLight);

    //=========================================================================================================
    /**
     * Constructs the MneMshLight.
     */
    MneMshLight(int state, float posX, float posY,float posZ,float diffX,float diffY,float diffZ);

    //=========================================================================================================
    /**
     * Destroys the MneMshLight.
     */
    ~MneMshLight();

public:
    int   state;			/* On or off? */
    float pos[3];			/* Where is the light? */
    float diff[3];		/* Diffuse intensity */

// ### OLD STRUCT ###
//    typedef struct {		/* Definition of lighting */
//      int   state;			/* On or off? */
//      float pos[3];			/* Where is the light? */
//      float diff[3];		/* Diffuse intensity */
//    } *mshLight,mshLightRec;	/* We are only using diffuse lights here */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHLIGHT_H
