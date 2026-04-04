//=============================================================================================================
/**
 * @file     butterflysceneitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ButterflySceneItem class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflysceneitem.h"
#include "rawsettings.h"

#include <cmath>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflySceneItem::ButterflySceneItem(QString setName, int setKind, int setUnit, const QList<QColor> &defaultColors)
: m_sSetName(setName)
, m_iSetKind(setKind)
, m_iSetUnit(setUnit)
, m_cAverageColors(defaultColors)
, m_pFiffInfo(Q_NULLPTR)
{
    //Init m_scaleMap
    m_scaleMap["MEG_grad"] = DELEGATE_SCALE_MEG_GRAD;
    m_scaleMap["MEG_mag"]  = DELEGATE_SCALE_MEG_MAG;
    m_scaleMap["MEG_EEG"]  = DELEGATE_SCALE_EEG;
    m_scaleMap["MEG_EOG"]  = DELEGATE_SCALE_EOG;
    m_scaleMap["MEG_EMG"]  = DELEGATE_SCALE_EMG;
    m_scaleMap["MEG_ECG"]  = DELEGATE_SCALE_ECG;
    m_scaleMap["MEG_MISC"] = DELEGATE_SCALE_MISC;
    m_scaleMap["MEG_STIM"] = DELEGATE_SCALE_STIM;
}


//*************************************************************************************************************

QRectF ButterflySceneItem::boundingRect() const
{
    const int totalW = kMarginLeft + m_plotWidth + kMarginRight;
    const int totalH = kMarginTop + m_plotHeight + kMarginBottom;
    return QRectF(0, 0, totalW, totalH);
}


//*************************************************************************************************************

QRectF ButterflySceneItem::plotArea() const
{
    return QRectF(kMarginLeft, kMarginTop, m_plotWidth, m_plotHeight);
}


//*************************************************************************************************************

void ButterflySceneItem::setPlotSize(int plotW, int plotH)
{
    prepareGeometryChange();
    m_plotWidth  = qMax(100, plotW);
    m_plotHeight = qMax(60, plotH);
}


//*************************************************************************************************************

double ButterflySceneItem::xToTime(double sceneX) const
{
    if(!m_pFiffInfo || m_lAverageData.second <= 0)
        return 0.0;
    const QRectF pa = plotArea();
    const double frac = (sceneX - pa.x()) / pa.width();
    const double tMin = m_firstLastSample.first / m_pFiffInfo->sfreq;
    const double tMax = m_firstLastSample.second / m_pFiffInfo->sfreq;
    return tMin + frac * (tMax - tMin);
}


//*************************************************************************************************************

double ButterflySceneItem::yToAmplitude(double sceneY) const
{
    const QRectF pa = plotArea();
    double dMaxValue = 1e-09;
    if(m_iSetKind == FIFFV_MEG_CH) {
        dMaxValue = (m_iSetUnit == FIFF_UNIT_T_M) ? m_scaleMap.value("MEG_grad", DELEGATE_SCALE_MEG_GRAD)
                                                   : m_scaleMap.value("MEG_mag", DELEGATE_SCALE_MEG_MAG);
    } else if(m_iSetKind == FIFFV_EEG_CH) {
        dMaxValue = m_scaleMap.value("MEG_EEG", DELEGATE_SCALE_EEG);
    }
    const double centerY = pa.y() + pa.height() / 2.0;
    return -(sceneY - centerY) / (pa.height() / (2.0 * dMaxValue));
}


//*************************************************************************************************************

void ButterflySceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Background
    painter->save();
    painter->setBrush(QColor(250, 250, 250));
    painter->setPen(Qt::NoPen);
    painter->drawRect(plotArea());
    painter->restore();

    // Axes and labels
    painter->save();
    paintAxes(painter);
    painter->restore();

    // Average data paths
    painter->save();
    paintAveragePaths(painter);
    painter->restore();

    // GFP overlay
    if(m_bShowGFP) {
        painter->save();
        paintGFP(painter);
        painter->restore();
    }

    // Title
    painter->save();
    QFont titleFont = painter->font();
    titleFont.setPointSizeF(10);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(Qt::black);
    painter->drawText(QRectF(kMarginLeft, 2, m_plotWidth, kMarginTop - 4),
                      Qt::AlignHCenter | Qt::AlignTop, m_sSetName);
    painter->restore();
}


//*************************************************************************************************************

void ButterflySceneItem::setEvokedData(const FiffEvoked& evoked)
{
    m_displayEvoked = evoked;
    m_pFiffInfo = &m_displayEvoked.info;
    m_lAverageData.first = m_displayEvoked.data.data();
    m_lAverageData.second = m_displayEvoked.data.cols();
    m_firstLastSample.first = m_displayEvoked.first;
    m_firstLastSample.second = m_displayEvoked.last;
}


//*************************************************************************************************************

void ButterflySceneItem::paintAxes(QPainter *painter)
{
    if(!m_pFiffInfo || m_lAverageData.second <= 0)
        return;

    const QRectF pa = plotArea();
    const double sfreq = m_pFiffInfo->sfreq;
    const double tMin = m_firstLastSample.first / sfreq;
    const double tMax = m_firstLastSample.second / sfreq;
    const double tRange = tMax - tMin;
    if(tRange <= 0.0)
        return;

    // --- Determine max value for Y axis ---
    double dMaxValue = 1e-09;
    QString yUnitLabel;
    if(m_iSetKind == FIFFV_MEG_CH) {
        if(m_iSetUnit == FIFF_UNIT_T_M) {
            dMaxValue = m_scaleMap.value("MEG_grad", DELEGATE_SCALE_MEG_GRAD);
            yUnitLabel = "fT/cm";
        } else {
            dMaxValue = m_scaleMap.value("MEG_mag", DELEGATE_SCALE_MEG_MAG);
            yUnitLabel = "fT";
        }
    } else if(m_iSetKind == FIFFV_EEG_CH) {
        dMaxValue = m_scaleMap.value("MEG_EEG", DELEGATE_SCALE_EEG);
        yUnitLabel = QStringLiteral("\u00B5V");  // µV
    }

    // --- Axis pens ---
    QPen axisPen(QColor(80, 80, 80));
    axisPen.setWidthF(1.0);
    QPen gridPen(QColor(200, 200, 200));
    gridPen.setWidthF(0.5);
    gridPen.setStyle(Qt::DotLine);
    QPen stimPen(QColor(200, 0, 0));
    stimPen.setWidthF(1.5);

    QFont labelFont = painter->font();
    labelFont.setPointSizeF(7);
    painter->setFont(labelFont);

    // --- Draw frame ---
    painter->setPen(axisPen);
    painter->drawRect(pa);

    // --- Zero line ---
    double centerY = pa.y() + pa.height() / 2.0;
    painter->setPen(gridPen);
    QPainterPath zeroPath;
    zeroPath.moveTo(pa.x(), centerY);
    zeroPath.lineTo(pa.x() + pa.width(), centerY);
    painter->drawPath(zeroPath);

    // --- Time axis ticks ---
    // Determine nice tick spacing: aim for ~8-12 ticks
    double rawStep = tRange / 10.0;
    double mag = std::pow(10.0, std::floor(std::log10(rawStep)));
    double residual = rawStep / mag;
    double tickStep;
    if(residual < 1.5) tickStep = 1.0 * mag;
    else if(residual < 3.5) tickStep = 2.0 * mag;
    else if(residual < 7.5) tickStep = 5.0 * mag;
    else tickStep = 10.0 * mag;

    double tStart = std::ceil(tMin / tickStep) * tickStep;
    painter->setPen(axisPen);
    for(double t = tStart; t <= tMax + tickStep * 0.01; t += tickStep) {
        double xFrac = (t - tMin) / tRange;
        double xPos = pa.x() + xFrac * pa.width();

        // Grid line
        painter->setPen(gridPen);
        painter->drawLine(QPointF(xPos, pa.y()), QPointF(xPos, pa.y() + pa.height()));

        // Tick + label
        painter->setPen(axisPen);
        painter->drawLine(QPointF(xPos, pa.y() + pa.height()), QPointF(xPos, pa.y() + pa.height() + 4));
        QString timeLabel = QString::number(t * 1000.0, 'f', 0); // ms
        QRectF labelRect(xPos - 25, pa.y() + pa.height() + 5, 50, 15);
        painter->drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);
    }

    // Time axis label
    QFont axisLabelFont = labelFont;
    axisLabelFont.setPointSizeF(8);
    painter->setFont(axisLabelFont);
    painter->setPen(axisPen);
    painter->drawText(QRectF(pa.x(), pa.y() + pa.height() + 20, pa.width(), 18),
                      Qt::AlignHCenter | Qt::AlignTop, "Time (ms)");

    // --- Amplitude axis ticks ---
    painter->setFont(labelFont);
    // Y ticks: ~5 ticks above + 5 below
    double yTickStep;
    double yRawStep = dMaxValue / 4.0;

    // Determine Y display scale factor and unit
    double yDisplayFactor = 1.0;
    if(m_iSetKind == FIFFV_MEG_CH && m_iSetUnit == FIFF_UNIT_T_M) {
        yDisplayFactor = 1e13;  // T/m → fT/cm
    } else if(m_iSetKind == FIFFV_MEG_CH && m_iSetUnit == FIFF_UNIT_T) {
        yDisplayFactor = 1e15;  // T → fT
    } else if(m_iSetKind == FIFFV_EEG_CH) {
        yDisplayFactor = 1e6;   // V → µV
    }

    double yMag = std::pow(10.0, std::floor(std::log10(yRawStep * yDisplayFactor)));
    double yRes = (yRawStep * yDisplayFactor) / yMag;
    double yTickDisplayStep;
    if(yRes < 1.5) yTickDisplayStep = 1.0 * yMag;
    else if(yRes < 3.5) yTickDisplayStep = 2.0 * yMag;
    else if(yRes < 7.5) yTickDisplayStep = 5.0 * yMag;
    else yTickDisplayStep = 10.0 * yMag;
    yTickStep = yTickDisplayStep / yDisplayFactor;

    double scaleY = pa.height() / (2.0 * dMaxValue);
    for(double v = -dMaxValue; v <= dMaxValue + yTickStep * 0.01; v += yTickStep) {
        double yPos = centerY - v * scaleY;
        if(yPos < pa.y() - 1 || yPos > pa.y() + pa.height() + 1)
            continue;

        // Grid line
        if(std::abs(v) > yTickStep * 0.01) {
            painter->setPen(gridPen);
            painter->drawLine(QPointF(pa.x(), yPos), QPointF(pa.x() + pa.width(), yPos));
        }

        // Tick + label
        painter->setPen(axisPen);
        painter->drawLine(QPointF(pa.x() - 4, yPos), QPointF(pa.x(), yPos));
        double displayVal = v * yDisplayFactor;
        QString valLabel = QString::number(displayVal, 'g', 3);
        QRectF yLabelRect(0, yPos - 8, kMarginLeft - 8, 16);
        painter->drawText(yLabelRect, Qt::AlignRight | Qt::AlignVCenter, valLabel);
    }

    // Y axis label
    painter->save();
    painter->setFont(axisLabelFont);
    painter->setPen(axisPen);
    painter->translate(12, pa.y() + pa.height() / 2.0);
    painter->rotate(-90);
    painter->drawText(QRectF(-pa.height()/2, -8, pa.height(), 16),
                      Qt::AlignHCenter | Qt::AlignVCenter, yUnitLabel);
    painter->restore();

    // --- Stimulus line at t=0 ---
    if(tMin < 0 && tMax > 0) {
        double x0 = pa.x() + (-tMin / tRange) * pa.width();
        painter->setPen(stimPen);
        painter->drawLine(QPointF(x0, pa.y()), QPointF(x0, pa.y() + pa.height()));

        painter->setFont(labelFont);
        painter->setPen(stimPen);
        painter->drawText(QRectF(x0 - 20, pa.y() - 14, 40, 14),
                          Qt::AlignHCenter | Qt::AlignBottom, "0 ms");
    }
}


//*************************************************************************************************************

void ButterflySceneItem::paintAveragePaths(QPainter *painter)
{
    if(!m_pFiffInfo) {
        return;
    }

    const QRectF pa = plotArea();
    const double sfreq = m_pFiffInfo->sfreq;
    const double tMin = m_firstLastSample.first / sfreq;
    const double tMax = m_firstLastSample.second / sfreq;
    const double tRange = tMax - tMin;

    //Create path for all channels
    for(int i = 0; i < m_pFiffInfo->chs.size() ;i++) {

        FiffChInfo fiffChInfoTemp = m_pFiffInfo->chs.at(i);

        if(m_pFiffInfo->bads.contains(fiffChInfoTemp.ch_name) == false) {
            //Only plot EEG or MEG channels
            if((fiffChInfoTemp.kind == FIFFV_MEG_CH && m_iSetKind == FIFFV_MEG_CH && fiffChInfoTemp.unit == FIFF_UNIT_T_M && m_iSetUnit == FIFF_UNIT_T_M) ||    //MEG grad
               (fiffChInfoTemp.kind == FIFFV_MEG_CH && m_iSetKind == FIFFV_MEG_CH && fiffChInfoTemp.unit == FIFF_UNIT_T && m_iSetUnit == FIFF_UNIT_T) ||        //MEG mag
               (fiffChInfoTemp.kind == FIFFV_EEG_CH && m_iSetKind == FIFFV_EEG_CH)) {                                                                           //EEG
                //Determine channel scaling
                double dMaxValue = 1e-09;
                switch(fiffChInfoTemp.kind) {
                    case FIFFV_MEG_CH: {
                        if(fiffChInfoTemp.unit == FIFF_UNIT_T_M) {
                            dMaxValue = m_scaleMap["MEG_grad"];
                        }
                        else if(fiffChInfoTemp.unit == FIFF_UNIT_T)
                            dMaxValue = m_scaleMap["MEG_mag"];
                        break;
                    }
                    case FIFFV_EEG_CH: {
                        dMaxValue = m_scaleMap["MEG_EEG"];
                        break;
                    }
                }

                //Get data pointer for the current channel
                const double* averageData = m_lAverageData.first;
                int totalCols =  m_lAverageData.second; //equals to the number of samples stored in the data matrix
                if(totalCols <= 0)
                    continue;

                //Calculate step sizes using the plot area
                double xStep = pa.width() / static_cast<double>(totalCols);

                //Calculate scaling value
                double dScaleY = (pa.height())/(2*dMaxValue);

                //Setup the painter
                double centerY = pa.y() + pa.height() / 2.0;
                double startVal = (*(averageData+(0*m_pFiffInfo->chs.size())+i)) * dScaleY;
                QPainterPath path = QPainterPath(QPointF(pa.x(), centerY - startVal));
                QPen pen;
                pen.setStyle(Qt::SolidLine);
                if(!m_cAverageColors.isEmpty() && i<m_cAverageColors.size())
                    pen.setColor(m_cAverageColors.at(i));
                pen.setWidthF(0.5);
                painter->setPen(pen);

                //Generate plot path
                for(int u = 0; u < totalCols; ++u) {
                    //evoked matrix is stored in column major
                    double val = (*(averageData+(u*m_pFiffInfo->chs.size())+i)) * dScaleY;

                    //Clamp to plot area
                    double halfH = pa.height() / 2.0;
                    if(val > halfH) val = halfH;
                    else if(val < -halfH) val = -halfH;

                    double xPos = pa.x() + u * xStep;
                    path.lineTo(QPointF(xPos, centerY - val));
                }

                //Paint the path
                painter->drawPath(path);
            }
        }
    }
}


//*************************************************************************************************************

void ButterflySceneItem::paintGFP(QPainter *painter)
{
    if(!m_pFiffInfo)
        return;

    const QRectF pa = plotArea();
    const double* averageData = m_lAverageData.first;
    int totalCols = m_lAverageData.second;
    if(totalCols <= 0)
        return;

    // Determine scale for this channel type
    double dMaxValue = 1e-09;
    if(m_iSetKind == FIFFV_MEG_CH) {
        if(m_iSetUnit == FIFF_UNIT_T_M)
            dMaxValue = m_scaleMap.value("MEG_grad", 4e-11);
        else
            dMaxValue = m_scaleMap.value("MEG_mag", 1.2e-12);
    } else if(m_iSetKind == FIFFV_EEG_CH) {
        dMaxValue = m_scaleMap.value("MEG_EEG", 30e-6);
    }

    double dScaleY = pa.height() / (2.0 * dMaxValue);
    double xStep = pa.width() / static_cast<double>(totalCols);
    double centerY = pa.y() + pa.height() / 2.0;

    // Collect channel indices matching this kind/unit
    QVector<int> chIndices;
    int nChs = m_pFiffInfo->chs.size();
    for(int i = 0; i < nChs; ++i) {
        const FiffChInfo& ch = m_pFiffInfo->chs[i];
        if(m_pFiffInfo->bads.contains(ch.ch_name))
            continue;
        if(ch.kind == FIFFV_MEG_CH && m_iSetKind == FIFFV_MEG_CH &&
           ch.unit == m_iSetUnit) {
            chIndices.append(i);
        } else if(ch.kind == FIFFV_EEG_CH && m_iSetKind == FIFFV_EEG_CH) {
            chIndices.append(i);
        }
    }
    if(chIndices.isEmpty())
        return;

    int nMatch = chIndices.size();

    // Build GFP path: RMS across matching channels at each time point
    // GFP is always positive, drawn symmetrically around the center line as filled area
    QPainterPath pathTop(QPointF(pa.x(), centerY));
    QPainterPath pathBot(QPointF(pa.x(), centerY));

    for(int u = 0; u < totalCols; ++u) {
        double sumSq = 0.0;
        for(int c = 0; c < nMatch; ++c) {
            double v = *(averageData + (u * nChs) + chIndices[c]);
            sumSq += v * v;
        }
        double gfp = std::sqrt(sumSq / nMatch);
        double yVal = gfp * dScaleY;
        double halfH = pa.height() / 2.0;
        if(yVal > halfH) yVal = halfH;

        double xPos = pa.x() + u * xStep;
        pathTop.lineTo(QPointF(xPos, centerY - yVal));
        pathBot.lineTo(QPointF(xPos, centerY + yVal));
    }

    // Close top path back along center
    pathTop.lineTo(QPointF(pa.x() + (totalCols - 1) * xStep, centerY));
    pathTop.closeSubpath();

    // Draw filled GFP area (semi-transparent)
    QColor gfpFill(108, 92, 231, 35);   // purple, very transparent
    QColor gfpLine(108, 92, 231, 180);  // purple, mostly opaque
    painter->setBrush(gfpFill);
    painter->setPen(Qt::NoPen);
    painter->drawPath(pathTop);

    // Bottom mirror
    pathBot.lineTo(QPointF(pa.x() + (totalCols - 1) * xStep, centerY));
    pathBot.closeSubpath();
    painter->drawPath(pathBot);

    // Draw GFP outline
    QPainterPath outline(QPointF(pa.x(), centerY));
    for(int u = 0; u < totalCols; ++u) {
        double sumSq = 0.0;
        for(int c = 0; c < nMatch; ++c) {
            double v = *(averageData + (u * nChs) + chIndices[c]);
            sumSq += v * v;
        }
        double gfp = std::sqrt(sumSq / nMatch);
        double yVal = gfp * dScaleY;
        double halfH = pa.height() / 2.0;
        if(yVal > halfH) yVal = halfH;
        outline.lineTo(QPointF(pa.x() + u * xStep, centerY - yVal));
    }

    QPen gfpPen(gfpLine);
    gfpPen.setWidthF(1.5);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(gfpPen);
    painter->drawPath(outline);

    // Mirror outline
    QPainterPath outlineMirror(QPointF(pa.x(), centerY));
    for(int u = 0; u < totalCols; ++u) {
        double sumSq = 0.0;
        for(int c = 0; c < nMatch; ++c) {
            double v = *(averageData + (u * nChs) + chIndices[c]);
            sumSq += v * v;
        }
        double gfp = std::sqrt(sumSq / nMatch);
        double yVal = gfp * dScaleY;
        double halfH = pa.height() / 2.0;
        if(yVal > halfH) yVal = halfH;
        outlineMirror.lineTo(QPointF(pa.x() + u * xStep, centerY + yVal));
    }
    painter->drawPath(outlineMirror);
}









