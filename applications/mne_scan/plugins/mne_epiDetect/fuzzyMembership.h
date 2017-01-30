//=============================================================================================================
/**
* @file     fuzzyMembership.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     %{currentdate} Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief     fuzzyMembership class declaration.
*
*/

#ifndef FUZZYMEMBERSHIP_H
#define FUZZYMEMBERSHIP_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
//#include <generics/circularmatrixbuffer.h>


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
// DEFINE NAMESPACE Cannot convert result of " Cpp.namespaces('fuzzyMembership')[0]" to string.
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Cannot convert result of " Cpp.namespaces('fuzzyMembership')[0]" to string. FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class fuzzyMembership
{

public:
    typedef QSharedPointer<fuzzyMembership> SPtr;            /**< Shared pointer type for fuzzyMembership. */
    typedef QSharedPointer<const fuzzyMembership> ConstSPtr; /**< Const shared pointer type for fuzzyMembership. */

    //=========================================================================================================
    /**
    * Constructs a fuzzyMembership object.
    */
    fuzzyMembership();
    Eigen::VectorXd getMembership(const Eigen::MatrixXd valHistory, const Eigen::MatrixXd valHistoryOld, const Eigen::VectorXd current, const Eigen::VectorXd epiHistory,  double margin, char type);


protected:

private:

    Eigen::MatrixXd m_dmatValHistory;
    Eigen::VectorXd m_dvecMaxVal;
    Eigen::VectorXd m_dvecMinVal;
    Eigen::VectorXd m_dvecMeanVal;
    Eigen::VectorXd m_dvecMuChannel;

signals:


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


#endif // FUZZYMEMBERSHIP_H
