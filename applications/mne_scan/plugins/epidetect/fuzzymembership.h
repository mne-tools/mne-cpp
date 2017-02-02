//=============================================================================================================
/**
* @file     fuzzymembership.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Louis Eichhorst and Matti Hamalainen. All rights reserved.
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
* @brief     FuzzyMembership class declaration.
*
*/

#ifndef FUZZYMEMBERSHIP_H
#define FUZZYMEMBERSHIP_H


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


//=============================================================================================================
/**
*
* @brief Calculates membership of the current value in relation to the value history.
*/

class FuzzyMembership
{

public:
    typedef QSharedPointer<FuzzyMembership> SPtr;            /**< Shared pointer type for FuzzyMembership. */
    typedef QSharedPointer<const FuzzyMembership> ConstSPtr; /**< Const shared pointer type for FuzzyMembership. */

    //=========================================================================================================
    /**
    * Constructs a FuzzyMembership object.
    */
    FuzzyMembership();

    //=========================================================================================================
    /**
    * Returns membershipvalues.
    */
    Eigen::VectorXd getMembership(const Eigen::MatrixXd valHistory, const Eigen::MatrixXd valHistoryOld, const Eigen::VectorXd current, const Eigen::VectorXd epiHistory,  double margin, char type);

private:

    Eigen::MatrixXd m_dmatValHistory;   /**< Contains previous values for each channel. */
    Eigen::VectorXd m_dvecMaxVal;       /**< Contains the maximum value for each channel. */
    Eigen::VectorXd m_dvecMinVal;       /**< Contains the minimum value for each channel. */
    Eigen::VectorXd m_dvecMeanVal;      /**< Contains the mean value for each channel. */
    Eigen::VectorXd m_dvecMuChannel;    /**< Contains the membership value for each channel. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


#endif // FUZZYMEMBERSHIP_H
