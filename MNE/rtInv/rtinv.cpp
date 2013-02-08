//=============================================================================================================
/**
* @file     rtinv.cpp
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
* @brief     implementation of the RtInv Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtinv.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTINVLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtInv::RtInv(FiffInfo &p_fiffInfo, MNEForwardSolution::SPtr p_pFwd, QObject *parent)
: QThread(parent)
, m_fiffInfo(p_fiffInfo)
, m_pFwd(p_pFwd)
{
}


//*************************************************************************************************************

RtInv::~RtInv()
{
    stop();
}


//*************************************************************************************************************
//ToDo use shared pointer for public slot
void RtInv::receiveNoiseCov(FiffCov p_NoiseCov)
{
    mutex.lock();
    //Use here a circular buffer
    m_vecNoiseCov.push_back(FiffCov::SDPtr(new FiffCov(p_NoiseCov)));

    mutex.unlock();
}


//*************************************************************************************************************

bool RtInv::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void RtInv::run()
{
    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        if(m_vecNoiseCov.size() > 0)
        {

            // Restrict forward solution as necessary for MEG
            MNEForwardSolution forward_meg = m_pFwd->pick_types_forward(m_fiffInfo, true, false);

            //Put this inside make_inverse_operator

            qDebug() << "Inverse operator";

//            inverse_operator_meeg = make_inverse_operator(info, forward_meeg, noise_cov,
//                                                          loose=0.2, depth=0.8)

            FiffCov::SDPtr t_NoiseCov(new FiffCov(m_vecNoiseCov[0]->prepare_noise_cov(m_fiffInfo, m_fiffInfo.ch_names)));
            m_vecNoiseCov.pop_front();

//            prepare_noise_cov;

        }
    }
}
