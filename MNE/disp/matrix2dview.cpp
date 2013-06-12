//=============================================================================================================
/**
* @file     matrix2dview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the Matrix2DView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "matrix2dview.h"
#include "colormap.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QResizeEvent>
#include <QPainter>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Matrix2DView::Matrix2DView(QWidget *parent)
: QWidget(parent)
, pixmap(NULL)
{
    init();
}


//*************************************************************************************************************

Matrix2DView::Matrix2DView(MatrixXd &p_dMat, QWidget *parent)
: QWidget(parent)
, pixmap(NULL)
{
    init();
    updateMatrix(p_dMat);
}


//*************************************************************************************************************

Matrix2DView::Matrix2DView(MatrixXf &p_fMat, QWidget *parent)
: QWidget(parent)
, pixmap(NULL)
{
    init();
    updateMatrix(p_fMat);
}


//*************************************************************************************************************

Matrix2DView::Matrix2DView(MatrixXi &p_iMat, QWidget *parent)
: QWidget(parent)
, pixmap(NULL)
{
    init();
    updateMatrix(p_iMat);
}


//*************************************************************************************************************

Matrix2DView::~Matrix2DView()
{
//    delete ui;
    if(pixmap)
    {
        delete pixmap;
    }

}


//*************************************************************************************************************

void Matrix2DView::init()
{
    //Set Borders
    m_iBorderTopBottom = 50;
    m_iBorderLeftRight = 50;

    //Set Fonts
    m_qFontAxes.setPixelSize(12);
    m_qPenAxes = QPen(Qt::black);

    m_qFontTitle.setPixelSize(20);
    m_qFontTitle.setBold(true);
    m_qPenTitle = QPen(Qt::black);
}


//*************************************************************************************************************

void Matrix2DView::updateMatrix(MatrixXd &p_dMat)
{
    if(pixmap)
    {
        delete pixmap;
        pixmap = NULL;
    }

    if(p_dMat.rows() > 0 && p_dMat.cols())
    {
        m_sTitle = QString("Test Matrix");
        m_sXLabel = QString("XXX");
        m_sYLabel = QString("YYY");

        MatrixXd dataNormalized = p_dMat;

        double minValue = dataNormalized.minCoeff();
        dataNormalized.array() -= minValue;

        double maxValue = dataNormalized.maxCoeff();
        dataNormalized.array() /= maxValue;

        qint32 x = p_dMat.cols();
        qint32 y = p_dMat.rows();
        QImage image(x, y, QImage::Format_RGB32);
        for(qint32 i = 0; i < x; ++i)
            for(qint32 j = 0; j < y; ++j)
                image.setPixel(i, j, ColorMap::valueToHsv(dataNormalized(j,i)));

        pixmap = new QPixmap(QPixmap::fromImage(image));
    }
    update();
}


//*************************************************************************************************************

void Matrix2DView::updateMatrix(MatrixXf &p_fMat)
{
    MatrixXd t_dMat = p_fMat.cast<double>();
    updateMatrix(t_dMat);
}


//*************************************************************************************************************

void Matrix2DView::updateMatrix(MatrixXi &p_iMat)
{
    MatrixXd t_dMat = p_iMat.cast<double>();
    updateMatrix(t_dMat);
}


//*************************************************************************************************************

void Matrix2DView::resizeEvent (QResizeEvent* event)
{
    widgetSize = event->size();
    // Call base class impl
    QWidget::resizeEvent(event);
}


//*************************************************************************************************************

void Matrix2DView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!pixmap->isNull())
    {
        QPoint centerPoint(0,0);

        QSize pixmapSize = widgetSize;

        pixmapSize.setHeight(pixmapSize.height()-m_iBorderTopBottom*2);
        pixmapSize.setWidth(pixmapSize.width()-m_iBorderLeftRight*2);

        // Scale new image which size is widgetSize
        QPixmap scaledPixmap = pixmap->scaled(pixmapSize, Qt::KeepAspectRatio);
        // Calculate image center position into screen
        centerPoint.setX((widgetSize.width()-scaledPixmap.width())/2);
        centerPoint.setY((widgetSize.height()-scaledPixmap.height())/2);
        // Draw image
        painter.drawPixmap(centerPoint,scaledPixmap);

        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);


        qint32 t_iLabelWidth = scaledPixmap.width();
        qint32 t_iLabelHeight = 100;

        painter.save();

        // -- Title --
        painter.setPen(m_qPenTitle);
        painter.setFont(m_qFontTitle);

        painter.translate((widgetSize.width()-t_iLabelWidth)/2, (widgetSize.height()-scaledPixmap.height())/2 - m_iBorderTopBottom*1.5);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sTitle);

        painter.restore();
        painter.save();

        // -- Axes --

        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);

        //X Label
        painter.translate((widgetSize.width()-t_iLabelWidth)/2, scaledPixmap.height()+((widgetSize.height()-scaledPixmap.height()-m_iBorderTopBottom)/2));
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sXLabel);

        painter.restore();

        //Y Label
        painter.rotate(270);
        painter.translate(-(widgetSize.height()+t_iLabelWidth)/2,(widgetSize.width()-scaledPixmap.width())/2-t_iLabelHeight*0.75);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sYLabel);
    }
}


//*************************************************************************************************************
//HSV Colormap
int Matrix2DView::R(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return 0;
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(slopeMRaising(x, -1.5)*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(1.0*255);
    else if(x >= 0.875)
        return (int)floor(slopeMFalling(x, 4.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

int Matrix2DView::G(double x)
{
    //Describe the green fuzzy set
    if(x < 0.125)
        return 0;
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(slopeMRaising(x, -0.5)*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(1.0*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(slopeMFalling(x, 2.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

int Matrix2DView::B(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.125)
        return (int)floor(slopeMRaising(x, 0.5)*255);
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(1.0*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(slopeMFalling(x, 2.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

double Matrix2DView::slopeMRaising(double x, double n)
{
    //f = m*x + n
    return 4*x + n;
}


//*************************************************************************************************************

double Matrix2DView::slopeMFalling(double x,double n)
{
    //f = m*x + n
    return -4*x + n;
}
