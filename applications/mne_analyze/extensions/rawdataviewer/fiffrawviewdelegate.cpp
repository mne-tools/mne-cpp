//=============================================================================================================
/**
 * @file     fiffrawviewdelegate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     January, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffrawviewdelegate.h"

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Utils/metatypes.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QPainter>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWEREXTENSION;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawViewDelegate::FiffRawViewDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{

}


//*************************************************************************************************************

void FiffRawViewDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch(index.column()) {
        case 1: { //data plot
            QBrush backgroundBrush = index.model()->data(index, Qt::BackgroundRole).value<QBrush>();
            bool bIsBadChannel = index.model()->data(index.model()->index(index.row(), 2), Qt::DisplayRole).toBool();

            // Draw special background when channel is marked as bad
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

                createPlotPath(option, path, data, pFiffRawModel->pixelDifference());

                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->save();
                painter->translate(0, t_fPlotHeight/2);

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
            }
            break;
        }
    }
}


//*************************************************************************************************************

QSize FiffRawViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
        case 1:
            const FiffRawViewModel* pFiffRawModel = static_cast<const FiffRawViewModel*>(index.model());
            qint32 nsamples = pFiffRawModel->absoluteLastSample() - pFiffRawModel->absoluteFirstSample();

            size = QSize(nsamples*pFiffRawModel->pixelDifference(), option.rect.height());
            break;
    }

    Q_UNUSED(option);

    return size;
}


//*************************************************************************************************************

void FiffRawViewDelegate::createPlotPath(const QStyleOptionViewItem &option,
                                         QPainterPath& path,
                                         ChannelData& data,
                                         double dDx) const
{
    double dMaxValue = 1.0e-10;
    double dScaleY = option.rect.height()/(2*dMaxValue);

    double y_base = path.currentPosition().y();

    QPointF qSamplePosition;
    double dValue, newY;
    int iStep = floor(1.0/dDx);
    if(iStep < 1) {
        iStep = 1;
    }

    for(qint32 j=0; j < data.size(); j+=iStep) {
        dValue = data[j]*dScaleY;

        //Reverse direction -> plot the right way
        newY = y_base-dValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x()+1.0);
        path.lineTo(qSamplePosition);
    }
}

