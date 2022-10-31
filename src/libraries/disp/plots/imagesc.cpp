//=============================================================================================================
/**
 * @file     imagesc.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the ImageSc class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "imagesc.h"
#include "helpers/colormap.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ImageSc::ImageSc(QWidget *parent)
: Graph(parent)
, m_pPixmapData(NULL)
, m_pPixmapColorbar(NULL)
{
    init();
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXd &p_dMat, QWidget *parent)
: Graph(parent)
, m_pPixmapData(NULL)
, m_pPixmapColorbar(NULL)
{
    init();
    updateData(p_dMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXf &p_fMat, QWidget *parent)
: Graph(parent)
, m_pPixmapData(NULL)
, m_pPixmapColorbar(NULL)
{
    init();
    updateData(p_fMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXi &p_iMat, QWidget *parent)
: Graph(parent)
, m_pPixmapData(NULL)
, m_pPixmapColorbar(NULL)
{
    init();
    updateData(p_iMat);
}

//=============================================================================================================

ImageSc::~ImageSc()
{
    if(m_pPixmapData)
        delete m_pPixmapData;
    if(m_pPixmapColorbar)
        delete m_pPixmapColorbar;
}

//=============================================================================================================

void ImageSc::init()
{
    //Colormap
    pColorMapper = ColorMap::valueToColor;
    m_sColorMap = "Hot";

    //Parent init
    Graph::init();

    m_iBorderTopBottom = 20;
    m_iBorderLeftRight = 60;

    //Colorbar
    m_bColorbar = true;
    m_qFontColorbar.setPixelSize(10);
    m_qPenColorbar = QPen(Qt::black);
    m_iColorbarWidth = 12;
    m_iColorbarSteps = 7;//>= 2!!
    m_iColorbarGradSteps = 200;
}

//=============================================================================================================

void ImageSc::updateData(MatrixXd &p_dMat)
{
    if(p_dMat.rows() > 0 && p_dMat.cols() > 0)
    {
        m_dMinValue = p_dMat.minCoeff();
        m_dMaxValue = p_dMat.maxCoeff();

        // -- data --
        m_matCentNormData = p_dMat;
        double minValue = m_matCentNormData.minCoeff();
        m_matCentNormData.array() -= minValue;
        double maxValue = m_matCentNormData.maxCoeff();
        m_matCentNormData.array() /= maxValue;

        updateMaps();
    }
}

//=============================================================================================================

void ImageSc::updateData(MatrixXf &p_fMat)
{
    MatrixXd t_dMat = p_fMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::updateData(MatrixXi &p_iMat)
{
    MatrixXd t_dMat = p_iMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::updateMaps()
{
    if(m_pPixmapData)
    {
        delete m_pPixmapData;
        m_pPixmapData = NULL;
    }
    if(m_pPixmapColorbar)
    {
        delete m_pPixmapColorbar;
        m_pPixmapColorbar = NULL;
    }

    if(m_matCentNormData.rows() > 0 && m_matCentNormData.cols() > 0)
    {
        // --Data--
        qint32 x = m_matCentNormData.cols();
        qint32 y = m_matCentNormData.rows();
        qint32 i, j;
        QImage t_qImageData(x, y, QImage::Format_RGB32);
        for(i = 0; i < x; ++i)
            for(j = 0; j < y; ++j)
                t_qImageData.setPixel(i, j, pColorMapper(m_matCentNormData(j,i), m_sColorMap));

        m_pPixmapData = new QPixmap(QPixmap::fromImage(t_qImageData));

        // --Colorbar--
        QImage t_qImageColorbar(1, m_iColorbarGradSteps, QImage::Format_RGB32);

        double t_dQuantile = 1.0/((double)m_iColorbarGradSteps-1);
        for(j = 0; j < m_iColorbarGradSteps; ++j)
        {
            QRgb t_qRgb = pColorMapper(t_dQuantile*((double)(m_iColorbarGradSteps-1-j))*1.0, m_sColorMap);
            t_qImageColorbar.setPixel(0, j, t_qRgb);
        }
        m_pPixmapColorbar = new QPixmap(QPixmap::fromImage(t_qImageColorbar));

        // --Scale Values--
        m_qVecScaleValues.clear();

        double scale = pow(10, floor(log(m_dMaxValue-m_dMinValue)/log(10.0)));

        //Zero Based Scale?
        if(m_dMaxValue > 0 && m_dMinValue < 0)
        {
            double quantum = floor((((m_dMaxValue-m_dMinValue)/scale)/(m_iColorbarSteps-1))*10.0)*(scale/10.0);
            double start = 0;
            while(m_dMinValue < (start - quantum))
                start -= quantum;
            //Create Steps
            m_qVecScaleValues.push_back(start);
            for(qint32 i = 1; i < m_iColorbarSteps-1; ++i)
                m_qVecScaleValues.push_back(m_qVecScaleValues[i-1]+quantum);
        }
        else
        {
            double quantum = floor((((m_dMaxValue-m_dMinValue)/scale)/(m_iColorbarSteps-1))*10.0)*(scale/10.0);
            double start = floor(((m_dMaxValue-m_dMinValue)/2.0 + m_dMinValue)/scale)*scale;
            while(m_dMinValue < (start - quantum))
                start -= quantum;
            //Create Steps
            m_qVecScaleValues.push_back(start);
            for(qint32 i = 1; i < m_iColorbarSteps-1; ++i)
                m_qVecScaleValues.push_back(m_qVecScaleValues[i-1]+quantum);
        }
        update();
    }
}

//=============================================================================================================

void ImageSc::setColorMap(const QString &p_sColorMap)
{
    m_sColorMap = p_sColorMap;

    updateMaps();
}

//=============================================================================================================

void ImageSc::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (m_pPixmapData)
    {
        QPoint t_qPointTopLeft(0,0);

        // -- Data --
        QSize t_qSizePixmapData = m_qSizeWidget;

        t_qSizePixmapData.setHeight(t_qSizePixmapData.height()-m_iBorderTopBottom*2);
        t_qSizePixmapData.setWidth(t_qSizePixmapData.width()-m_iBorderLeftRight*2);
        // Scale data
        QPixmap t_qPixmapScaledData = m_pPixmapData->scaled(t_qSizePixmapData,  Qt::IgnoreAspectRatio);//Qt::KeepAspectRatio);
        // Calculate data position
        t_qPointTopLeft.setX((m_qSizeWidget.width()-t_qPixmapScaledData.width())/2);
        t_qPointTopLeft.setY((m_qSizeWidget.height()-t_qPixmapScaledData.height())/2);
        //Draw data
        painter.drawPixmap(t_qPointTopLeft,t_qPixmapScaledData);
        //Draw border
        painter.drawRect(t_qPointTopLeft.x()-1, t_qPointTopLeft.y()-1, t_qPixmapScaledData.width()+1, t_qPixmapScaledData.height()+1);

        // -- Colorbar --
        if(m_bColorbar && m_pPixmapColorbar && m_qVecScaleValues.size() >= 2)
        {
            QSize t_qSizePixmapColorbar = m_qSizeWidget;

            t_qSizePixmapColorbar.setWidth(m_iColorbarWidth);
            t_qSizePixmapColorbar.setHeight(t_qPixmapScaledData.height());

            // Scale colorbar
            QPixmap t_qPixmapScaledColorbar = m_pPixmapColorbar->scaled(t_qSizePixmapColorbar, Qt::IgnoreAspectRatio);
            // Calculate colorbar position
            t_qPointTopLeft.setY(t_qPointTopLeft.y());
            t_qPointTopLeft.setX(t_qPointTopLeft.x() + t_qPixmapScaledData.width() + m_iBorderLeftRight/3);
            //Draw colorbar
            painter.drawPixmap(t_qPointTopLeft,t_qPixmapScaledColorbar);
            //Draw border
            painter.drawRect(t_qPointTopLeft.x()-1, t_qPointTopLeft.y()-1, m_iColorbarWidth+1, t_qPixmapScaledData.height()+1);

            // -- Scale --
            painter.setPen(m_qPenColorbar);
            painter.setFont(m_qFontColorbar);

            qint32 x = t_qPointTopLeft.x()+ m_iColorbarWidth + m_qFontColorbar.pixelSize()/2;
            qint32 x_markLeft = x - m_iColorbarWidth - m_qFontColorbar.pixelSize()/2;
            qint32 x_markRight = x_markLeft + m_iColorbarWidth;

            // max
            painter.save();
            qint32 y_max = t_qPointTopLeft.y() - m_qFontColorbar.pixelSize()/2;
            painter.translate(x, y_max-1);
            painter.drawText(QRect(0, 0, 100, 12), Qt::AlignLeft, QString::number(m_dMaxValue));
            painter.restore();
            //draw max marks
            qint32 y_max_mark = y_max + m_qFontColorbar.pixelSize()/2;
            painter.drawLine(x_markLeft,y_max_mark,x_markLeft+2,y_max_mark);
            painter.drawLine(x_markRight-3,y_max_mark,x_markRight-1,y_max_mark);

            // min
            painter.save();
            qint32 y_min = t_qPointTopLeft.y() + t_qSizePixmapColorbar.height()-1 - m_qFontColorbar.pixelSize()/2;
            painter.translate(x, y_min-1);
            painter.drawText(QRect(0, 0, 100, 12), Qt::AlignLeft, QString::number(m_dMinValue));
            painter.restore();
            //draw min marks
            qint32 y_min_mark = y_min + m_qFontColorbar.pixelSize()/2;
            painter.drawLine(x_markLeft,y_min_mark,x_markLeft+2,y_min_mark);
            painter.drawLine(x_markRight-3,y_min_mark,x_markRight-1,y_min_mark);

            //Scale values
            qint32 y_dist = y_min - y_max;
            double minPercent = (m_qVecScaleValues[0]- m_dMinValue)/(m_dMaxValue-m_dMinValue);
            double distPercent = (m_qVecScaleValues[1]-m_qVecScaleValues[0])/(m_dMaxValue-m_dMinValue);
            qint32 y_current = y_min - (minPercent*y_dist);
            qint32 y_current_mark;
            //draw scale
            for(qint32 i = 0; i < m_qVecScaleValues.size(); ++i)
            {
                painter.save();
                painter.translate(x, y_current-1);
                painter.drawText(QRect(0, 0, 100, 12), Qt::AlignLeft, QString::number(m_qVecScaleValues[i]));
                painter.restore();
                //draw marks
                y_current_mark =  y_current + m_qFontColorbar.pixelSize()/2;
                painter.drawLine(x_markLeft,y_current_mark,x_markLeft+2,y_current_mark);
                painter.drawLine(x_markRight-3,y_current_mark,x_markRight-1,y_current_mark);
                //update y_current
                y_current -= distPercent*y_dist;
            }
        }

        //Draw title & axes
        Graph::drawLabels(t_qPixmapScaledData.width(), t_qPixmapScaledData.height());
    }
}
