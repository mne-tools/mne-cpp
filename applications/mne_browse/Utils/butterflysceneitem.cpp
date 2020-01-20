//=============================================================================================================
/**
 * @file     butterflysceneitem.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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
    m_scaleMap["MEG_grad"] = 400 * 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
    m_scaleMap["MEG_mag"] = 1.2 * 1e-12;
    m_scaleMap["MEG_EEG"] = 30 * 1e-06;
    m_scaleMap["MEG_EOG"] = 150 * 1e-06;
    m_scaleMap["MEG_EMG"] = 1 * 1e-03;
    m_scaleMap["MEG_ECG"] = 1 * 1e-03;
    m_scaleMap["MEG_MISC"] = 1 * 1;
    m_scaleMap["MEG_STIM"] = 5 * 1;
}


//*************************************************************************************************************

QRectF ButterflySceneItem::boundingRect() const
{
    int height = 100;
    int width = 500;
    return QRectF(-width/2, -height/2, width, height);
}


//*************************************************************************************************************

void ButterflySceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //Plot bounding rect / drawing region of this item
    //painter->drawRect(this->boundingRect());

    painter->setRenderHint(QPainter::Antialiasing, true);

    //Plot average data
    painter->save();
    paintAveragePaths(painter);
    painter->restore();
}


//*************************************************************************************************************

void ButterflySceneItem::paintAveragePaths(QPainter *painter)
{
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

                //Calculate downsampling factor of averaged data in respect to the items width
                int dsFactor;
                QRectF boundingRect = this->boundingRect();
                totalCols / boundingRect.width()<1 ? dsFactor = 1 : dsFactor = totalCols / boundingRect.width();

                //Calculate scaling value
                double dScaleY = (boundingRect.height())/(2*dMaxValue);

                //Setup the painter
                QPainterPath path = QPainterPath(QPointF(boundingRect.x(), *(averageData+(0*m_pFiffInfo->chs.size())+i) * -dScaleY));
                QPen pen;
                pen.setStyle(Qt::SolidLine);
                if(!m_cAverageColors.isEmpty() && i<m_cAverageColors.size())
                    pen.setColor(m_cAverageColors.at(i));
                pen.setWidthF(0.25);
                painter->setPen(pen);

                //Generate plot path
                QPointF qSamplePosition;

                for(int u = 0; u < totalCols && path.elementCount() <= boundingRect.width(); u += dsFactor) {
                    //evoked matrix is stored in column major
                    double val = (*(averageData+(u*m_pFiffInfo->chs.size())+i) * dScaleY);

                    qSamplePosition.setY(-val);
                    qSamplePosition.setX(path.currentPosition().x()+1);

                    path.lineTo(qSamplePosition);
                }

                //Paint the path
                painter->drawPath(path);
            }
        }
    }
}










