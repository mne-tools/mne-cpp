//=============================================================================================================
/**
* @file		ecgproducer.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the ECGProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgproducer.h"
#include "ecgsimulator.h"

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ECGSimulatorModule;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECGProducer::ECGProducer(ECGSimulator* simulator, ECGBuffer* buffer_I, ECGBuffer* buffer_II, ECGBuffer* buffer_III)
: m_pECGSimulator(simulator)
, m_pdBuffer_I(buffer_I)
, m_pdBuffer_II(buffer_II)
, m_pdBuffer_III(buffer_III)
, m_bIsRunning(true)

{

}


//*************************************************************************************************************

ECGProducer::~ECGProducer()
{

}


//*************************************************************************************************************

void ECGProducer::stop()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void ECGProducer::run()
{
    unsigned int uiSamplePeriod = (unsigned int) (1000000.0/(m_pECGSimulator->m_fSamplingRate));
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

        //ECG I
        if(m_pECGSimulator->m_pECGChannel_ECG_I->isEnabled())
        {
            if(uiCounter_I >= (m_pECGSimulator->m_pECGChannel_ECG_I->getSamples().size()-1))
                uiCounter_I = 0;

            value_I = 0;

            for(unsigned char i = 0; i < m_pECGSimulator->m_iDownsamplingFactor; ++i)
            {
                value_I = value_I + m_pECGSimulator->m_pECGChannel_ECG_I->getSamples()[uiCounter_I];
            }

            value_I = value_I / m_pECGSimulator->m_iDownsamplingFactor;
            m_pdBuffer_I->push(value_I);

            uiCounter_I = uiCounter_I + m_pECGSimulator->m_iDownsamplingFactor;

        }
        //ECG II
        if(m_pECGSimulator->m_pECGChannel_ECG_II->isEnabled())
        {
            if(uiCounter_II >= (m_pECGSimulator->m_pECGChannel_ECG_II->getSamples().size()-1))
                uiCounter_II = 0;

            value_II = 0;

            for(unsigned char i = 0; i < m_pECGSimulator->m_iDownsamplingFactor; ++i)
            {
                value_II = value_II + m_pECGSimulator->m_pECGChannel_ECG_II->getSamples()[uiCounter_II];
            }

            value_II = value_II / m_pECGSimulator->m_iDownsamplingFactor;
            m_pdBuffer_II->push(value_II);

            uiCounter_II = uiCounter_II + m_pECGSimulator->m_iDownsamplingFactor;
        }

        //ECG III
        if(m_pECGSimulator->m_pECGChannel_ECG_III->isEnabled())
        {
            if(uiCounter_III >= (m_pECGSimulator->m_pECGChannel_ECG_III->getSamples().size()-1))
                uiCounter_III = 0;

            value_III = 0;

            for(unsigned char i = 0; i < m_pECGSimulator->m_iDownsamplingFactor; ++i)
            {

                value_III = value_III + m_pECGSimulator->m_pECGChannel_ECG_III->getSamples()[uiCounter_III];
            }

            value_III = value_III / m_pECGSimulator->m_iDownsamplingFactor;
            m_pdBuffer_III->push(value_III);

            uiCounter_III = uiCounter_III + m_pECGSimulator->m_iDownsamplingFactor;
        }

    }
}
