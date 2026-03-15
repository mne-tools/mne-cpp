//=============================================================================================================
/**
 * @file     ecd.h
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
 * @brief    Electric Current Dipole (ECD) class declaration.
 *
 */

#ifndef ECD_H
#define ECD_H

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

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Implements one Electric Current Dipole (Replaces *ecd,ecdRec struct of MNE-C fit_types.h).
 *
 * @brief Single equivalent current dipole with position, orientation, amplitude, and goodness-of-fit.
 */
class INVSHARED_EXPORT ECD
{
public:
    typedef QSharedPointer<ECD> SPtr;              /**< Shared pointer type for ECD. */
    typedef QSharedPointer<const ECD> ConstSPtr;   /**< Const shared pointer type for ECD. */

    //=========================================================================================================
    /**
     * Constructs the Electric Current Dipole
     */
    ECD();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_ECD      Electric Current Dipole which should be copied.
     */
    ECD(const ECD& p_ECD);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     */
    ~ECD();

    //=========================================================================================================
    /**
     * prints the ECD to an stdio file stream.
     *
     * @param[in] f      the file stream to print to;.
     */
    void print(FILE *f) const;

public:
    bool            valid;  /**< Is this dipole valid. */
    float           time;   /**< Time point. */
    Eigen::Vector3f rd;     /**< Dipole location. */
    Eigen::Vector3f Q;      /**< Dipole moment. */
    float           good;   /**< Goodness of fit. */
    float           khi2;   /**< khi^2 value. */
    int             nfree;  /**< Degrees of freedom for the above. */
    int             neval;  /**< Number of function evaluations required for this fit. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVLIB

#endif // ECD_H
