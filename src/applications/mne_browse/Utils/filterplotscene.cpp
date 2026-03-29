//=============================================================================================================
/**
 * @file     filterplotscene.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
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

#include <algorithm>
#include <cmath>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace Eigen;

namespace {

constexpr double kPhaseMinDegrees = -180.0;
constexpr double kPhaseMaxDegrees = 180.0;

double mapMagnitudeToY(double magnitudeDbScaled,
                       double maxMagnitude,
                       int marginVert)
{
    return std::min(maxMagnitude, magnitudeDbScaled) - marginVert;
}

double mapPhaseToY(double phaseDegrees,
                   double diagramHeight,
                   int marginVert)
{
    const double clamped = std::clamp(phaseDegrees, kPhaseMinDegrees, kPhaseMaxDegrees);
    const double normalized = (kPhaseMaxDegrees - clamped) / (kPhaseMaxDegrees - kPhaseMinDegrees);
    return normalized * diagramHeight - marginVert;
}

QString formatFrequencyLabel(double frequencyHz)
{
    if(frequencyHz >= 100.0) {
        return QStringLiteral("%1 Hz").arg(QString::number(frequencyHz, 'f', 0));
    }

    if(frequencyHz >= 10.0) {
        return QStringLiteral("%1 Hz").arg(QString::number(frequencyHz, 'f', 1));
    }

    return QStringLiteral("%1 Hz").arg(QString::number(frequencyHz, 'f', 2));
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterPlotScene::FilterPlotScene(QObject *parent) :
    QGraphicsScene(parent),
    m_pCurrentFilter(),
    m_pGraphicsItemPath(new QGraphicsPathItem()),
    m_iScalingFactor(5),
    m_dMaxMagnitude(100*m_iScalingFactor),
    m_iNumberHorizontalLines(4),
    m_iNumberVerticalLines(3),
    m_iAxisTextSize(24),
    m_iDiagramMarginsHoriz(5),
    m_iDiagramMarginsVert(5),
    m_iDiagramSpacing(140),
    m_iCutOffMarkerWidth(3)
{
}


//*************************************************************************************************************

void FilterPlotScene::updateFilter(const QSharedPointer<SessionFilter>& filter, int samplingFreq)
{
    //Clear the scene
    this->clear();

    if(filter && filter->isValid()) {
        m_pCurrentFilter = filter;
    } else {
        m_pCurrentFilter.clear();
        setSceneRect(QRectF());
        return;
    }

    const int diagramWidth = static_cast<int>(m_pCurrentFilter->magnitudeResponse().size());
    const int magnitudePanelX = 0;
    const int phasePanelX = magnitudePanelX + diagramWidth + m_iDiagramSpacing;

    plotFilterFrequencyResponse(magnitudePanelX, diagramWidth);
    plotMagnitudeDiagram(samplingFreq, magnitudePanelX, diagramWidth);

    plotFilterPhaseResponse(phasePanelX, diagramWidth);
    plotPhaseDiagram(samplingFreq, phasePanelX, diagramWidth);
}


//*************************************************************************************************************

void FilterPlotScene::plotMagnitudeDiagram(int samplingFreq, int xOffset, int diagramWidth)
{
    const int fMax = samplingFreq/2; //nyquist frequency

    addRect(xOffset - m_iDiagramMarginsHoriz,
            -m_iDiagramMarginsVert,
            diagramWidth + (m_iDiagramMarginsHoriz * 2),
            m_dMaxMagnitude+(m_iDiagramMarginsVert*2));

    QGraphicsTextItem* title = addText(QStringLiteral("Magnitude"),
                                       QFont(QStringLiteral("Times"), m_iAxisTextSize, QFont::Bold));
    title->setPos(xOffset + diagramWidth / 2.0 - title->boundingRect().width() / 2.0,
                  -title->boundingRect().height() - (m_iDiagramMarginsVert * 4));

    //HORIZONTAL
    //Draw horizontal lines
    for(int i = 1; i <= m_iNumberHorizontalLines; i++)
        addLine(xOffset - m_iDiagramMarginsHoriz,
                (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - m_iDiagramMarginsVert,
                xOffset + diagramWidth + m_iDiagramMarginsHoriz,
                (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - m_iDiagramMarginsVert,
                QPen(Qt::DotLine));

    //Draw vertical axis texts - db magnitude
    for(int i = 0; i <= m_iNumberHorizontalLines+1; i++) {
        QGraphicsTextItem * text = addText(QString("-%1 db").arg(QString().number(i * m_dMaxMagnitude/(m_iScalingFactor*(m_iNumberHorizontalLines+1)),'g',3)),
                                           QFont("Times", m_iAxisTextSize));
        text->setPos(xOffset - text->boundingRect().width() - m_iAxisTextSize/2,
                     (i * (m_dMaxMagnitude/(m_iNumberHorizontalLines+1))) - (text->boundingRect().height()/2) - m_iDiagramMarginsVert);
    }

    //VERTICAL
    //Draw vertical lines
    double length = static_cast<double>(diagramWidth) / static_cast<double>(m_iNumberVerticalLines+1);
    for(int i = 1; i<=m_iNumberVerticalLines; i++)
        addLine(xOffset + i*length - m_iDiagramMarginsHoriz,
                -m_iDiagramMarginsVert,
                xOffset + i*length - m_iDiagramMarginsHoriz,
                m_dMaxMagnitude + m_iDiagramMarginsVert,
                QPen(Qt::DotLine));

    //Draw horizontal axis texts - Hz frequency
    for(int i = 0; i <= m_iNumberVerticalLines+1; i++) {
        const double frequencyHz = i * (fMax / static_cast<double>(m_iNumberVerticalLines + 1));
        QGraphicsTextItem * text = addText(formatFrequencyLabel(frequencyHz),
                                           QFont("Times", m_iAxisTextSize));
        text->setPos(xOffset + i * length - m_iDiagramMarginsHoriz - (text->boundingRect().width()/2),
                     m_dMaxMagnitude + (text->boundingRect().height()/2));
    }

    //Plot lower higher cut off frequency
    auto addCutoffMarker = [this, xOffset, diagramWidth, fMax](double frequencyHz) {
        const double pos = xOffset + (frequencyHz / static_cast<double>(fMax)) * diagramWidth;
        addLine(pos - m_iDiagramMarginsHoriz,
                -m_iDiagramMarginsVert + m_iCutOffMarkerWidth/2,
                pos - m_iDiagramMarginsHoriz,
                m_dMaxMagnitude + m_iDiagramMarginsVert - m_iCutOffMarkerWidth/2,
                QPen(Qt::red,m_iCutOffMarkerWidth));
    };

    switch(m_pCurrentFilter->filterType()) {
        case SessionFilter::FilterType::LowPass:
        case SessionFilter::FilterType::HighPass:
            addCutoffMarker(m_pCurrentFilter->cutoffLowHz());
            break;
        case SessionFilter::FilterType::BandPass:
        case SessionFilter::FilterType::BandStop:
            addCutoffMarker(m_pCurrentFilter->cutoffLowHz());
            addCutoffMarker(m_pCurrentFilter->cutoffHighHz());
            break;
    }
}

//*************************************************************************************************************

void FilterPlotScene::plotPhaseDiagram(int samplingFreq, int xOffset, int diagramWidth)
{
    const double phaseHeight = m_dMaxMagnitude;
    const int fMax = samplingFreq / 2;

    addRect(xOffset - m_iDiagramMarginsHoriz,
            -m_iDiagramMarginsVert,
            diagramWidth + (m_iDiagramMarginsHoriz * 2),
            phaseHeight + (m_iDiagramMarginsVert * 2));

    QGraphicsTextItem* title = addText(QStringLiteral("Phase"),
                                       QFont(QStringLiteral("Times"), m_iAxisTextSize, QFont::Bold));
    title->setPos(xOffset + diagramWidth / 2.0 - title->boundingRect().width() / 2.0,
                  -title->boundingRect().height() - (m_iDiagramMarginsVert * 4));

    for(int i = 1; i <= m_iNumberHorizontalLines; ++i) {
        const double y = (i * (phaseHeight / (m_iNumberHorizontalLines + 1))) - m_iDiagramMarginsVert;
        addLine(xOffset - m_iDiagramMarginsHoriz,
                y,
                xOffset + diagramWidth + m_iDiagramMarginsHoriz,
                y,
                QPen(Qt::DotLine));
    }

    for(int i = 0; i <= m_iNumberHorizontalLines + 1; ++i) {
        const double phaseDegrees = kPhaseMaxDegrees
                                    - i * ((kPhaseMaxDegrees - kPhaseMinDegrees) / static_cast<double>(m_iNumberHorizontalLines + 1));
        QGraphicsTextItem* text = addText(QStringLiteral("%1°").arg(QString::number(phaseDegrees, 'f', 0)),
                                          QFont(QStringLiteral("Times"), m_iAxisTextSize));
        text->setPos(xOffset - text->boundingRect().width() - m_iAxisTextSize / 2.0,
                     (i * (phaseHeight / (m_iNumberHorizontalLines + 1)))
                        - (text->boundingRect().height() / 2.0)
                        - m_iDiagramMarginsVert);
    }

    const double length = static_cast<double>(diagramWidth) / static_cast<double>(m_iNumberVerticalLines + 1);
    for(int i = 1; i <= m_iNumberVerticalLines; ++i) {
        addLine(xOffset + i * length - m_iDiagramMarginsHoriz,
                -m_iDiagramMarginsVert,
                xOffset + i * length - m_iDiagramMarginsHoriz,
                phaseHeight + m_iDiagramMarginsVert,
                QPen(Qt::DotLine));
    }

    for(int i = 0; i <= m_iNumberVerticalLines + 1; ++i) {
        const double frequencyHz = i * (fMax / static_cast<double>(m_iNumberVerticalLines + 1));
        QGraphicsTextItem* text = addText(formatFrequencyLabel(frequencyHz),
                                          QFont(QStringLiteral("Times"), m_iAxisTextSize));
        text->setPos(xOffset + i * length - m_iDiagramMarginsHoriz - (text->boundingRect().width() / 2.0),
                     phaseHeight + (text->boundingRect().height() / 2.0));
    }

    auto addCutoffMarker = [this, xOffset, diagramWidth, fMax, phaseHeight](double frequencyHz) {
        const double pos = xOffset + (frequencyHz / static_cast<double>(fMax)) * diagramWidth;
        addLine(pos - m_iDiagramMarginsHoriz,
                -m_iDiagramMarginsVert + m_iCutOffMarkerWidth / 2.0,
                pos - m_iDiagramMarginsHoriz,
                phaseHeight + m_iDiagramMarginsVert - m_iCutOffMarkerWidth / 2.0,
                QPen(Qt::red, m_iCutOffMarkerWidth));
    };

    switch(m_pCurrentFilter->filterType()) {
        case SessionFilter::FilterType::LowPass:
        case SessionFilter::FilterType::HighPass:
            addCutoffMarker(m_pCurrentFilter->cutoffLowHz());
            break;
        case SessionFilter::FilterType::BandPass:
        case SessionFilter::FilterType::BandStop:
            addCutoffMarker(m_pCurrentFilter->cutoffLowHz());
            addCutoffMarker(m_pCurrentFilter->cutoffHighHz());
            break;
    }
}


//*************************************************************************************************************

void FilterPlotScene::plotFilterFrequencyResponse(int xOffset, int diagramWidth)
{
    const VectorXd magnitudeResponse = m_pCurrentFilter->magnitudeResponse();
    if(magnitudeResponse.size() == 0) {
        return;
    }

    const int numberCoeff = static_cast<int>(magnitudeResponse.size());
    const int dsFactor = numberCoeff > 2000 ? std::max(1, numberCoeff / 2000) : 1;

    const double maxMagnitude = std::max(1e-12, magnitudeResponse.maxCoeff());
    const VectorXd normalizedResponse = magnitudeResponse / maxMagnitude;

    //Create painter path
    QPainterPath path;
    const double initialDb = -20.0 * log10(std::max(1e-12, std::abs(normalizedResponse(0)))) * m_iScalingFactor;
    path.moveTo(xOffset, mapMagnitudeToY(initialDb, m_dMaxMagnitude, m_iDiagramMarginsVert));

    for(int i = 0; i<numberCoeff; i+=dsFactor) {
        const double x = xOffset + (static_cast<double>(i) / static_cast<double>(std::max(1, numberCoeff - 1))) * diagramWidth;
        const double magnitudeDb = -20.0 * log10(std::max(1e-12, std::abs(normalizedResponse(i)))) * m_iScalingFactor;
        path.lineTo(x, mapMagnitudeToY(magnitudeDb, m_dMaxMagnitude, m_iDiagramMarginsVert));
    }

    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);

    //Clear old and plot new filter path
    m_pGraphicsItemPath = addPath(path, pen);
}

//*************************************************************************************************************

void FilterPlotScene::plotFilterPhaseResponse(int xOffset, int diagramWidth)
{
    const VectorXd phaseResponse = m_pCurrentFilter->phaseResponse();
    if(phaseResponse.size() == 0) {
        return;
    }

    const int pointCount = static_cast<int>(phaseResponse.size());
    const int dsFactor = pointCount > 2000 ? std::max(1, pointCount / 2000) : 1;

    QPainterPath path;
    path.moveTo(xOffset,
                mapPhaseToY(phaseResponse(0),
                            m_dMaxMagnitude,
                            m_iDiagramMarginsVert));

    for(int index = 0; index < pointCount; index += dsFactor) {
        const double x = xOffset + (static_cast<double>(index) / static_cast<double>(std::max(1, pointCount - 1))) * diagramWidth;
        const double y = mapPhaseToY(phaseResponse(index),
                                     m_dMaxMagnitude,
                                     m_iDiagramMarginsVert);
        path.lineTo(x, y);
    }

    QPen pen;
    pen.setColor(QColor(35, 73, 138));
    pen.setWidth(2);

    addPath(path, pen);
}


//*************************************************************************************************************
