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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InverseViewProducer::InverseViewProducer(InverseView *pInverseView)
: m_pInverseView(pInverseView)
{

}


//*************************************************************************************************************

InverseViewProducer::~InverseViewProducer()
{
}


//*************************************************************************************************************

void InverseViewProducer::pushSourceEstimate()//SourceEstimate &p_sourceEstimate)
{
//    m_timer->stop();
//    m_curSourceEstimate = p_sourceEstimate;

//    m_nTimeSteps = m_curSourceEstimate.times.size();

//    qDebug() << "source estimates" << m_curSourceEstimate.data.rows();

//    m_dMaxActivation = m_curSourceEstimate.data.colwise().maxCoeff();

//    m_dGlobalMaximum = m_dMaxActivation.maxCoeff();

//    m_timer->start(m_curSourceEstimate.tstep*1000);
}


//*************************************************************************************************************

void InverseViewProducer::run()
{
//    qint32 currentSample = simCount%m_nTimeSteps;

//    VectorXd t_curLabelActivation = VectorXd::Zero(m_pSceneNode->palette()->size());

//    for(qint32 h = 0; h < 2; ++h)
//    {
//        for(qint32 i = 0; i < m_sourceSpace[h].cluster_info.numClust(); ++i)
//        {
//            qint32 labelId = m_sourceSpace[h].cluster_info.clusterLabelIds[i];
//            qint32 colorIdx = m_qListMapLabelIdIndex[h][labelId];
//            //search for max activation within one label - by checking if there is already an assigned value
//            if(abs(t_curLabelActivation[colorIdx]) < abs(m_curSourceEstimate.data(i, currentSample)))
//                t_curLabelActivation[colorIdx] = m_curSourceEstimate.data(i, currentSample);
//        }
//    }

//    for(qint32 i = 0; i < m_pSceneNode->palette()->size(); ++i)
//    {
//        if(m_dMaxActivation[i] != 0)
//        {
//            qint32 iVal = (t_curLabelActivation[i]/m_dGlobalMaximum/*m_dMaxActivation[currentSample]*/) * 255;
//            iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

//            int r, g, b;
//            if(m_iColorMode == 0)
//            {
//                r = iVal;
//                g = iVal;
//                b = iVal;
//            }
//            else if(m_iColorMode == 1)
//            {
//                r = iVal;
//                g = iVal;
//                b = iVal;
//            }

//            m_pSceneNode->palette()->material(i)->setSpecularColor(QColor(r,g,b,200));
//        }
//    }


//    ++simCount;

//    this->update();
}
