//=============================================================================================================
/**
* @file     deepviewerrenderer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    DeepViewerWidget class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "arthurwidgets.h"
#include "deepviewerrenderer.h"

#include <stdio.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepViewerRenderer::DeepViewerRenderer(QWidget *parent)
: ArthurFrame(parent)
{
    m_pointSize = 10;
    m_activePoint = -1;
    m_capStyle = Qt::FlatCap;
    m_joinStyle = Qt::BevelJoin;
    m_pathMode = CurveMode;
    m_penWidth = 1;
    m_penStyle = Qt::SolidLine;
    m_wasAnimated = false;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_AcceptTouchEvents);
}


//*************************************************************************************************************

void DeepViewerRenderer::paint(QPainter *painter)
{
    if (m_points.isEmpty())
        initializePoints();

    painter->setRenderHint(QPainter::Antialiasing);

    QPalette pal = palette();
    painter->setPen(Qt::NoPen);

    // Construct the path
    QPainterPath path;
    path.moveTo(m_points.at(0));

    if (m_pathMode == LineMode) {
        for (int i=1; i<m_points.size(); ++i)
            path.lineTo(m_points.at(i));
    } else {
        int i=1;
        while (i + 2 < m_points.size()) {
            path.cubicTo(m_points.at(i), m_points.at(i+1), m_points.at(i+2));
            i += 3;
        }
        while (i < m_points.size()) {
            path.lineTo(m_points.at(i));
            ++i;
        }
    }

    // Draw the path
    {
        QColor lg = Qt::red;

        // The "custom" pen
        if (m_penStyle == Qt::NoPen) {
            QPainterPathStroker stroker;
            stroker.setWidth(m_penWidth);
            stroker.setJoinStyle(m_joinStyle);
            stroker.setCapStyle(m_capStyle);

            QVector<qreal> dashes;
            qreal space = 4;
            dashes << 1 << space
                   << 3 << space
                   << 9 << space
                   << 27 << space
                   << 9 << space
                   << 3 << space;
            stroker.setDashPattern(dashes);
            QPainterPath stroke = stroker.createStroke(path);
            painter->fillPath(stroke, lg);

        } else {
            QPen pen(lg, m_penWidth, m_penStyle, m_capStyle, m_joinStyle);
            painter->strokePath(path, pen);
        }
    }

    if (1) {
        // Draw the control points
        painter->setPen(QColor(50, 100, 120, 200));
        painter->setBrush(QColor(200, 200, 210, 120));
        for (int i=0; i<m_points.size(); ++i) {
            QPointF pos = m_points.at(i);
            painter->drawEllipse(QRectF(pos.x() - m_pointSize,
                                       pos.y() - m_pointSize,
                                       m_pointSize*2, m_pointSize*2));
        }
        painter->setPen(QPen(Qt::lightGray, 0, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawPolyline(m_points);
    }

}


//*************************************************************************************************************

void DeepViewerRenderer::initializePoints()
{
    const int count = 7;
    m_points.clear();
    m_vectors.clear();

    QMatrix m;
    qreal rot = 360.0 / count;
    QPointF center(width() / 2, height() / 2);
    QMatrix vm;
    vm.shear(2, -1);
    vm.scale(3, 3);

    for (int i=0; i<count; ++i) {
        m_vectors << QPointF(.1f, .25f) * (m * vm);
        m_points << QPointF(0, 100) * m + center;
        m.rotate(rot);
    }
}


//*************************************************************************************************************

void DeepViewerRenderer::updatePoints()
{
    qreal pad = 10;
    qreal left = pad;
    qreal right = width() - pad;
    qreal top = pad;
    qreal bottom = height() - pad;

    Q_ASSERT(m_points.size() == m_vectors.size());
    for (int i=0; i<m_points.size(); ++i) {
        QPointF pos = m_points.at(i);
        QPointF vec = m_vectors.at(i);
        pos += vec;
        if (pos.x() < left || pos.x() > right) {
            vec.setX(-vec.x());
            pos.setX(pos.x() < left ? left : right);
        } if (pos.y() < top || pos.y() > bottom) {
            vec.setY(-vec.y());
            pos.setY(pos.y() < top ? top : bottom);
        }
        m_points[i] = pos;
        m_vectors[i] = vec;
    }
    update();
}


//*************************************************************************************************************

void DeepViewerRenderer::mousePressEvent(QMouseEvent *e)
{
    if (!m_fingerPointMapping.isEmpty())
        return;
    setDescriptionEnabled(false);
    m_activePoint = -1;
    qreal distance = -1;
    for (int i=0; i<m_points.size(); ++i) {
        qreal d = QLineF(e->pos(), m_points.at(i)).length();
        if ((distance < 0 && d < 8 * m_pointSize) || d < distance) {
            distance = d;
            m_activePoint = i;
        }
    }

    if (m_activePoint != -1) {
        m_wasAnimated = m_timer.isActive();
        setAnimation(false);
        mouseMoveEvent(e);
    }

    // If we're not running in small screen mode, always assume we're dragging
    m_mouseDrag = true;
    m_mousePress = e->pos();
}


//*************************************************************************************************************

void DeepViewerRenderer::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_fingerPointMapping.isEmpty())
        return;
    // If we've moved more then 25 pixels, assume user is dragging
    if (!m_mouseDrag && QPoint(m_mousePress - e->pos()).manhattanLength() > 25)
        m_mouseDrag = true;

    if (m_mouseDrag && m_activePoint >= 0 && m_activePoint < m_points.size()) {
        m_points[m_activePoint] = e->pos();
        update();
    }
}


//*************************************************************************************************************

void DeepViewerRenderer::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_fingerPointMapping.isEmpty())
        return;
    m_activePoint = -1;
    setAnimation(m_wasAnimated);
}


//*************************************************************************************************************

void DeepViewerRenderer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_timer.timerId()) {
        updatePoints();
    }
}


//*************************************************************************************************************

bool DeepViewerRenderer::event(QEvent *e)
{
    bool touchBegin = false;
    switch (e->type()) {
    case QEvent::TouchBegin:
        touchBegin = true;
    case QEvent::TouchUpdate:
    {
        const QTouchEvent *const event = static_cast<const QTouchEvent*>(e);
        const QList<QTouchEvent::TouchPoint> points = event->touchPoints();
        foreach (const QTouchEvent::TouchPoint &touchPoint, points) {
            const int id = touchPoint.id();
            switch (touchPoint.state()) {
            case Qt::TouchPointPressed:
            {
                // find the point, move it
                QSet<int> activePoints = QSet<int>::fromList(m_fingerPointMapping.values());
                int activePoint = -1;
                qreal distance = -1;
                const int pointsCount = m_points.size();
                for (int i=0; i<pointsCount; ++i) {
                    if (activePoints.contains(i))
                        continue;

                    qreal d = QLineF(touchPoint.pos(), m_points.at(i)).length();
                    if ((distance < 0 && d < 12 * m_pointSize) || d < distance) {
                        distance = d;
                        activePoint = i;
                    }
                }
                if (activePoint != -1) {
                    m_fingerPointMapping.insert(touchPoint.id(), activePoint);
                    m_points[activePoint] = touchPoint.pos();
                }
                break;
            }
            case Qt::TouchPointReleased:
            {
                // move the point and release
                QHash<int,int>::iterator it = m_fingerPointMapping.find(id);
                m_points[it.value()] = touchPoint.pos();
                m_fingerPointMapping.erase(it);
                break;
            }
            case Qt::TouchPointMoved:
            {
                // move the point
                const int pointIdx = m_fingerPointMapping.value(id, -1);
                if (pointIdx >= 0)
                    m_points[pointIdx] = touchPoint.pos();
                break;
            }
            default:
                break;
            }
        }
        if (m_fingerPointMapping.isEmpty()) {
            e->ignore();
            return false;
        } else {
            if (touchBegin) {
                m_wasAnimated = m_timer.isActive();
                setAnimation(false);
            }
            update();
            return true;
        }
    }
        break;
    case QEvent::TouchEnd:
        if (m_fingerPointMapping.isEmpty()) {
            e->ignore();
            return false;
        }
        m_fingerPointMapping.clear();
        setAnimation(m_wasAnimated);
        return true;
        break;
    default:
        break;
    }
    return QWidget::event(e);
}


//*************************************************************************************************************

void DeepViewerRenderer::setAnimation(bool animation)
{
    m_timer.stop();

    if (animation) {
        m_timer.start(25, this);
    }
}
