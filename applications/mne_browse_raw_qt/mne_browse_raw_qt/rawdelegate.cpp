//=============================================================================================================
/**
* @file     rawdelegate.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of delegate of mne_browse_raw_qt
*
*/


//*************************************************************************************************************
// INCLUDES

#include "rawdelegate.h"
#include "rawmodel.h"

#include <QPointF>
#include <QRect>

//#include <QDebug>

//*************************************************************************************************************
// USED NAMESPACES


using namespace MNE_BROWSE_RAW_QT;
using namespace Eigen;
using namespace MNELIB;

//*************************************************************************************************************

RawDelegate::RawDelegate(QObject *parent)
: m_dPlotHeight(70)
, m_dDx(1)
, m_nhlines(6)
{
}

//*************************************************************************************************************

void RawDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    switch(index.column()) {
    case 0: //chnames
        painter->save();

//        qDebug() << "option.rect.x" << option.rect.x() << "y" << option.rect.y() << "w" << option.rect.width() << "h" << option.rect.height();
        painter->rotate(-90);
        painter->drawText(QRectF(-option.rect.y()-m_dPlotHeight,0,m_dPlotHeight,20),Qt::AlignCenter,index.model()->data(index,Qt::DisplayRole).toString());
        painter->restore();
        break;
    case 1: //data plot
        painter->save();

        QPainterPath path(QPointF(option.rect.x(),option.rect.y()));

        //Get data
        QVariant variant = index.model()->data(index,Qt::DisplayRole);

//        RowVectorXd data = variant.value<MapRowVectorXd>();

//        QList<MatrixXdR> datalist = variant.value<QList<MatrixXdR>();
//        MatrixXdR data;
//        data = datalist[0].row(index.row());

        //Plot grid
        painter->setRenderHint(QPainter::Antialiasing, false);
        createGridPath(path,data);

        painter->save();
        QPen pen;
        pen.setStyle(Qt::DotLine);
        pen.setWidthF(0.5);
        painter->setPen(pen);
        painter->drawPath(path);
        painter->restore();

        //Plot data path
        path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));
        createPlotPath(index,path,data);

//        qDebug() << "option.rect.x" << option.rect.x() << "y" << option.rect.y() << "w" << option.rect.width() << "h" << option.rect.height();

        painter->translate(0,m_dPlotHeight/2);

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawPath(path);

//        painter->translate(0,-m_dPlotHeight/2);

        //Write channel name
//        const QAbstractItemModel* model = index.model();

//        painter->translate(-option.rect.x(),0);
//        rect.moveTo(0,option.rect.y());
//        qDebug() << "New Rect, x" << rect.x() << "y" << rect.y() << "w" << rect.width() << "h" << rect.height();
//        painter->drawText(option.rect,model->data(model->index(index.row(), 0), Qt::DisplayRole).toString());
//        painter->drawText(QPointF(0,0),"test");
        painter->restore();
        break;
    }

}

QSize RawDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
    case 0:
        size = QSize(20,m_dPlotHeight);
        break;
    case 1:
        qint32 nsamples = index.model()->data(index).value<MatrixXdR>().cols();
        size = QSize(m_dDx*nsamples,m_dPlotHeight);
        break;
    }

    return size;
}

//=============================================================================================================

void RawDelegate::createPlotPath(const QModelIndex &index, QPainterPath& path, MatrixXdR& data) const
{
    double dValue;
    double dMaxValue = data.maxCoeff();
//    double dMaxValue = (static_cast<const RawModel*>(index.model()))->getMaxDataValue(FIFFV_MEG_CH); //ToDo: scale single channel to all channels of same type
    double dScaleY = m_dPlotHeight/(2*dMaxValue);

    double y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    //create lines from one to the next sample
    for(qint32 i=0; i < data.cols(); ++i)
    {
        dValue = data(0,i)*dScaleY;
        qSamplePosition.setY(y_base+dValue);
        qSamplePosition.setX(path.currentPosition().x()+m_dDx);

        path.lineTo(qSamplePosition);

        path.moveTo(qSamplePosition);
    }

//    qDebug("Plot-PainterPath created!");
}

void RawDelegate::createGridPath(QPainterPath& path, MatrixXdR& data) const
{
    //horizontal lines
    double distance = m_dPlotHeight/m_nhlines;

    QPointF startpos = path.currentPosition();
    QPointF endpoint(data.cols()*m_dDx,path.currentPosition().y());

    for(qint8 i=0; i < m_nhlines-1; ++i) {
        endpoint.setY(endpoint.y()+distance);
        path.moveTo(startpos.x(),endpoint.y());
        path.lineTo(endpoint);
    }

//    qDebug("Grid-PainterPath created!");
}

