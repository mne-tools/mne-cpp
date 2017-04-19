//=============================================================================================================
/**
* @file     analyzedata.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the Analyze Data Container Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzedata.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;
using namespace INVERSELIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeData::AnalyzeData(QObject *parent)
: QObject(parent)
, m_iCurrentEstimate(-1)
, m_iCurrentSample(0)
, m_iCurrentECDSet(-1)
{

}


//*************************************************************************************************************

AnalyzeData::~AnalyzeData()
{

}


//*************************************************************************************************************

const MNESourceEstimate &AnalyzeData::currentSTC() const
{
    return m_qListEstimates[m_iCurrentEstimate];
}


//*************************************************************************************************************

void AnalyzeData::addSTC(const MNESourceEstimate &stc)
{
    m_qListEstimates.append(stc);
    m_iCurrentEstimate = m_qListEstimates.size() -1;
    m_iCurrentSample = 0;
    emit stcChanged_signal();
}


//*************************************************************************************************************

void AnalyzeData::setCurrentSTCSample(int sample)
{
    if(sample >= 0 && sample < m_qListEstimates[m_iCurrentEstimate].samples() && sample != m_iCurrentSample) {
        m_iCurrentSample = sample;
        emit stcSampleChanged_signal(sample);
    }
}


//*************************************************************************************************************

int AnalyzeData::currentSTCSample()
{
    return m_iCurrentSample;
}


//*************************************************************************************************************

const ECDSet& AnalyzeData::currentECDSet() const
{
    return m_qListECDSets[m_iCurrentECDSet].second;
}


//*************************************************************************************************************

void AnalyzeData::addECDSet( DipoleFitSettings &ecdSettings,  ECDSet &ecdSet)
{
    m_qListECDSets.append(qMakePair(ecdSettings, ecdSet));
    m_iCurrentECDSet = m_qListECDSets.size() -1;
    emit ecdSetChanged_signal();
}


//*************************************************************************************************************

const QList< QPair< DipoleFitSettings, ECDSet > >& AnalyzeData::ecdSets() const
{
    return m_qListECDSets;
}
