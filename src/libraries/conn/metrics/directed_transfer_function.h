//=============================================================================================================
/**
 * @file     directed_transfer_function.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief     DirectedTransferFunction class declaration.
 *
 */

#ifndef DIRECTEDTRANSFERFUNCTION_H
#define DIRECTEDTRANSFERFUNCTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

#include "abstractmetric.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

class Network;
class ConnectivitySettings;

//=============================================================================================================
/**
 * This class computes the Directed Transfer Function (DTF) from an MVAR model.
 *
 * DTF_{ij}(f) = |H_{ij}(f)|^2 / sum_k |H_{ik}(f)|^2
 *
 * @brief This class computes the Directed Transfer Function.
 * @since 2.2.0
 */
class CONNSHARED_EXPORT DirectedTransferFunction : public AbstractMetric
{

public:
    typedef QSharedPointer<DirectedTransferFunction> SPtr;            /**< Shared pointer type for DirectedTransferFunction. */
    typedef QSharedPointer<const DirectedTransferFunction> ConstSPtr; /**< Const shared pointer type for DirectedTransferFunction. */

    //=========================================================================================================
    /**
     * Constructs a DirectedTransferFunction object.
     */
    explicit DirectedTransferFunction();

    //=========================================================================================================
    /**
     * Calculates the Directed Transfer Function between all channel pairs.
     *
     * @param[in] connectivitySettings   The input data and parameters.
     *
     * @return                   The connectivity information in form of a network structure.
     *
     * @since 2.2.0
     */
    static Network calculate(ConnectivitySettings &connectivitySettings);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // DIRECTEDTRANSFERFUNCTION_H
