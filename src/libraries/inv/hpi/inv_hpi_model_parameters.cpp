//=============================================================================================================
/**
 * @file     inv_hpi_model_parameters.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     February, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Ruben Dörfel. All rights reserved.
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
 * @brief    InvHpiModelParameters class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_hpi_model_parameters.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvHpiModelParameters::InvHpiModelParameters(const QVector<int> vecHpiFreqs,
                                       const int iSampleFreq,
                                       const int iLineFreq,
                                       const bool bBasic)
    : m_vecHpiFreqs(vecHpiFreqs),
      m_iSampleFreq(iSampleFreq),
      m_iLineFreq(iLineFreq),
      m_bBasic(bBasic)
{
    computeNumberOfCoils();
    checkForLineFreq();
}

//=============================================================================================================

InvHpiModelParameters::InvHpiModelParameters(const InvHpiModelParameters& hpiModelParameter)
    : m_vecHpiFreqs(hpiModelParameter.vecHpiFreqs()),
      m_iNHpiCoils(hpiModelParameter.iNHpiCoils()),
      m_iSampleFreq(hpiModelParameter.iSampleFreq()),
      m_iLineFreq(hpiModelParameter.iLineFreq()),
      m_bBasic(hpiModelParameter.bBasic())
{
}

//=============================================================================================================

void InvHpiModelParameters::computeNumberOfCoils()
{
    m_iNHpiCoils = m_vecHpiFreqs.size();
}

void InvHpiModelParameters::checkForLineFreq()
{
    if(m_iLineFreq == 0) {
        m_bBasic = true;
    }
}

//=============================================================================================================

InvHpiModelParameters InvHpiModelParameters::operator= (const InvHpiModelParameters& other)
{
    if (this != &other) {
        m_vecHpiFreqs = other.vecHpiFreqs();
        m_iNHpiCoils = other.iNHpiCoils();
        m_iSampleFreq = other.iSampleFreq();
        m_iLineFreq = other.iLineFreq();
        m_bBasic = other.bBasic();
    }
    return *this;
}
