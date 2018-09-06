//=============================================================================================================
/**
* @file     rtinvop.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     Definition of the RtInvOp Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinvop.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtInvOp::RtInvOp(FiffInfo::SPtr &p_pFiffInfo, MNEForwardSolution::SPtr &p_pFwd, QObject *parent)
: QThread(parent)
, m_bIsRunning(false)
, m_pFiffInfo(p_pFiffInfo)
, m_pFwd(p_pFwd)
{
    qRegisterMetaType<MNEInverseOperator::SPtr>("MNEInverseOperator::SPtr");
}


//*************************************************************************************************************

RtInvOp::~RtInvOp()
{
    stop();
}


//*************************************************************************************************************

void RtInvOp::appendNoiseCov(FiffCov &p_noiseCov)
{
    mutex.lock();
    //Use here a circular buffer
    m_vecNoiseCov.push_back(p_noiseCov);

    qDebug() << "RtInvOp m_vecNoiseCov" << m_vecNoiseCov.size();

    mutex.unlock();
}


//*************************************************************************************************************

bool RtInvOp::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void RtInvOp::run()
{
    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        mutex.lock();
        if(m_vecNoiseCov.size() > 0)
        {
            // Restrict forward solution as necessary for MEG
            MNEForwardSolution t_forwardMeg = m_pFwd->pick_types(true, false);

            MNEInverseOperator::SPtr t_invOpMeg(new MNEInverseOperator(*m_pFiffInfo.data(), t_forwardMeg, m_vecNoiseCov[0], 0.2f, 0.8f));
            m_vecNoiseCov.pop_front();

            emit invOperatorCalculated(t_invOpMeg);
        }
        mutex.unlock();
    }
}
