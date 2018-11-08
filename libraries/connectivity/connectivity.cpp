//=============================================================================================================
/**
* @file     connectivity.cpp
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
* @brief    Connectivity class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity.h"

#include "connectivitysettings.h"
#include "network/network.h"
#include "metrics/correlation.h"
#include "metrics/crosscorrelation.h"
#include "metrics/coherence.h"
#include "metrics/imagcoherence.h"
#include "metrics/phaselagindex.h"
#include "metrics/phaselockingvalue.h"
#include "metrics/weightedphaselagindex.h"
#include "metrics/unbiasedsquaredphaselagindex.h"
#include "metrics/debiasedsquaredweightedphaselagindex.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Connectivity::Connectivity()
{
}


//*************************************************************************************************************

Network Connectivity::calculate(const ConnectivitySettings& connectivitySettings)
{
    //TODO: Use multithreading to work on multiple connectivity methods at the same time
    if(connectivitySettings.m_sConnectivityMethods.contains("COR")) {
        return Correlation::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("XCOR")) {
        return CrossCorrelation::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("PLI")) {
        return PhaseLagIndex::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("COH")) {
        return Coherence::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("IMAGCOH")) {
        return ImagCoherence::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("PLV")) {
        return PhaseLockingValue::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("WPLI")) {
        return WeightedPhaseLagIndex::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("USPLI")) {
        return UnbiasedSquaredPhaseLagIndex::calculate(connectivitySettings);
    } else if(connectivitySettings.m_sConnectivityMethods.contains("DSWPLI")) {
        return DebiasedSquaredWeightedPhaseLagIndex::calculate(connectivitySettings);
    }

    qDebug() << "Connectivity::calculateConnectivity - Connectivity method unknown.";

    return Network();
}
