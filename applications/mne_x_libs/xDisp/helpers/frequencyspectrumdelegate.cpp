//=============================================================================================================
/**
* @file     frequencyspectrumdelegate.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FrequencySpectrumDelegate Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrumdelegate.h"

#include "frequencyspectrummodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumDelegate::FrequencySpectrumDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{

}


//*************************************************************************************************************

void FrequencySpectrumDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    switch(index.column()) {
        case 0: { //chnames
            painter->save();

            painter->rotate(-90);
            painter->drawText(QRectF(-option.rect.y()-t_fPlotHeight,0,t_fPlotHeight,20),Qt::AlignCenter,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 1: { //data plot
            painter->save();

            //draw special background when channel is marked as bad
//            QVariant v = index.model()->data(index,Qt::BackgroundRole);
//            if(v.canConvert<QBrush>() && !(option.state & QStyle::State_Selected)) {
//                QPointF oldBO = painter->brushOrigin();
//                painter->setBrushOrigin(option.rect.topLeft());
//                painter->fillRect(option.rect, qvariant_cast<QBrush>(v));
//                painter->setBrushOrigin(oldBO);
//            }

//            //Highlight selected channels
//            if(option.state & QStyle::State_Selected) {
//                QPointF oldBO = painter->brushOrigin();
//                painter->setBrushOrigin(option.rect.topLeft());
//                painter->fillRect(option.rect, option.palette.highlight());
//                painter->setBrushOrigin(oldBO);
//            }

            //Get data
            QVariant variant = index.model()->data(index,Qt::DisplayRole);
            RowVectorXd data = variant.value< RowVectorXd >();


            const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

            if(data.size() > 0)
            {
                QPainterPath path(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor()-1,option.rect.y()));

                //Plot grid
                painter->setRenderHint(QPainter::Antialiasing, false);
                createGridPath(index, option, path, data);

                painter->save();
                QPen pen;
                pen.setStyle(Qt::DotLine);
                pen.setWidthF(0.5);
                painter->setPen(pen);
                painter->drawPath(path);
                painter->restore();

                //Plot data path
                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));

                createPlotPath(index, option, path, data);

                painter->save();
                painter->translate(0,t_fPlotHeight/2);
                painter->setRenderHint(QPainter::Antialiasing, true);

                if(option.state & QStyle::State_Selected)
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkRed : Qt::red, 1, Qt::SolidLine));
                else
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkGray : Qt::darkBlue, 1, Qt::SolidLine));

                painter->drawPath(path);
                painter->restore();
            }
            painter->restore();
            break;
        }
    }

}


//*************************************************************************************************************

QSize FrequencySpectrumDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
    case 0:
        size = QSize(20,option.rect.height());
        break;
    case 1:
        RowVectorXd data = index.model()->data(index).value< RowVectorXd >();
//        qint32 nsamples = (static_cast<const FrequencySpectrumModel*>(index.model()))->lastSample()-(static_cast<const FrequencySpectrumModel*>(index.model()))->firstSample();

//        size = QSize(nsamples*m_dDx,m_dPlotHeight);
        Q_UNUSED(option);
        break;
    }


    return size;
}


//*************************************************************************************************************

void FrequencySpectrumDelegate::createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorXd& data) const
{
    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    float fMaxValue = data.maxCoeff();

    float fValue;
    float fScaleY = option.rect.height()/(fMaxValue*0.5);

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    //Move to initial starting point
    if(data.size() > 0)
    {
        float val = 0;
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX((double)option.rect.width()*t_pModel->getFreqScale()[0]);

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < data.size(); ++i) {
        float val = data[i]-data[0]; //remove first sample data[0] as offset
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX((double)option.rect.width()*t_pModel->getFreqScale()[i]);

        path.lineTo(qSamplePosition);
    }
}


//*************************************************************************************************************

void FrequencySpectrumDelegate::createGridPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorXd& data) const
{
    Q_UNUSED(data)

    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    if(t_pModel->getInfo())
    {
        double fs = t_pModel->getInfo()->sfreq/2;

        qint32 numLines = (qint32)ceil(log10(fs));

        QList<qint32> qListLineSamples;

        for(qint32 lineIdx = 0; lineIdx < numLines; ++lineIdx)
        {
            double val = pow(10,lineIdx);
            qint32 idx = (qint32)floor((val/fs) * t_pModel->getNumStems());
            qListLineSamples.append(idx);
        }

        //horizontal lines
        float yStart = option.rect.topLeft().y();

        float yEnd = option.rect.bottomRight().y();

        for(qint32 i = 0; i < qListLineSamples.size(); ++i) {
            float x = (t_pModel->getFreqScale()[qListLineSamples[i]])*option.rect.width();
            path.moveTo(x,yStart);
            path.lineTo(x,yEnd);
        }
    }
}
