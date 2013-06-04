//=============================================================================================================
/**
* @file     inverseviewproducer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
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
* @brief    Implementation of the InverseViewProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inverseviewproducer.h"

#include "inverseview.h"

#include <QApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InverseViewProducer::InverseViewProducer(qint32 p_iT)
: m_bIsRunning(false)
, m_iFps(60)
, m_iT(p_iT)
, m_nTimeSteps(0)
, m_dGlobalMaximum(0)
, m_iDownSampling(20)
, m_bBeep(true)
{

}


//*************************************************************************************************************

InverseViewProducer::~InverseViewProducer()
{
}


//*************************************************************************************************************

void InverseViewProducer::pushSourceEstimate(SourceEstimate &p_sourceEstimate)
{
    mutex.lock();

    m_curSourceEstimate = p_sourceEstimate;
    m_iT = (qint32)floor(p_sourceEstimate.tstep*1000000);
    m_nTimeSteps = p_sourceEstimate.data.cols();

    m_vecMaxActivation = m_curSourceEstimate.data.rowwise().maxCoeff();
    m_dGlobalMaximum = m_vecMaxActivation.maxCoeff();

    float t_fFpu = ((float)m_iFps)/(1000000.0f);

    m_iDownSampling = (qint32)floor(1/(m_iT*t_fFpu));

    mutex.unlock();
}


//*************************************************************************************************************

void InverseViewProducer::stop()
{
    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();
}


//*************************************************************************************************************

void InverseViewProducer::run()
{
    qint32 simCount = 0;
    qint32 currentSample = 0;

    float t_fTimeOld = -1.0;

    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        //LNdT hack
        mutex.lock();
        if(m_nTimeSteps > 0)
        {
            //downsample to 60Hz
            if(simCount%m_iDownSampling == 0)
            {
                currentSample = simCount%m_nTimeSteps;
                if (m_bBeep && ((t_fTimeOld < 0.0) && (m_curSourceEstimate.times(currentSample) >= 0.0)))
                {
                    QApplication::beep();
                    qDebug("beep");
                }
                t_fTimeOld = m_curSourceEstimate.times(currentSample);
                QSharedPointer<VectorXd> p_qVecCurrentActivation(new VectorXd(m_curSourceEstimate.data.col(currentSample)));
                emit sourceEstimateSample(p_qVecCurrentActivation);
            }

            ++simCount;
        }
        mutex.unlock();

        usleep(m_iT);
    }
}
