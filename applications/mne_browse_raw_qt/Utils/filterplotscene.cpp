//=============================================================================================================
/**
* @file     filterplotscene.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the FilterPlotScene class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterplotscene.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterPlotScene::FilterPlotScene(QObject *parent) :
    QGraphicsScene(parent),
    m_pCurrentFilter(new FilterOperator())
{
//    addLine(0,0,50,50);
//    addRect(this->sceneRect());
}


//*************************************************************************************************************

void FilterPlotScene::updateFilter(QSharedPointer<MNEOperator> operatorFilter)
{
    if(operatorFilter->m_OperatorType == MNEOperator::FILTER)
        m_pCurrentFilter = operatorFilter.staticCast<FilterOperator>();

    //Plot newly set filter
    plotFilterFrequencyResponse();
}


//*************************************************************************************************************

void FilterPlotScene::plotFilterFrequencyResponse()
{
    RowVectorXcd coefficientsAFreq = m_pCurrentFilter->m_dFFTCoeffA;

    coefficientsAFreq.normalize();

    //Create painter path
    QPainterPath path;
    path.moveTo(0,-abs(coefficientsAFreq(0))*8000);

    for(int i = 0; i<coefficientsAFreq.cols(); i++) {
        //qDebug()<<abs(coefficientsAFreq(i));
        path.lineTo(path.currentPosition().x()+1,-abs(coefficientsAFreq(i))*8000);
    }

    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(4);

    //Clear scene and plot new filter path
    clear();
    addPath(path, pen);
}


//*************************************************************************************************************

