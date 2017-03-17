//=============================================================================================================
/**
* @file     connectivity.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     Connectivity class declaration.
*
*/

#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {


//*************************************************************************************************************
//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class ConnectivitySettings;
class Network;


//=============================================================================================================
/**
* This class handles the incoming settings and computes the actual connectivity estimation.
*
* @brief This class is a container for connectivity settings.
*/
class CONNECTIVITYSHARED_EXPORT Connectivity
{

public:
    typedef QSharedPointer<Connectivity> SPtr;            /**< Shared pointer type for Connectivity. */
    typedef QSharedPointer<const Connectivity> ConstSPtr; /**< Const shared pointer type for Connectivity. */

    //=========================================================================================================
    /**
    * Constructs a Connectivity object.
    */
    explicit Connectivity(const ConnectivitySettings& connectivitySettings);

    //=========================================================================================================
    /**
    * Computes the network based on the current settings.
    *
    * @return Returns the network.
    */
    Network calculateConnectivity() const;

protected:
    //=========================================================================================================
    /**
    * Generate the source level data based on the current settings.
    *
    * @param [out] matData      The source level data.
    * @param [out] matNodePos   The nodes position in 3D space.
    */
    void generateSourceLevelData(Eigen::MatrixXd& matData, Eigen::MatrixX3f& matNodePos) const;

    //=========================================================================================================
    /**
    * Generate the sensor level data based on the current settings.
    *
    * @param [out] matData      The source level data.
    * @param [out] matNodePos   The nodes position in 3D space.
    */
    void generateSensorLevelData(Eigen::MatrixXd& matData, Eigen::MatrixX3f& matNodePos) const;

    QSharedPointer<ConnectivitySettings>    m_pConnectivitySettings;           /**< The current connectivity settings. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITY_H
