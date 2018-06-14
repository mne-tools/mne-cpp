//=============================================================================================================
/**
* @file     realtimeevoked.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the RealTimeEvoked class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevoked.h"

#include <time.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvoked::RealTimeEvoked(QObject *parent)
: NewMeasurement(QMetaType::type("RealTimeEvoked::SPtr"), parent)
, m_pFiffEvoked(new FiffEvoked)
, m_bInitialized(false)
, m_iPreStimSamples(0)
{

}


//*************************************************************************************************************

RealTimeEvoked::~RealTimeEvoked()
{

}


//*************************************************************************************************************

void RealTimeEvoked::init(FiffInfo::SPtr p_fiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo.clear();
    m_qListChColors.clear();

    m_pFiffInfo = p_fiffInfo;

    qsrand(time(NULL));
    for(qint32 i = 0; i < p_fiffInfo->nchan; ++i)
    {
         m_qListChColors.append(QColor(qrand() % 256, qrand() % 256, qrand() % 256));

        RealTimeSampleArrayChInfo initChInfo;
        initChInfo.setChannelName(p_fiffInfo->chs[i].ch_name);

        // set channel Unit
        initChInfo.setUnit(p_fiffInfo->chs[i].unit);

        //Treat stimulus channels different
        if(p_fiffInfo->chs[i].kind == FIFFV_STIM_CH)
        {
//            initChInfo.setUnit("");
            initChInfo.setMinValue(0);
            initChInfo.setMaxValue(1.0e6);
        }

        // set channel Kind
        initChInfo.setKind(p_fiffInfo->chs[i].kind);

        // set channel coil
        initChInfo.setCoil(p_fiffInfo->chs[i].chpos.coil_type);

        m_qListChInfo.append(initChInfo);
    }
}


//*************************************************************************************************************

FiffEvoked::SPtr& RealTimeEvoked::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffEvoked;
}


//*************************************************************************************************************

void RealTimeEvoked::setValue(FiffEvoked& v, FiffInfo::SPtr p_fiffinfo)
{
    if(m_pFiffEvoked->data.cols() != v.data.cols())
        m_bInitialized = false;

    //Store
    m_qMutex.lock();
    *m_pFiffEvoked = v;
    m_qMutex.unlock();

    if(!m_bInitialized)
    {
        init(p_fiffinfo);

        m_qMutex.lock();
        m_iPreStimSamples = 0;
        for(qint32 i = 0; i < m_pFiffEvoked->times.size(); ++i)
        {
            if(m_pFiffEvoked->times[i] >= 0)
                break;
            else
                ++m_iPreStimSamples;
        }

        m_bInitialized = true;
        m_qMutex.unlock();
    }

    emit notify();
}


//*************************************************************************************************************
