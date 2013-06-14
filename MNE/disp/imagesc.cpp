//=============================================================================================================
/**
* @file     imagesc.cpp
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
* @brief    Implementation of the ImageSc class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "imagesc.h"
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

ImageSc::ImageSc(QWidget *parent)
: QWidget(parent)
, m_qPixmapData(NULL)
, m_qPixmapLegend(NULL)
{
    init();
}


//*************************************************************************************************************

ImageSc::ImageSc(MatrixXd &p_dMat, QWidget *parent)
: QWidget(parent)
, m_qPixmapData(NULL)
, m_qPixmapLegend(NULL)
{
    init();
    updateMatrix(p_dMat);
}


//*************************************************************************************************************

ImageSc::ImageSc(MatrixXf &p_fMat, QWidget *parent)
: QWidget(parent)
, m_qPixmapData(NULL)
, m_qPixmapLegend(NULL)
{
    init();
    updateMatrix(p_fMat);
}


//*************************************************************************************************************

ImageSc::ImageSc(MatrixXi &p_iMat, QWidget *parent)
: QWidget(parent)
, m_qPixmapData(NULL)
, m_qPixmapLegend(NULL)
{
    init();
    updateMatrix(p_iMat);
}


//*************************************************************************************************************

ImageSc::~ImageSc()
{
    if(m_qPixmapData)
        delete m_qPixmapData;
    if(m_qPixmapLegend)
        delete m_qPixmapLegend;
}


//*************************************************************************************************************

void ImageSc::init()
{
    //Set Borders
    m_iBorderTopBottom = 50;
    m_iBorderLeftRight = 100;

    //Set Fonts
    m_qFontAxes.setPixelSize(12);
    m_qPenAxes = QPen(Qt::black);

    m_qFontTitle.setPixelSize(20);
    m_qFontTitle.setBold(true);
    m_qPenTitle = QPen(Qt::black);
}


//*************************************************************************************************************

void ImageSc::updateMatrix(MatrixXd &p_dMat)
{
    if(m_qPixmapData)
    {
        delete m_qPixmapData;
        m_qPixmapData = NULL;
    }
    if(m_qPixmapLegend)
    {
        delete m_qPixmapLegend;
        m_qPixmapLegend = NULL;
    }

    if(p_dMat.rows() > 0 && p_dMat.cols())
    {
        m_sTitle = QString("");
        m_sXLabel = QString("");
        m_sYLabel = QString("");

        m_dMinValue = p_dMat.minCoeff();
        m_dMaxValue = p_dMat.maxCoeff();

        // -- data --
        MatrixXd dataNormalized = p_dMat;
        double minValue = dataNormalized.minCoeff();
        dataNormalized.array() -= minValue;
        double maxValue = dataNormalized.maxCoeff();
        dataNormalized.array() /= maxValue;

        qint32 x = p_dMat.cols();
        qint32 y = p_dMat.rows();
        qint32 i, j;
        QImage t_qImageData(x, y, QImage::Format_RGB32);
        for(i = 0; i < x; ++i)
            for(j = 0; j < y; ++j)
                t_qImageData.setPixel(i, j, ColorMap::valueToHsv(dataNormalized(j,i)));

        m_qPixmapData = new QPixmap(QPixmap::fromImage(t_qImageData));

        // -- legend --
        qint32 t_iLegendWidth = 20;

        QImage t_qImageLegend(t_iLegendWidth, y, QImage::Format_RGB32);

        double t_dQuantile = 1.0/((double)y);
        for(j = 0; j < y; ++j)
        {
            QRgb t_qRgb = ColorMap::valueToHsv(t_dQuantile*((double)j)*1.0);
            for(i = 0; i < t_iLegendWidth; ++i)
                t_qImageLegend.setPixel(i, j, t_qRgb);
        }

        m_qPixmapLegend = new QPixmap(QPixmap::fromImage(t_qImageLegend));
    }
    update();
}


//*************************************************************************************************************

void ImageSc::updateMatrix(MatrixXf &p_fMat)
{
    MatrixXd t_dMat = p_fMat.cast<double>();
    updateMatrix(t_dMat);
}


//*************************************************************************************************************

void ImageSc::updateMatrix(MatrixXi &p_iMat)
{
    MatrixXd t_dMat = p_iMat.cast<double>();
    updateMatrix(t_dMat);
}


//*************************************************************************************************************

void ImageSc::resizeEvent (QResizeEvent* event)
{
    widgetSize = event->size();
    // Call base class impl
    QWidget::resizeEvent(event);
}


//*************************************************************************************************************

void ImageSc::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!m_qPixmapData->isNull())
    {
        QPoint t_qPointCenter(0,0);

        // -- Data --

        QSize t_qSizePixmapData = widgetSize;

        t_qSizePixmapData.setHeight(t_qSizePixmapData.height()-m_iBorderTopBottom*2);
        t_qSizePixmapData.setWidth(t_qSizePixmapData.width()-m_iBorderLeftRight*2);

        // Scale new image which size is widgetSize
        QPixmap t_qPixmapScaledData = m_qPixmapData->scaled(t_qSizePixmapData, Qt::KeepAspectRatio);
        // Calculate image center position into screen
        t_qPointCenter.setX((widgetSize.width()-t_qPixmapScaledData.width())/2);
        t_qPointCenter.setY((widgetSize.height()-t_qPixmapScaledData.height())/2);

        painter.drawPixmap(t_qPointCenter,t_qPixmapScaledData);

        // -- Legend --
        QSize t_qSizePixmapLegend = widgetSize;

        t_qSizePixmapLegend.setHeight(t_qPixmapScaledData.height());//t_qSizePixmapLegend.height()-m_iBorderTopBottom*2);
        t_qSizePixmapLegend.setWidth(m_iBorderLeftRight/3);

        // Scale new image which size is widgetSize
        QPixmap t_qPixmapScaledLegend = m_qPixmapLegend->scaled(t_qSizePixmapLegend, Qt::IgnoreAspectRatio);
        // Calculate image center position into screen
        t_qPointCenter.setY(t_qPointCenter.y());

        t_qPointCenter.setX(t_qPointCenter.x() + t_qPixmapScaledData.width() + m_iBorderLeftRight/2);

        painter.drawPixmap(t_qPointCenter,t_qPixmapScaledLegend);

        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);

        qint32 t_iLabelWidth = t_qPixmapScaledData.width();
        qint32 t_iLabelHeight = 100;

        // -- Title --
        if(!m_sTitle.isEmpty())
        {
            painter.save();
            painter.setPen(m_qPenTitle);
            painter.setFont(m_qFontTitle);

            painter.translate((widgetSize.width()-t_iLabelWidth)/2, (widgetSize.height()-t_qPixmapScaledData.height())/2 - m_iBorderTopBottom*1.5);
            painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sTitle);

            painter.restore();
        }

        // -- Axes --
        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);

        // X Label
        if(!m_sXLabel.isEmpty())
        {
            painter.save();
            painter.translate((widgetSize.width()-t_iLabelWidth)/2, t_qPixmapScaledData.height()+((widgetSize.height()-t_qPixmapScaledData.height()-m_iBorderTopBottom)/2));
            painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sXLabel);
            painter.restore();
        }

        //Y Label
        if(!m_sYLabel.isEmpty())
        {
            painter.save();
            painter.rotate(270);
            painter.translate(-(widgetSize.height()+t_iLabelWidth)/2,(widgetSize.width()-t_qPixmapScaledData.width())/2-t_iLabelHeight*0.75);
            painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sYLabel);
            painter.restore();
        }
    }
}


//*************************************************************************************************************

void ImageSc::setTitle(const QString &p_sTitle)
{
    m_sTitle = p_sTitle;
    update();
}


//*************************************************************************************************************

void ImageSc::setXLabel(const QString &p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    update();
}


//*************************************************************************************************************

void ImageSc::setYLabel(const QString &p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    update();
}
