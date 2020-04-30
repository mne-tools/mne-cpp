//=============================================================================================================
/**
 * @file     frequencyspectrumdelegate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FrequencySpectrumDelegate Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrumdelegate.h"

#include "frequencyspectrummodel.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QTableView>
#include <QPainterPath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumDelegate::FrequencySpectrumDelegate(QTableView* m_pTableView,QObject *parent)
: QAbstractItemDelegate(parent)
, m_iScaleType(0)
, m_fMaxValue(0.0)
, m_fScaleY(0.0)
, m_tableview_row(0)
, m_mousex(0)
, m_mousey(0)
, m_x_rate(0.0)
{
    m_tableview = m_pTableView;
    m_tableview->setMouseTracking(true);
}

//=============================================================================================================

void FrequencySpectrumDelegate::setScaleType(qint8 ScaleType)
{
    m_iScaleType = ScaleType;
}

//=============================================================================================================

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
                createGridTick(index, option, painter);

                //capture the mouse
                capturePoint(index, option, path, data, painter);

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

//=============================================================================================================

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

//=============================================================================================================

void FrequencySpectrumDelegate::rcvMouseLoc(int tableview_row, int mousex, int mousey, QRect visRect)
{

    if(mousex != m_mousex){

    m_tableview_row = tableview_row;
    m_mousex = mousex;
    m_mousey = mousey;
    m_visRect = visRect;

    m_x_rate = (float)m_mousex/(float)m_visRect.width();

    m_tableview->viewport()->repaint();
    }
}

//=============================================================================================================

void FrequencySpectrumDelegate::capturePoint(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorXd& data, QPainter *painter) const
{
    Q_UNUSED(option);
    Q_UNUSED(path);

    if (m_tableview_row == index.row()){
    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    qint32 i;

    RowVectorXd org_vecFreqScale = t_pModel->getFreqScale();
    RowVectorXd vecFreqScale = t_pModel->getFreqScaleBound();

    qint32 lowerIdx = t_pModel->getLowerFrqBound();
    qint32 upperIdx = t_pModel->getUpperFrqBound();

    //qint32 numbins = vecFreqScale.size();//  data.size();

    //qDebug() << "numbins" << numbins;
    //qDebug() << "lowerIdx" << lowerIdx << "upperIdx" << upperIdx;

    // find the index for the current mouse cursor location
    for(i = lowerIdx+1; i <= upperIdx; ++i) {

        //float tmp_rate = t_pModel->getFreqScale()[i]/t_pModel->getFreqScale()[numbins-1];

        float tmp_rate = (vecFreqScale[i] - vecFreqScale[lowerIdx])/(vecFreqScale[upperIdx]-vecFreqScale[lowerIdx]);

        if (tmp_rate > m_x_rate) { break;}
        //qDebug()<<"tmp_rate"<<tmp_rate<<"m_x_rate"<<m_x_rate<<"i"<<i;
    }

    /***************************************************
     * Mouse moving showing the frequency and value
     *
     * *************************************************/

    unsigned short usPosY = m_visRect.bottom();
    unsigned short usPosX = m_visRect.left();
    unsigned short usHeight = m_visRect.height();
    unsigned short usWidth = m_visRect.width();

    int iPosX = m_mousex;
    int iPosY = m_mousey;

    if(iPosX>usPosX && iPosX < usPosX+usWidth && iPosY > (usPosY - usHeight) && iPosY < usPosY )
    {
    //qDebug()<<" index row" << index.row()<< "i"<< i << "iPosX,iposY" << iPosX << iPosY << "usPosY"<<usPosY<<"usHeight"<<usHeight;
    //Horizontal line
    painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));

    QPoint start(iPosX - 25, iPosY);//iStartY-5);//paint measure line vertical direction
    QPoint end(iPosX + 25, iPosY);//iStartY+5);

//    painter->drawLine(start, end);

    //vertical line
    start.setX(iPosX); start.setY(usPosY -usHeight); // iPosY - 25);//iStartY - 5);
    end.setX(iPosX); end.setY(usPosY); //iPosY + 25);//iStartY + 5);
    painter->drawLine(start, end);
    // Draw text
    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));

    // cal the frequency according to the iPosX
    double fs = t_pModel->getInfo()->sfreq/2;

    //RowVectorXd vecFreqScale = t_pModel->getFreqScale();
    //RowVectorXd vecFreqScale = t_pModel->getFreqScaleBound();
    double freq;
    if (m_iScaleType) // log
    {
        double max = log10(fs+1);
        org_vecFreqScale *= max;

        freq = pow(10,org_vecFreqScale[i]) - 1;
    }
    else //normal
    {
        org_vecFreqScale *= fs;

        freq = org_vecFreqScale[i];

    }

    QString tx = QString("%1 [DB], %2 [Hz]").arg(data[i]).arg(freq);

    if (iPosX > usPosX + usWidth - tx.size()*8 )
        painter->drawText(iPosX-tx.size()*8, iPosY-8, tx);// ToDo Precision should be part of preferences
    else
        painter->drawText(iPosX+8, iPosY-8, tx);// ToDo Precision should be part of preferences
    }
    }//correct row to plot
}

//=============================================================================================================

void FrequencySpectrumDelegate::createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorXd& data) const
{
    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    float fMaxValue = data.maxCoeff();

    float fValue;
    float fScaleY = option.rect.height()/(fMaxValue*0.5);

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    qint32 lowerIdx = t_pModel->getLowerFrqBound();
    qint32 upperIdx = t_pModel->getUpperFrqBound();

    //Move to initial starting point
    if(data.size() > 0)
    {
        float val = 0;
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX((double)option.rect.width()*t_pModel->getFreqScaleBound()[lowerIdx]);

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = lowerIdx+1; i <= upperIdx; ++i) {
        float val = data[i]-data[0]; //remove first sample data[0] as offset
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX((double)option.rect.width()*t_pModel->getFreqScaleBound()[i]);

        path.lineTo(qSamplePosition);
    }
}

//=============================================================================================================

void FrequencySpectrumDelegate::createGridPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorXd& data) const
{
    Q_UNUSED(data)

    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    if(t_pModel->getInfo())
    {
        double nf = t_pModel->getInfo()->sfreq/2;

        qint32 numLines = (m_iScaleType)? (qint32)ceil(log10(nf)) : 5 ;

        QList<qint32> qListLineSamples;

        qListLineSamples << 0;

        if (m_iScaleType)
        { // log
            for(qint32 lineIdx = 0; lineIdx < numLines; ++lineIdx)
            {
                double val = pow(10,lineIdx);
                qint32 idx = (qint32)floor(val / ((float)nf/(float)t_pModel->getNumStems()));
                qListLineSamples.append(idx);
            }
        }
        else{ // normal
            for(qint32 lineIdx = 1; lineIdx < numLines; ++lineIdx)
            {
                double val = lineIdx*(nf/numLines);
                qint32 idx = (qint32)floor(val / ((float)nf/(float)t_pModel->getNumStems()));
                qListLineSamples.append(idx);
            }

        }
        //vertical lines
        float yStart = option.rect.topLeft().y();

        float yEnd = option.rect.bottomRight().y();

        for(qint32 i = 0; i < qListLineSamples.size(); ++i) {
            if(qListLineSamples[i] > t_pModel->getLowerFrqBound() && qListLineSamples[i] < t_pModel->getUpperFrqBound())
            {
                float x = (t_pModel->getFreqScaleBound()[qListLineSamples[i]])*option.rect.width();
                path.moveTo(x,yStart);
                path.lineTo(x,yEnd);
            }
        }

    }
}

//=============================================================================================================

void FrequencySpectrumDelegate::createGridTick(const QModelIndex &index, const QStyleOptionViewItem &option,  QPainter *painter) const
{
    const FrequencySpectrumModel* t_pModel = static_cast<const FrequencySpectrumModel*>(index.model());

    if(t_pModel->getInfo())
    {
        double nf = t_pModel->getInfo()->sfreq/2;

        qint32 numLines = (m_iScaleType)? (qint32)ceil(log10(nf)) : 5 ;

        QList<qint32> qListLineSamples;

        qListLineSamples << 0;
        if (m_iScaleType)
        { // log
            for(qint32 lineIdx = 0; lineIdx < numLines; ++lineIdx)
            {
                double val = pow(10,lineIdx);
                qint32 idx = (qint32)floor(val / ((float)nf/(float)t_pModel->getNumStems()));
                qListLineSamples.append(idx);
            }
        }
        else
        { // normal
            for(qint32 lineIdx = 1; lineIdx < numLines; ++lineIdx)
            {
                double val = lineIdx*(nf/numLines);
                qint32 idx = (qint32)floor(val / ((float)nf/(float)t_pModel->getNumStems()));
                qListLineSamples.append(idx);
            }

        }

        // XTick
        float yStart = 1.0*option.rect.topLeft().y();

        if(qListLineSamples[0] > t_pModel->getLowerFrqBound() && qListLineSamples[0] < t_pModel->getUpperFrqBound())
        {
            double val = 0.0;
            float x = (t_pModel->getFreqScaleBound()[qListLineSamples[0]])*option.rect.width();
            painter->drawText(x,yStart,QString("%1Hz").arg(val));
        }

        for(qint32 i = 1; i < qListLineSamples.size(); ++i) {
            if(qListLineSamples[i] > t_pModel->getLowerFrqBound() && qListLineSamples[i] < t_pModel->getUpperFrqBound())
            {
                double val = (m_iScaleType)? pow(10,i-1) : t_pModel->getFreqScaleBound()[qListLineSamples[i]]*nf;
                float x = (t_pModel->getFreqScaleBound()[qListLineSamples[i]])*option.rect.width();
                painter->drawText(x,yStart,QString("%1Hz").arg(val));
            }
        }

        // YTick
    }
}

