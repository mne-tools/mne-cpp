//=============================================================================================================
/**
 * @file     fiffrawviewdelegate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the FiffRawViewDelegate Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffrawviewdelegate.h"

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/annotationmodel.h>
#include <anShared/Utils/metatypes.h>

#include <disp/viewers/scalingview.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QPainter>
#include <QPainterPath>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawViewDelegate::FiffRawViewDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
, m_iUpperItemIndex(0)
{

}

//=============================================================================================================

void FiffRawViewDelegate::setUpperItemIndex(int iUpperItemIndex)
{
    m_iUpperItemIndex = iUpperItemIndex;
}

//=============================================================================================================

void FiffRawViewDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch(index.column()) {
        case 1: { //data plot
            QBrush backgroundBrush = index.model()->data(index, Qt::BackgroundRole).value<QBrush>();

            // Plot background based on user chosen color
            // This is a rather ugly hack in order to cope with QOpenGLWidget's/QtableView's problem when setting a background color
            if(index.row() == m_iUpperItemIndex) {
                // Plot background based on user chosen color
                // This is a rather ugly hack in order to cope with QOpenGLWidget's/QtableView's problem when setting a background color
                painter->save();
                painter->setBrushOrigin(option.rect.topLeft());
                QRect rect = option.rect;
                rect.setHeight(2000);
                painter->fillRect(rect, backgroundBrush);
                painter->restore();
            }

            // Draw special background when channel is marked as bad
            bool bIsBadChannel = index.model()->data(index.model()->index(index.row(), 2), Qt::DisplayRole).toBool();

            if(bIsBadChannel) {
                painter->save();
                QBrush brush(QColor(254,74,93,40));
                painter->setBrushOrigin(option.rect.topLeft());
                painter->fillRect(option.rect, brush);
                painter->restore();
            }

            //Get data
            QVariant variant = index.model()->data(index,Qt::DisplayRole);
            ChannelData data = variant.value<ChannelData>();

            if(data.size() > 0) {
                //Plot data path
                const FiffRawViewModel* pFiffRawModel = static_cast<const FiffRawViewModel*>(index.model());

                int pos = pFiffRawModel->pixelDifference() * (pFiffRawModel->currentFirstSample() - pFiffRawModel->absoluteFirstSample());

                QPainterPath path = QPainterPath(QPointF(option.rect.x()+pos, option.rect.y()));

                //Plot data
                createPlotPath(option,
                               path,
                               data,
                               pFiffRawModel->pixelDifference(),
                               index);

                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->save();
                painter->translate(0, t_fPlotHeight/2);

                //Set colors
                if(bIsBadChannel) {
                    if(option.state & QStyle::State_Selected)
                        painter->setPen(m_penNormalSelectedBad);
                    else
                        painter->setPen(m_penNormalBad);
                } else {
                    if(option.state & QStyle::State_Selected)
                        painter->setPen(m_penNormalSelected);
                    else
                        painter->setPen(m_penNormal);
                }

                painter->drawPath(path);
                painter->restore();

                path = QPainterPath(QPointF(option.rect.x()+pos, option.rect.y()));

                //Plot time spacers
                createTimeSpacersPath(index,
                                      option,
                                      path,
                                      data);

                painter->save();
                painter->setPen(QPen(m_penNormal.color().darker(350), 1, Qt::DashLine));
                painter->drawPath(path);
                painter->restore();

                if(pFiffRawModel->shouldDisplayAnnotation()) {
                    path = QPainterPath(QPointF(option.rect.x()+pos, option.rect.y()));

                    painter->setPen(QPen(m_penNormal.color().darker(250), 1, Qt::SolidLine));

                    //Plot time marks
                    createMarksPath(index,
                                    option,
                                    path,
                                    data,
                                    painter);
//                    painter->save();
//                    painter->setPen(QPen(m_penNormal.color().darker(250), 2, Qt::SolidLine));
//                    painter->drawPath(path);
//                    painter->restore();
                }
            }
            break;
        }
    }
}

//=============================================================================================================

QSize FiffRawViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
        case 1:
            const FiffRawViewModel* pFiffRawModel = static_cast<const FiffRawViewModel*>(index.model());
            qint32 nsamples = pFiffRawModel->absoluteLastSample() - pFiffRawModel->absoluteFirstSample();

            nsamples *= pFiffRawModel->pixelDifference();

            size = QSize(nsamples, option.rect.height());

            break;
    }

    return size;
}

//=============================================================================================================

void FiffRawViewDelegate::createPlotPath(const QStyleOptionViewItem &option,
                                         QPainterPath& path,
                                         ChannelData& data,
                                         double dDx,
                                         const QModelIndex &index) const
{

    const FiffRawViewModel* t_pModel = static_cast<const FiffRawViewModel*>(index.model());

    double dMaxValue = DISPLIB::getScalingValue(t_pModel->getScaling(), t_pModel->getKind(index.row()), t_pModel->getUnit(index.row()));
    double dScaleY = option.rect.height()/(2*dMaxValue);
    double y_base = path.currentPosition().y();
    double dValue, newY;

    QPointF qSamplePosition;

    //Deactivate downsampling for now due to aliasing effects
//    int iPaintStep = (int)(1.0/dDx) - 1;
//    if (iPaintStep < 2){
//        iPaintStep = 1;
//    }
    int iPaintStep = 1;

    for(unsigned int j = 0; j < data.size(); j = j + iPaintStep) {
        dValue = data[j] * dScaleY;

        //Reverse direction -> plot the right way
        newY = y_base - dValue;
        //qDebug() << "data:" << dValue;
        qSamplePosition.setY(newY);

        // Multiply by dDx because we need to take into account different visible window sizes specified by the user.
        // The spacing we use to paint the samples is therefore dependent on how much data we plot (in samples) in
        // the GUI view with user selected width (in pixels). This relationship is calculated in the FiffRawView
        // and is reflected by dDx
        qSamplePosition.setX(path.currentPosition().x() + (dDx * (float)iPaintStep));

        path.lineTo(qSamplePosition);
    }
//    for(int j = iPaintStep; j < data.size(); j = j + iPaintStep) {
//        dValue = data[j] * dScaleY;
//        diff = dValue - (data[j - iPaintStep] * dScaleY);

//        //Reverse direction -> plot the right way

//        qSamplePosition.setY((path.currentPosition().y() - diff));
//        qSamplePosition.setX(path.currentPosition().x() + (dDx * (float)iPaintStep));
//        path.lineTo(qSamplePosition);
//    }
}

//=============================================================================================================

void FiffRawViewDelegate::setSignalColor(const QColor& signalColor)
{
    m_penNormal.setColor(signalColor);
    m_penNormalBad.setColor(signalColor);
}

//=============================================================================================================

void FiffRawViewDelegate::createTimeSpacersPath(const QModelIndex &index,
                                                const QStyleOptionViewItem &option,
                                                QPainterPath& path,
                                                ANSHAREDLIB::ChannelData &data) const
{
    Q_UNUSED(data);

    const FiffRawViewModel* t_pModel = static_cast<const FiffRawViewModel*>(index.model());

    double dDx = t_pModel->pixelDifference();

    float iSpacersPerSecond = t_pModel->getNumberOfTimeSpacers();
    float fSampFreq = t_pModel->getFiffInfo()->sfreq;
    float fTop = option.rect.topLeft().y();
    float fBottom = option.rect.bottomRight().y();

    for(int j = 0; j < (1.5 * iSpacersPerSecond * t_pModel->getTotalBlockCount()); j++) {
        //draw vertical line
        path.moveTo(path.currentPosition().x(), fTop);
        path.lineTo(path.currentPosition().x(), fBottom);

        //jump to next place to draw
        path.moveTo(path.currentPosition().x() + ((dDx * fSampFreq) / iSpacersPerSecond), fTop);
    }
}

//=============================================================================================================

void FiffRawViewDelegate::createMarksPath(const QModelIndex &index,
                                          const QStyleOptionViewItem &option,
                                          QPainterPath &path,
                                          ANSHAREDLIB::ChannelData &data,
                                          QPainter* painter) const
{
    const FiffRawViewModel* t_pModel = static_cast<const FiffRawViewModel*>(index.model());
    QSharedPointer<AnnotationModel> t_pAnnModel = t_pModel->getAnnotationModel();

    double dDx = t_pModel->pixelDifference();

    int iStart = t_pModel->currentFirstSample();

    float fTop = option.rect.topLeft().y();
    float fBottom = option.rect.bottomRight().y();
    float fInitX = path.currentPosition().x();

    //QMap<int, QColor> typeColor = t_pAnnModel->getTypeColors();
    QMap<int, QColor> groupColor = t_pAnnModel->getGroupColors();

    for(int i = 0; i < t_pModel->getTimeListSize(); i++) {
        unsigned int uiTime = t_pModel->getTimeMarks(i);
        if ((t_pModel->getTimeMarks(i) > iStart) && (uiTime < (iStart + data.size()))) {
//            int type = t_pAnnModel->data(t_pAnnModel->index(i,2)).toInt();
//            painter->setPen(QPen(typeColor.value(type), Qt::black));
            int group = t_pAnnModel->currentGroup(i);
            painter->setPen(QPen(groupColor.value(group), 1, Qt::SolidLine));
            painter->drawLine(fInitX + static_cast<float>(t_pModel->getTimeMarks(i) - iStart) * dDx,
                              fTop,
                              fInitX + static_cast<float>(t_pModel->getTimeMarks(i) - iStart) * dDx,
                              fBottom);
        }
    }
}
