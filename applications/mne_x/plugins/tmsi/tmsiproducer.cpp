//=============================================================================================================
/**
* @file     tmsiproducer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the TMSIProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsiproducer.h"
#include "tmsi.h"

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIProducer::TMSIProducer(TMSI* simulator, dBuffer::SPtr& buffer_I, dBuffer::SPtr& buffer_II, dBuffer::SPtr& buffer_III)
: m_pTMSI(simulator)
, m_pdBuffer_I(buffer_I)
, m_pdBuffer_II(buffer_II)
, m_pdBuffer_III(buffer_III)
, m_bIsRunning(true)

{

}


//*************************************************************************************************************

TMSIProducer::~TMSIProducer()
{

}


//*************************************************************************************************************

void TMSIProducer::stop()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void TMSIProducer::run()
{
    unsigned int uiSamplePeriod = (unsigned int) (1000000.0/(m_pTMSI->m_fSamplingRate));
    int uiCounter_I = 0;
    int uiCounter_II = 0;
    int uiCounter_III = 0;
    m_bIsRunning = true;

    double value_I;
    double value_II;
    double value_III;

    while(m_bIsRunning)
    {
        usleep(uiSamplePeriod);

        //TMSI I
        if(m_pTMSI->m_pTMSIChannel_TMSI_I->isEnabled())
        {
            if(uiCounter_I >= (m_pTMSI->m_pTMSIChannel_TMSI_I->getSamples().size()-1))
                uiCounter_I = 0;

            value_I = 0;

            for(unsigned char i = 0; i < m_pTMSI->m_iDownsamplingFactor; ++i)
            {
                value_I = value_I + m_pTMSI->m_pTMSIChannel_TMSI_I->getSamples()[uiCounter_I];
            }

            value_I = value_I / m_pTMSI->m_iDownsamplingFactor;
            m_pdBuffer_I->push(value_I);

            uiCounter_I = uiCounter_I + m_pTMSI->m_iDownsamplingFactor;

        }
        //TMSI II
        if(m_pTMSI->m_pTMSIChannel_TMSI_II->isEnabled())
        {
            if(uiCounter_II >= (m_pTMSI->m_pTMSIChannel_TMSI_II->getSamples().size()-1))
                uiCounter_II = 0;

            value_II = 0;

            for(unsigned char i = 0; i < m_pTMSI->m_iDownsamplingFactor; ++i)
            {
                value_II = value_II + m_pTMSI->m_pTMSIChannel_TMSI_II->getSamples()[uiCounter_II];
            }

            value_II = value_II / m_pTMSI->m_iDownsamplingFactor;
            m_pdBuffer_II->push(value_II);

            uiCounter_II = uiCounter_II + m_pTMSI->m_iDownsamplingFactor;
        }

        //TMSI III
        if(m_pTMSI->m_pTMSIChannel_TMSI_III->isEnabled())
        {
            if(uiCounter_III >= (m_pTMSI->m_pTMSIChannel_TMSI_III->getSamples().size()-1))
                uiCounter_III = 0;

            value_III = 0;

            for(unsigned char i = 0; i < m_pTMSI->m_iDownsamplingFactor; ++i)
            {

                value_III = value_III + m_pTMSI->m_pTMSIChannel_TMSI_III->getSamples()[uiCounter_III];
            }

            value_III = value_III / m_pTMSI->m_iDownsamplingFactor;
            m_pdBuffer_III->push(value_III);

            uiCounter_III = uiCounter_III + m_pTMSI->m_iDownsamplingFactor;
        }

    }
}
