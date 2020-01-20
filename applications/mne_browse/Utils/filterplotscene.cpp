//=============================================================================================================
/**
 * @file     filterplotscene.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lars Debor, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the FilterPlotScene class.
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

using namespace MNEBROWSE;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterPlotScene::FilterPlotScene(QObject *parent) :
    QGraphicsScene(parent),
    m_pCurrentFilter(new FilterOperator()),
    m_pGraphicsItemPath(new QGraphicsPathItem()),
    m_iScalingFactor(5),
    m_dMaxMagnitude(100*m_iScalingFactor),
    m_iNumberHorizontalLines(4),
    m_iNumberVerticalLines(3),
    m_iAxisTextSize(24),
    m_iDiagramMarginsHoriz(5),
    m_iDiagramMarginsVert(5),
    m_iCutOffLow(5),
    m_iCutOffHigh(40),
    m_iCutOffMarkerWidth(3)
{
}


//*************************************************************************************************************

void FilterPlotScene::updateFilter(QSharedPointer<MNEOperator> operatorFilter, int samplingFreq, int cutOffLow, int cutOffHigh)
{
    //set member variables
    m_iCutOffLow = cutOffLow;
    m_iCutOffHigh = cutOffHigh;

    //Clear the scene
    this->clear();

    if(operatorFilter->m_OperatorType == MNEOperator::FILTER)
        m_pCurrentFilter = operatorFilter.staticCast<FilterOperator>();

    //Plot newly set filter
    plotFilterFrequencyResponse();

    //Plot the magnitude diagram
    plotMagnitudeDiagram(samplingFreq);
}


//*************************************************************************************************************

void FilterPlotScene::plotMagnitudeDiagram(int samplingFreq)
{
    //Get row vector with filter coefficients
    RowVectorXcd coefficientsAFreq = m_pCurrentFilter->m_dFFTCoeffA;

    int numberCoeff = coefficientsAFreq.cols();
    if(numberCoeff>2000) {//if to large downsample
        int dsFactor = numberCoeff/2000;
        numberCoeff = numberCoeff/dsFactor;
    }

    int fMax = samplingFreq/2; //nyquist frequency

    addRect(-m_iDiagramMarginsHoriz,
            -m_iDiagramMarginsVert,
            numberCoeff+(m_iDiagramMarginsHoriz*2),
            m_dMaxMagnitude+(m_iDiagramMarginsVert*2));

    //HORIZONTAL
    //Draw horizontal lines
    for(int i = 1; i <= m_iNumberHorizontalLines; i++)
        addLine(-m_iDiagramMarginsHoriz,
                (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - m_iDiagramMarginsVert,
                numberCoeff + m_iDiagramMarginsHoriz,
                (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - m_iDiagramMarginsVert,
                QPen(Qt::DotLine));

    //Draw vertical axis texts - db magnitude
    for(int i = 0; i <= m_iNumberHorizontalLines+1; i++) {
        QGraphicsTextItem * text = addText(QString("-%1 db").arg(QString().number(i * m_dMaxMagnitude/(m_iScalingFactor*(m_iNumberHorizontalLines+1)),'g',3)),
                                           QFont("Times", m_iAxisTextSize));
        text->setPos(-text->boundingRect().width() - m_iAxisTextSize/2,
                     (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - (text->boundingRect().height()/2) - m_iDiagramMarginsVert);
    }

    //VERTICAL
    //Draw vertical lines
    double length = (double)(numberCoeff) / (double)(m_iNumberVerticalLines+1);
    for(int i = 1; i<=m_iNumberVerticalLines; i++)
        addLine(i*length - m_iDiagramMarginsHoriz,
                -m_iDiagramMarginsVert,
                i*length - m_iDiagramMarginsHoriz,
                m_dMaxMagnitude + m_iDiagramMarginsVert,
                QPen(Qt::DotLine));

    //Draw horizontal axis texts - Hz frequency
    for(int i = 0; i <= m_iNumberVerticalLines+1; i++) {
        QGraphicsTextItem * text = addText(QString("%1 Hz").arg(i*(fMax/(m_iNumberVerticalLines+1))),
                                           QFont("Times", m_iAxisTextSize));
        text->setPos(i * length - m_iDiagramMarginsHoriz - (text->boundingRect().width()/2),
                     m_dMaxMagnitude + (text->boundingRect().height()/2));
    }

    //Plot lower higher cut off frequency
    double pos = 0;
    switch(m_pCurrentFilter->m_Type) {
        case 0://LPF
            pos = ((double)m_iCutOffLow / (double)fMax) * numberCoeff;
            addLine(pos - m_iDiagramMarginsHoriz,
                    -m_iDiagramMarginsVert + m_iCutOffMarkerWidth/2,
                    pos - m_iDiagramMarginsHoriz,
                    m_dMaxMagnitude + m_iDiagramMarginsVert - m_iCutOffMarkerWidth/2,
                    QPen(Qt::red,m_iCutOffMarkerWidth));
        break;

        case 1://HPF
            pos = ((double)m_iCutOffHigh / (double)fMax) * numberCoeff;
            addLine(pos - m_iDiagramMarginsHoriz,
                    -m_iDiagramMarginsVert + m_iCutOffMarkerWidth/2,
                    pos - m_iDiagramMarginsHoriz,
                    m_dMaxMagnitude + m_iDiagramMarginsVert - m_iCutOffMarkerWidth/2,
                    QPen(Qt::red,m_iCutOffMarkerWidth));
        break;

        case 2://BPF
            pos = ((double)m_iCutOffLow / (double)fMax) * numberCoeff;
            addLine(pos - m_iDiagramMarginsHoriz,
                    -m_iDiagramMarginsVert + m_iCutOffMarkerWidth/2,
                    pos - m_iDiagramMarginsHoriz,
                    m_dMaxMagnitude + m_iDiagramMarginsVert - m_iCutOffMarkerWidth/2,
                    QPen(Qt::red,m_iCutOffMarkerWidth));

            pos = ((double)m_iCutOffHigh / (double)fMax) * numberCoeff;
            addLine(pos - m_iDiagramMarginsHoriz,
                    -m_iDiagramMarginsVert + m_iCutOffMarkerWidth/2,
                    pos - m_iDiagramMarginsHoriz,
                    m_dMaxMagnitude + m_iDiagramMarginsVert - m_iCutOffMarkerWidth/2,
                    QPen(Qt::red,m_iCutOffMarkerWidth));
        break;
    }
}


//*************************************************************************************************************

void FilterPlotScene::plotFilterFrequencyResponse()
{
    //Get row vector with filter coefficients and norm to 1
    RowVectorXcd coefficientsAFreq = m_pCurrentFilter->m_dFFTCoeffA;

    int numberCoeff = coefficientsAFreq.cols();
    int dsFactor = 0;
    if(numberCoeff>2000)
        dsFactor = numberCoeff/2000;

    double max = 0;
    for(int i = 0; i<numberCoeff-dsFactor; i++)
        if(std::abs(coefficientsAFreq(i)) > max)
            max = std::abs(coefficientsAFreq(i));

    coefficientsAFreq = coefficientsAFreq / max;

    //Create painter path
    QPainterPath path;
    double y = -20 * log10(std::abs(coefficientsAFreq(0))) * m_iScalingFactor; //-1 because we want to plot upwards
    if(y > m_dMaxMagnitude)
        y = m_dMaxMagnitude;
    y -= m_iDiagramMarginsVert;

    path.moveTo(-m_iDiagramMarginsVert, y); //convert to db

    for(int i = 0; i<numberCoeff; i+=dsFactor) {
        y = -20 * log10(std::abs(coefficientsAFreq(i))) * m_iScalingFactor; //-1 because we want to plot upwards
        if(y > m_dMaxMagnitude)
            y = m_dMaxMagnitude;

        y -= m_iDiagramMarginsVert;
        path.lineTo(path.currentPosition().x()+1,y);
    }

    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);

    //Clear old and plot new filter path
    m_pGraphicsItemPath = addPath(path, pen);
}


//*************************************************************************************************************

