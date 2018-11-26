//=============================================================================================================
/**
* @file     ChannelDataItem.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     October, 2014
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
* @brief    Definition of the ChannelDataItem class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldataitem.h"

#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QStaticText>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataItem::ChannelDataItem(int iChannelKind,
                                 int iChannelUnit,
                                 const QColor& color)
: m_color(color)
, m_iChannelKind(iChannelKind)
, m_iChannelUnit(iChannelUnit)
, m_iIterator(0)
{
    m_rectBoundingRect = QRectF(0.0,0.0,1000.0,10.0);
}


//*************************************************************************************************************

QRectF ChannelDataItem::boundingRect() const
{
    return m_rectBoundingRect;
}


//*************************************************************************************************************

void ChannelDataItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

void ChannelDataItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

void ChannelDataItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    //Plot average data
    painter->save();
    paintDataPath(painter);
    painter->restore();

}


//*************************************************************************************************************

void ChannelDataItem::addData(const Eigen::RowVectorXd& data)
{
    if(m_data.size() < 180) {
        m_data.append(data);
        m_iIterator++;
    } else {
        if(m_iIterator < 180) {
            m_data[m_iIterator] = data;
            m_iIterator++;
        } else {
            m_data[0] = data;
            m_iIterator = 1;
        }
    }

    //this->update();
}


//*************************************************************************************************************

void ChannelDataItem::paintDataPath(QPainter *painter)
{
    if(m_data.isEmpty()) {
        return;
    }

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    float dMaxValue = 1e-9f;

    switch(m_iChannelKind) {
        case FIFFV_MEG_CH: {
            if(m_iChannelUnit == FIFF_UNIT_T_M) { //gradiometers
                dMaxValue = 1e-10f;               
            }
            else if(m_iChannelUnit == FIFF_UNIT_T) //magnitometers
            {
                dMaxValue = 1e-11f;
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {
            dMaxValue = 1e-11f;
            break;
        }
        case FIFFV_EEG_CH: {
            dMaxValue = 1e-4f;
            break;
        }
        case FIFFV_EOG_CH: {
            dMaxValue = 1e-3f;
            break;
        }
        case FIFFV_STIM_CH: {
            dMaxValue = 5;
            break;
        }
        case FIFFV_MISC_CH: {
            dMaxValue = 1e-3f;
            break;
        }
    }

    //Plot data
    QRectF boundingRect = this->boundingRect();
    double dScaleY = (boundingRect.height())/(2*dMaxValue);

    //Create path
    QPainterPath path = QPainterPath(QPointF(boundingRect.x(), boundingRect.y() + boundingRect.height()/2));
    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(m_color);
    pen.setWidthF(2);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, true);

    int counter = 0;

    for(int i = 0; i < m_data.size(); i++) {
        for(int k = 0; k < m_data.at(i).cols(); k++) {
            path.lineTo(QPointF(double(counter)/18.0, dScaleY * (m_data.at(i)(k)-m_data.first()(0))));
            counter++;
        }
    }

    painter->drawPath(path);
}
