//=============================================================================================================
/**
 * @file     inv_dipole_forward.h
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
 * @brief    InvDipoleForward class declaration.
 *
 */

#ifndef INV_DIPOLE_FORWARD_H
#define INV_DIPOLE_FORWARD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Implements InvDipoleForward (Replaces *dipoleForward,dipoleForwardRec struct of MNE-C fit_types.h).
 *
 * @brief Stores forward field matrices and source space data for magnetic dipole fitting.
 */
class INVSHARED_EXPORT InvDipoleForward
{
public:
    typedef QSharedPointer<InvDipoleForward> SPtr;              /**< Shared pointer type for InvDipoleForward. */
    typedef QSharedPointer<const InvDipoleForward> ConstSPtr;   /**< Const shared pointer type for InvDipoleForward. */
    typedef std::unique_ptr<InvDipoleForward> UPtr;             /**< Unique pointer type for InvDipoleForward. */

    //=========================================================================================================
    /**
     * Constructs the InvDipole Forward
     * Refactored: new_dipole_forward (dipole_forward.c)
     */
    InvDipoleForward();

//    //=========================================================================================================
//    /**
//    * Copy constructor.
//    *
//    * @param[in] p_DipoleForward    InvDipoleForward which should be copied
//    */
//    InvDipoleForward(const InvDipoleForward& p_DipoleForward);

    //=========================================================================================================
    /**
     * Destroys the InvDipole Forward description
     * Refactored: free_dipole_forward_2 (dipole_fit_setup.c)
     */
    ~InvDipoleForward();

public:
    float **rd;         /**< Dipole locations. */
    int   ndip;         /**< How many dipoles. */
    float **fwd;        /**< The forward solution (projected and whitened). */
    float *scales;      /**< Scales applied to the columns of fwd. */
    float **uu;         /**< The left singular vectors of the forward matrix. */
    float **vv;         /**< The right singular vectors of the forward matrix. */
    float *sing;        /**< The singular values. */
    int   nch;          /**< Number of channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVLIB

#endif // INV_DIPOLE_FORWARD_H
