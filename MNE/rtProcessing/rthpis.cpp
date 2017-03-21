//=============================================================================================================
/**
* @file     rthpis.cpp
* @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@ntu-ilmenau.de>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Chiran Doshi, Lorenz Esch, Limin Sun, and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the RtHPIS Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rthpis.h"

#include <inverse/hpiFit/hpifit.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace IOBUFFER;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtHPIS::RtHPIS(FiffInfo::SPtr p_pFiffInfo, QObject *parent)
: QThread(parent)
, m_pFiffInfo(p_pFiffInfo)
, m_bIsRunning(false)
{
    qRegisterMetaType<RTPROCESSINGLIB::FittingResult>("RTPROCESSINGLIB::FittingResult");
}


//*************************************************************************************************************

RtHPIS::~RtHPIS()
{
    if(this->isRunning()){
        stop();
    }
}


//*************************************************************************************************************

void RtHPIS::append(const MatrixXd &p_DataSegment)
{
    if(!m_pRawMatrixBuffer) {
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(20, p_DataSegment.rows(), p_DataSegment.cols()));
    }

    m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

bool RtHPIS::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()) {
        QThread::wait();
    }

    m_bIsRunning = true;
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtHPIS::stop()
{
    m_bIsRunning = false;

    m_pRawMatrixBuffer->releaseFromPop();

    m_pRawMatrixBuffer->clear();

    return true;
}


//*************************************************************************************************************

void RtHPIS::setCoilFrequencies(const QVector<int>& vCoilFreqs)
{
    m_mutex.lock();
    m_vCoilFreqs = vCoilFreqs;
    m_mutex.unlock();
}


//*************************************************************************************************************

void RtHPIS::run()
{
    MatrixXd matData;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            matData = m_pRawMatrixBuffer->pop();

            //Perform actual fitting
            FittingResult fitResult;
            fitResult.devHeadTrans.from = 1;
            fitResult.devHeadTrans.to = 4;

            HPIFit::fitHPI(matData,
                              fitResult.devHeadTrans,
                              m_vCoilFreqs,
                              fitResult.errorDistances,
                              fitResult.fittedCoils,
                              m_pFiffInfo);

            emit newFittingResultAvailable(fitResult);
        }
    }
}
