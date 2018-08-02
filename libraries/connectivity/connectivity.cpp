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

Connectivity::Connectivity(const ConnectivitySettings& connectivitySettings)
: m_pConnectivitySettings(ConnectivitySettings::SPtr(new ConnectivitySettings(connectivitySettings)))
{
}


//*************************************************************************************************************

Network Connectivity::calculateConnectivity() const
{
    if(m_pConnectivitySettings->m_sConnectivityMethods.contains("COR")) {
        return Correlation::correlationCoeff(m_pConnectivitySettings->m_matDataList,
                                             m_pConnectivitySettings->m_matNodePositions);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("XCOR")) {
        return CrossCorrelation::crossCorrelation(m_pConnectivitySettings->m_matDataList,
                                                  m_pConnectivitySettings->m_matNodePositions);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("PLI")) {
        return PhaseLagIndex::phaseLagIndex(m_pConnectivitySettings->m_matDataList,
                                            m_pConnectivitySettings->m_matNodePositions,
                                            m_pConnectivitySettings->m_iNfft,
                                            m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("COH")) {
        return Coherence::coherence(m_pConnectivitySettings->m_matDataList,
                                    m_pConnectivitySettings->m_matNodePositions,
                                    m_pConnectivitySettings->m_iNfft,
                                    m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("IMAGCOH")) {
        return ImagCoherence::imagCoherence(m_pConnectivitySettings->m_matDataList,
                                            m_pConnectivitySettings->m_matNodePositions,
                                            m_pConnectivitySettings->m_iNfft,
                                            m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("PLV")) {
        return PhaseLockingValue::phaseLockingValue(m_pConnectivitySettings->m_matDataList,
                                                    m_pConnectivitySettings->m_matNodePositions,
                                                    m_pConnectivitySettings->m_iNfft,
                                                    m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("WPLI")) {
        return WeightedPhaseLagIndex::weightedPhaseLagIndex(m_pConnectivitySettings->m_matDataList,
                                                            m_pConnectivitySettings->m_matNodePositions,
                                                            m_pConnectivitySettings->m_iNfft,
                                                            m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("USPLI")) {
        return UnbiasedSquaredPhaseLagIndex::unbiasedSquaredPhaseLagIndex(m_pConnectivitySettings->m_matDataList,
                                                                          m_pConnectivitySettings->m_matNodePositions,
                                                                          m_pConnectivitySettings->m_iNfft,
                                                                          m_pConnectivitySettings->m_sWindowType);
    } else if(m_pConnectivitySettings->m_sConnectivityMethods.contains("DSWPLI")) {
        return DebiasedSquaredWeightedPhaseLagIndex::debiasedSquaredWeightedPhaseLagIndex(m_pConnectivitySettings->m_matDataList,
                                                                                          m_pConnectivitySettings->m_matNodePositions,
                                                                                          m_pConnectivitySettings->m_iNfft,
                                                                                          m_pConnectivitySettings->m_sWindowType);
    }

    return Network();
}
