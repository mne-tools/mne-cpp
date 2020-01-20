//=============================================================================================================
/**
 * @file     fuzzymembership.cpp
 * @author   Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Louis Eichhorst. All rights reserved.
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
 * @brief   FuzzyMembership class definition.
 *
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fuzzymembership.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FuzzyMembership::FuzzyMembership()
{

}


//*************************************************************************************************************

Eigen::VectorXd FuzzyMembership::getMembership(const Eigen::MatrixXd valHistory, const Eigen::MatrixXd valHistoryOld, const Eigen::VectorXd current, const Eigen::VectorXd epiHistory,  double margin, char type)
{
//TODO: Add different treatment for max/min/meanvalues, if a seizure was detected
//TODO: Add adjustment for the old history values
    m_dmatValHistory = valHistory;
    m_dvecMaxVal = m_dmatValHistory.rowwise().maxCoeff();
    m_dvecMinVal = m_dmatValHistory.rowwise().minCoeff();
    m_dvecMeanVal = m_dmatValHistory.rowwise().mean();
    m_dvecMuChannel.resize(current.rows());

    for(int i=0; i < current.rows(); i++)
    {
        if (current(i) == m_dvecMeanVal(i))
            m_dvecMuChannel(i) = 0;
        else if ((current(i) < m_dvecMeanVal(i)) && ((type == 'l') || (type == 'm')))
        {
            m_dvecMuChannel(i) = 1-((current(i)/(margin*m_dvecMeanVal(i)-margin*m_dvecMinVal(i)))-(m_dvecMeanVal(i)/(margin*m_dvecMeanVal(i)-margin*m_dvecMinVal(i)))+1);
        }
        else if ((current(i) > m_dvecMeanVal(i)) && ((type == 'r') || (type == 'm')))
        {
            m_dvecMuChannel(i) = 1-((current(i)/(margin*m_dvecMeanVal(i)-margin*m_dvecMaxVal(i)))-(m_dvecMeanVal(i)/(margin*m_dvecMeanVal(i)-margin*m_dvecMaxVal(i)))+1);
        }
        else
            m_dvecMuChannel(i) = 0;

        if (m_dvecMuChannel(i) > 1)
            m_dvecMuChannel(i) = 1;

        if (m_dvecMuChannel(i) < 0)
            m_dvecMuChannel(i) = 0;

    }

    return m_dvecMuChannel;
}
