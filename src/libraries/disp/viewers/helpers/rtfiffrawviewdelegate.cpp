//=============================================================================================================
/**
 * @file     rtfiffrawviewdelegate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Definition of the RtFiffRawViewDelegate Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfiffrawviewdelegate.h"
#include "rtfiffrawviewmodel.h"
#include "../rtfiffrawview.h"

#include "../scalingview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>
#include <QPainterPath>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFiffRawViewDelegate::RtFiffRawViewDelegate(RtFiffRawView* parent)
: QAbstractItemDelegate(parent)
, m_pParent(parent)
, m_dMaxValue(0.0)
, m_dScaleY(0.0)
, m_iActiveRow(0)
, m_iUpperItemIndex(0)
{

}

//=============================================================================================================

void RtFiffRawViewDelegate::initPainterPaths(const QAbstractTableModel *model)
{
    for(int i = 0; i<model->rowCount(); i++)
        m_painterPaths.append(QPainterPath());

    // Init pens
    QColor colorMarker(233,0,43);
    colorMarker.setAlpha(160);

    m_penMarker = QPen(colorMarker, 2, Qt::DashLine);

    m_penGrid = QPen(Qt::black, 1, Qt::DashLine);
    m_penTimeSpacers = QPen(Qt::black, 1, Qt::DashLine);

    m_penFreeze = QPen(Qt::darkGray, 1, Qt::SolidLine);
    m_penFreezeSelected = QPen(Qt::darkRed, 1, Qt::SolidLine);

    m_penFreezeBad = QPen(Qt::darkGray, 0.1, Qt::SolidLine);
    m_penFreezeSelectedBad = QPen(Qt::darkRed, 1, Qt::SolidLine);

    m_penNormal = QPen(Qt::darkBlue, 1, Qt::SolidLine);
    m_penNormalSelected = QPen(Qt::red, 1, Qt::SolidLine);

    m_penNormalBad = QPen(Qt::darkBlue, 0.1, Qt::SolidLine);
    m_penNormalSelectedBad = QPen(Qt::red, 1, Qt::SolidLine);
}

//=============================================================================================================

void createPaths(const QModelIndex &index,
                 const QStyleOptionViewItem &option,
                 QPainterPath &path,
                 QPainterPath &lastPath,
                 QPointF &ellipsePos,
                 QPointF &markerPosition,
                 QString &amplitude,
                 const QVector<float> &data,
                 const QVector<float> &lastData)
{
    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = t_pModel->getKind(index.row());

    float fMaxValue = DISPLIB::getScalingValue(t_pModel->getScaling(), kind, t_pModel->getUnit(index.row()));

    float dValue;
    float dScaleY = option.rect.height()/(2*fMaxValue);

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    float dDx = ((float)option.rect.width()) / t_pModel->getMaxSamples();

    //Move to initial starting point
    if(data.size() > 0)
    {
//        float val = data[0];
        dValue = 0;//(val-data[0])*dScaleY;

        float newY = y_base-dValue;//Reverse direction -> plot the right way

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x());

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < data.size(); ++i) {
        float val = data[i] - data[0]; //remove first sample data[0] as offset
        dValue = val*dScaleY;
        //qDebug()<<"val"<<val<<"dScaleY"<<dScaleY<<"dValue"<<dValue;

        float newY = y_base-dValue;//Reverse direction -> plot the right way

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x()+dDx);

        path.lineTo(qSamplePosition);

        //Create ellipse position
        if(i == (qint32)(markerPosition.x()/dDx)) {
            ellipsePos.setX(path.currentPosition().x()+dDx);
            ellipsePos.setY(newY+(option.rect.height()/2));

            amplitude = QString::number(data[i]);
        }
    }

    //create lines from one to the next sample for last path
    qint32 sample_offset = t_pModel->numVLines() + 1;
    qSamplePosition.setX(qSamplePosition.x() + dDx*sample_offset);

    //start painting from first sample value
    float val = lastData[i] - lastData[0]; //remove first sample lastData[0] as offset
    dValue = val*dScaleY;
    float newY = y_base-dValue;
    qSamplePosition.setY(newY);

    lastPath.moveTo(qSamplePosition);

    for(i += sample_offset; i < lastData.size(); ++i) {
        val = lastData[i] - lastData[0]; //remove first sample lastData[0] as offset
        dValue = val*dScaleY;

        newY = y_base-dValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(lastPath.currentPosition().x()+dDx);

        lastPath.lineTo(qSamplePosition);

        //Create ellipse position
        if(i == (qint32)(markerPosition.x()/dDx)) {
            ellipsePos.setX(lastPath.currentPosition().x()+dDx);
            ellipsePos.setY(newY+(option.rect.height()/2));

            amplitude = QString::number(lastData[i]);
        }
    }
}

//=============================================================================================================

void RtFiffRawViewDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch(index.column()) {
        case 0: { //chnames
            painter->save();

            painter->rotate(-90);
            painter->drawText(QRectF(-option.rect.y()-t_fPlotHeight,0,t_fPlotHeight,20),Qt::AlignCenter,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }

        case 1: { //data plot
            QBrush backgroundBrush = index.model()->data(index, Qt::BackgroundRole).value<QBrush>();
            bool bIsBadChannel = index.model()->data(index.model()->index(index.row(), 2), Qt::DisplayRole).toBool();

            // Plot background based on user chosen color
            // This is a rather ugly hack in order to cope with QOpenGLWidget's/QtableView's problem when setting a background color
            if (index.row() == m_iUpperItemIndex) {
                painter->save();
                painter->setBrushOrigin(option.rect.topLeft());
                QRect rect = option.rect;
                rect.setHeight(2000);
                painter->fillRect(rect, backgroundBrush);
                painter->restore();
            }

            // Draw special background when channel is marked as bad
            if(bIsBadChannel) {
                painter->save();
                QBrush brush(QColor(254,74,93,40));
                painter->setBrushOrigin(option.rect.topLeft());
                painter->fillRect(option.rect, brush);
                painter->restore();
            }

//            //Highlight selected channels
//            if(option.state & QStyle::State_Selected) {
//                QPointF oldBO = painter->brushOrigin();
//                painter->setBrushOrigin(option.rect.topLeft());
//                painter->fillRect(option.rect, option.palette.highlight());
//                painter->setBrushOrigin(oldBO);
//            }

            //Get data
            QVariant variant = index.model()->data(index,Qt::DisplayRole);
            RowVectorPair data = variant.value<RowVectorPair>();

            const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

            if(data.second > 0) {
                QPainterPath path(QPointF(option.rect.x(),option.rect.y()));

//                //Plot hovering marker
//                createMarkerPath(option, path);

//                painter->save();
//                painter->setPen(m_penMarker);
//                painter->drawPath(path);
//                painter->restore();

                //Plot grid
                createGridPath(index, option, path, data);

                painter->save();
                painter->setPen(m_penGrid);
                painter->drawPath(path);
                painter->restore();

                //Plot time spacers
                createTimeSpacersPath(index, option, path, data);

                painter->save();
                painter->setPen(m_penTimeSpacers);
                painter->drawPath(path);
                painter->restore();

                //Plot detected triggers
                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));
                painter->save();
                createTriggerPath(painter, index, option, path, data);
                painter->restore();

                //Plot trigger threshold
                if(index.row() == t_pModel->getCurrentTriggerIndex() &&
                        t_pModel->triggerDetectionActive()) {
                    path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));
                    QPointF textPosition;
                    createTriggerThresholdPath(index, option, path, data, textPosition);

                    painter->save();
                    painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
                    painter->drawPath(path);
                    painter->drawText(textPosition, QString("%1 Threshold").arg(t_pModel->getTriggerName()));
                    painter->restore();
                }

                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));

                //Plot data path
                createPlotPath(index, option, path, data);

                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->save();
                painter->translate(0, t_fPlotHeight/2);

                if(bIsBadChannel) {
                    if(t_pModel->isFreezed()) {
                        if(option.state & QStyle::State_Selected)
                            painter->setPen(m_penFreezeSelectedBad);
                        else
                            painter->setPen(m_penFreezeBad);
                    } else {
                        if(option.state & QStyle::State_Selected)
                            painter->setPen(m_penNormalSelectedBad);
                        else
                            painter->setPen(m_penNormalBad);
                    }
                } else {
                    if(t_pModel->isFreezed()) {
                        if(option.state & QStyle::State_Selected)
                            painter->setPen(m_penFreezeSelected);
                        else
                            painter->setPen(m_penFreeze);
                    } else {
                        if(option.state & QStyle::State_Selected)
                            painter->setPen(m_penNormalSelected);
                        else
                            painter->setPen(m_penNormal);
                    }
                }

                painter->drawPath(path);
                painter->restore();

                //Plot current position marker
                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));
                createCurrentPositionMarkerPath(index, option, path);

                painter->save();
                painter->setPen(m_penMarker);
                painter->drawPath(path);
                painter->restore();

                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));
                createMarkerPath(index, option, path);

                painter->save();
                painter->setPen(QPen(Qt::green, 1, Qt::SolidLine));
                painter->drawPath(path);
                painter->restore();
            }
            break;
        }
    }
}

//=============================================================================================================

QSize RtFiffRawViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QSize size = option.rect.size();

    switch(index.column()) {
    case 0:
        size = QSize(20,option.rect.height());
        break;
    case 1:
        QList< QVector<float> > data = index.model()->data(index).value< QList<QVector<float> > >();
//        qint32 nsamples = (static_cast<const RtFiffRawViewModel*>(index.model()))->lastSample()-(static_cast<const RtFiffRawViewModel*>(index.model()))->firstSample();
//        size = QSize(nsamples*m_dDx,m_dPlotHeight);
        Q_UNUSED(option);
        break;
    }

    return size;
}

//=============================================================================================================

void RtFiffRawViewDelegate::markerMoved(QPoint position,
                                        int activeRow)
{
    m_markerPosition = position;
    m_iActiveRow = activeRow;
}

//=============================================================================================================

void RtFiffRawViewDelegate::setSignalColor(const QColor& signalColor)
{
    m_penNormal.setColor(signalColor);
    m_penNormalBad.setColor(signalColor);
}

//=============================================================================================================

QColor RtFiffRawViewDelegate::getSignalColor()
{
    return m_penNormal.color();
}

//=============================================================================================================

void RtFiffRawViewDelegate::setUpperItemIndex(int iUpperItemIndex)
{
    m_iUpperItemIndex = iUpperItemIndex;
}

//=============================================================================================================

void RtFiffRawViewDelegate::createPlotPath(const QModelIndex &index,
                                           const QStyleOptionViewItem &option,
                                           QPainterPath& path,
                                           const RowVectorPair &data) const
{
    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());
    int iPlotSizePx = option.rect.width();
    int iNumSamples = t_pModel->getMaxSamples();
    double dPixelsPerSample = static_cast<double>(iPlotSizePx) / static_cast<double>(iNumSamples);

    // locate time-cursor
    int iTimeCursorSample = t_pModel->getCurrentSampleIndex();
    double firstValuePreviousPlot = t_pModel->getLastBlockFirstValue(index.row());

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    double dMaxYValueEstimate = t_pModel->getMaxValueFromRawViewModel(index.row());
    double dScaleY = option.rect.height()/(2 * dMaxYValueEstimate);
    double dChannelOffset = path.currentPosition().y();

//    qDebug() << " - - - - - - - - - - - - - - - - - - - - - ";
//    qDebug() << " iPlotSizePx = " <<      iPlotSizePx;
//    qDebug() << " iNumSamples = " <<      iNumSamples;
//    qDebug() << " dPixelsPerSample = " << dPixelsPerSample;
//    qDebug() << " iTimeCursorSample = " << iTimeCursorSample;
//    qDebug() << " firstValuePreviousPlot = " << firstValuePreviousPlot;

//    qDebug() << " dMaxYValueEstimate = " << dMaxYValueEstimate;
//    qDebug() << " dScaleY = " << dScaleY;
//    qDebug() << " dChannelOffset = " << dChannelOffset;

    // Move to initial starting point
    path.moveTo(calcPoint(path, 0., 0., dChannelOffset, dScaleY));
    double dY(0);

    // The plot works as a rolling time-cursor, ploting data on top of previous
    // runs. You always plot one whole window of data, between first sample and
    // numSamplesToPlot (or data.second) Even if the only change is a new block
    // of samples to the left of the time-cursor. At any time, to the left of
    // the time-cursor you have the most recent data (A part) corresponding to
    // the current roll. To the right of the time-cursor, you have data
    // corresponding to the previous roll.
    for (int j = 0; j < data.second; ++j) {
        dY = data.first[j];
        path.lineTo(calcPoint(path, dPixelsPerSample, dY,
                              dChannelOffset, dScaleY));
    }
}

//=============================================================================================================

void RtFiffRawViewDelegate::createCurrentPositionMarkerPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path) const
{
    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    float currentSampleIndex = option.rect.x()+t_pModel->getCurrentSampleIndex();
    float dDx = ((float)option.rect.width()) / t_pModel->getMaxSamples();
    currentSampleIndex = currentSampleIndex*dDx;

    float yStart = option.rect.topLeft().y();
    float yEnd = option.rect.bottomRight().y();

    path.moveTo(currentSampleIndex,yStart);
    path.lineTo(currentSampleIndex,yEnd);
}

//=============================================================================================================

void RtFiffRawViewDelegate::createGridPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data) const
{
    Q_UNUSED(data)

    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    if(t_pModel->numVLines() > 0)
    {
        //vertical lines
        float distance = float (option.rect.width())/(t_pModel->numVLines()+1);

        float yStart = option.rect.topLeft().y();

        float yEnd = option.rect.bottomRight().y();

        for(qint8 i = 0; i < t_pModel->numVLines(); ++i) {
            float x = distance*(i+1);
            path.moveTo(x,yStart);
            path.lineTo(x,yEnd);
        }
    }
}

//=============================================================================================================

void RtFiffRawViewDelegate::createTimeSpacersPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data) const
{
    Q_UNUSED(data)

    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    if(t_pModel->getNumberOfTimeSpacers() > 0)
    {
        //vertical lines
        float distanceSec = float (option.rect.width())/(t_pModel->numVLines()+1);
        float distanceSpacers = distanceSec/(t_pModel->getNumberOfTimeSpacers()+1);

        float yStart = option.rect.topLeft().y();

        float yEnd = option.rect.bottomRight().y();

        for(qint8 t = 0; t < t_pModel->numVLines()+1; ++t) {
            for(qint8 i = 0; i < t_pModel->getNumberOfTimeSpacers(); ++i) {
                float x = (distanceSec*t)+(distanceSpacers*(i+1));
                path.moveTo(x,yStart);
                path.lineTo(x,yEnd);
            }
        }
    }
}

//=============================================================================================================

void RtFiffRawViewDelegate::createTriggerPath(QPainter *painter,
                                              const QModelIndex &index,
                                              const QStyleOptionViewItem &option,
                                              QPainterPath& path,
                                              RowVectorPair &data) const
{
    Q_UNUSED(data)
    Q_UNUSED(path)

    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    QList<QPair<int,double> > detectedTriggers = t_pModel->getDetectedTriggers();
    QList<QPair<int,double> > detectedTriggersOld = t_pModel->getDetectedTriggersOld();
    QMap<double, QColor> mapTriggerTypeColors = t_pModel->getTriggerColor();

    float yStart = option.rect.topLeft().y();
    float yEnd = option.rect.bottomRight().y();
    float dDx = ((float)option.rect.width()) / t_pModel->getMaxSamples();

    int currentSampleIndex = t_pModel->getCurrentSampleIndex();

    //Newly detected triggers
    for(int u = 0; u < detectedTriggers.size(); ++u) {
        QPainterPath path;

        int triggerPos = detectedTriggers[u].first;

        painter->save();
        if(mapTriggerTypeColors.contains(detectedTriggers[u].second)) {
            painter->setPen(QPen(mapTriggerTypeColors[detectedTriggers[u].second], 1.5, Qt::SolidLine));
        }

        if(triggerPos <= currentSampleIndex + t_pModel->getCurrentOverlapAddDelay()) {
            path.moveTo(triggerPos*dDx,yStart);
            path.lineTo(triggerPos*dDx,yEnd);
        }

        painter->drawPath(path);
        painter->restore();
    }

    //Old detected triggers
    for(int u = 0; u < detectedTriggersOld.size(); ++u) {
        QPainterPath path;

        int triggerPos = detectedTriggersOld[u].first;

        if(triggerPos >= currentSampleIndex + t_pModel->getCurrentOverlapAddDelay()) {
            painter->save();

            if(mapTriggerTypeColors.contains(detectedTriggersOld[u].second)) {
                painter->setPen(QPen(mapTriggerTypeColors[detectedTriggersOld[u].second], 1.5, Qt::SolidLine));
            }

            path.moveTo(triggerPos*dDx,yStart);
            path.lineTo(triggerPos*dDx,yEnd);

            painter->drawPath(path);
            painter->restore();
        }
    }
}

//=============================================================================================================

void RtFiffRawViewDelegate::createTriggerThresholdPath(const QModelIndex &index,
                                                       const QStyleOptionViewItem &option,
                                                       QPainterPath& path,
                                                       RowVectorPair &data,
                                                       QPointF &textPosition) const
{
    Q_UNUSED(data)

    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = t_pModel->getKind(index.row());
    double dMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_STIM_CH: {
            dMaxValue = 5.0;
            if(t_pModel->getScaling().contains(FIFFV_STIM_CH))
                dMaxValue = t_pModel->getScaling()[FIFFV_STIM_CH];
            break;
        }
    }

    double dScaleY = option.rect.height()/(2*dMaxValue);
    double triggerThreshold = -1*(t_pModel->getTriggerThreshold());

    path.moveTo(option.rect.topLeft().x(), option.rect.topLeft().y()+option.rect.height()/2+dScaleY*triggerThreshold);
    path.lineTo(option.rect.topRight().x(), option.rect.topLeft().y()+option.rect.height()/2+dScaleY*triggerThreshold);

    textPosition = QPointF(option.rect.topLeft().x()+5, option.rect.topLeft().y()+option.rect.height()/2+dScaleY*triggerThreshold-5);
}

//=============================================================================================================

void RtFiffRawViewDelegate::createMarkerPath(const QModelIndex &index,
                                             const QStyleOptionViewItem &option,
                                             QPainterPath& path) const
{
    const RtFiffRawViewModel* t_pModel = static_cast<const RtFiffRawViewModel*>(index.model());

    int iOffset = t_pModel->getFirstSampleOffset();
    int iTimeCursorSample = t_pModel->getCurrentSampleIndex();
    int iMaxSample = t_pModel->getMaxSamples();

    double dDx = static_cast<double>(option.rect.width()) / static_cast<double>(iMaxSample);

    float yStart = option.rect.topLeft().y();
    float yEnd = option.rect.bottomRight().y();

    int iEarliestDrawnSample = iOffset - iMaxSample + iTimeCursorSample;
    int iLatestDrawnSample = iOffset + iTimeCursorSample;

    auto events = t_pModel->getEventsToDisplay(iEarliestDrawnSample, iLatestDrawnSample);

    for(auto& e : *events)
    {
        int iEventSample = e.sample;
        int iLastStartingSample = iOffset - iMaxSample;
        int iDrawPositionInSamples = (iEventSample - iLastStartingSample) % iMaxSample;

        float iPositionInPixels = static_cast<float>(iDrawPositionInSamples) * dDx;

        path.moveTo(iPositionInPixels,yStart);
        path.lineTo(iPositionInPixels,yEnd);
    }
}

//=============================================================================================================

QPointF RtFiffRawViewDelegate::calcPoint(QPainterPath& path,
                                      const double dx,
                                      const double y,
                                      const double yBase,
                                      const double yScale) const
{
    double x = path.currentPosition().x() + dx;
    double yScaled = -((yScale * y) - yBase);//- because y pixels grow downwards.

//    qDebug() << " x = " << x;
//    qDebug() << " y = " << yScaled;

    return QPointF(x, yScaled);
}

//=============================================================================================================
