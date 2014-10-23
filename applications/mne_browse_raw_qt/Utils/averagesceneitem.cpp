//=============================================================================================================
/**
* @file     ChannelSceneItem.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     June, 2014
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
* @brief    Contains the implementation of the ChannelSceneItem class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagesceneitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSceneItem::AverageSceneItem(QString channelName, int channelNumber, QPointF channelPosition, QColor defaultColors)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_qpChannelPosition(channelPosition)
{
}


//*************************************************************************************************************

QRectF AverageSceneItem::boundingRect() const
{
    return QRectF(-25, -35, 50, 70);
}


//*************************************************************************************************************

void AverageSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Plot channel name
    QStaticText staticElectrodeName = QStaticText(m_sChannelName);
    QSizeF sizeText = staticElectrodeName.size();
    painter->drawStaticText(-15+((30-sizeText.width())/2), -32, staticElectrodeName);

    //Plot average data
    painter->save();

    for(int i = 0; i<m_lAverageData.size(); i++) {
        path = QPainterPath(QPointF(option.rect.x()+t_rawModel->relFiffCursor(),option.rect.y()));
        QPen pen;
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(1);
        painter->setPen(pen);

        createPlotPath(path);

        painter->drawPath(path);
    }

    painter->restore();

    //set posistion
    this->setPos(10*m_qpChannelPosition.x(), -10*m_qpChannelPosition.y());
}


//*************************************************************************************************************

void AverageSceneItem::createPlotPath(path)
{
    QMap<QString,double> scaleMap = m_pScaleWindow->getScalingMap();

    double dMaxValue =1e-09;

//    switch(kind) {
//        case FIFFV_MEG_CH: {
//            qint32 unit = (static_cast<const RawModel*>(index.model()))->m_pfiffIO->m_qlistRaw[0]->info.chs[index.row()].unit;
//            if(unit == FIFF_UNIT_T_M) {
//                dMaxValue = scaleMap["MEG_grad"];
//            }
//            else if(unit == FIFF_UNIT_T)
//                dMaxValue = scaleMap["MEG_mag"];
//            break;
//        }
//        case FIFFV_EEG_CH: {
//            dMaxValue = scaleMap["MEG_EEG"];
//            break;
//        }
//        case FIFFV_EOG_CH: {
//            dMaxValue = scaleMap["MEG_EOG"];
//            break;
//        }
//        case FIFFV_STIM_CH: {
//            dMaxValue = scaleMap["MEG_STIM"];
//            break;
//        }
//        case FIFFV_EMG_CH: {
//            dMaxValue = scaleMap["MEG_EMG"];
//            break;
//        }
//        case FIFFV_MISC_CH: {
//            dMaxValue = scaleMap["MEG_MISC"];
//            break;
//        }
//    }

    double dValue;
    double dScaleY = this->boundingRect().height()/(2*dMaxValue);

    double y_base = -path.currentPosition().y();
    QPointF qSamplePosition;

    //plot all rows from list of pairs
    for(qint8 i=0; i < listPairs.size(); ++i) {
        //create lines from one to the next sample
        for(qint32 j=0; j < listPairs[i].second; ++j)
        {
            double val = *(listPairs[i].first+j);

            //subtract mean of the channel here (if wanted by the user)
            dValue = (val - channelMean)*dScaleY;

            double newY = y_base+dValue;

            qSamplePosition.setY(-newY);
            qSamplePosition.setX(path.currentPosition().x()+m_dDx);

            path.lineTo(qSamplePosition);
        }
    }


}










