//=============================================================================================================
/**
* @file     connectivitysettings.h
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
* @brief     ConnectivitySettings class declaration.
*
*/

#ifndef CONNECTIVITYSETTINGS_H
#define CONNECTIVITYSETTINGS_H


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


//=============================================================================================================
/**
* This class is a container for connectivity settings.
*
* @brief This class is a container for connectivity settings.
*/
class CONNECTIVITYSHARED_EXPORT ConnectivitySettings
{

public:
    typedef QSharedPointer<ConnectivitySettings> SPtr;            /**< Shared pointer type for ConnectivitySettings. */
    typedef QSharedPointer<const ConnectivitySettings> ConstSPtr; /**< Const shared pointer type for ConnectivitySettings. */

    //=========================================================================================================
    /**
    * Constructs a ConnectivitySettings object.
    */
    explicit ConnectivitySettings(const QStringList &arguments);

    QString m_sConnectivityMethod;          /**< The connectivity method. */
    QString m_sAnnotType;                   /**< The annotation type. */
    QString m_sSubj;                        /**< The subject name. */
    QString m_sSubjDir;                     /**< The subject's folder. */
    QString m_sFwd;                         /**< The path to the forward solution. */
    QString m_sCov;                         /**< The path to the covariance matrix. */
    QString m_sSourceLocMethod;             /**< The source localization method. */
    QString m_sMeas;                        /**< The path to the averaged data. */
    QString m_sCoilType;                    /**< The coil type. Only used if channel type is set to meg. */
    QString m_sChType;                      /**< The channel type. */

    bool m_bDoSourceLoc;                    /**< Whether to perform source localization before the connectivity estimation. */
    bool m_bDoClust;                        /**< Whether to cluster the source space for source localization. */

    double m_dSnr;                          /**< The SNR value. */
    int m_iAveIdx;                          /**< The The average index to take from the input data. */

protected:
    //=========================================================================================================
    /**
    * Parses the input arguments.
    *
    * @param [in] arguments     List of all the arguments.
    */
    void parseArguments(const QStringList& arguments);

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITYSETTINGS_H
