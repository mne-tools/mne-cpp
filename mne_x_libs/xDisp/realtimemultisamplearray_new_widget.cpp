//=============================================================================================================
/**
* @file     realtimesamplearray_new_widget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
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
* @brief    Implementation of the RealTimeMultiSampleArrayNewWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearray_new_widget.h"
//#include "annotationwindow.h"
#include "displaymanager.h"

#include <xMeas/Measurement/realtimemultisamplearray_new.h>

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QTime>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayNewWidget::RealTimeMultiSampleArrayNewWidget(RealTimeMultiSampleArrayNew* pRTMSA_New, QTime* pTime, QWidget* parent)
: MeasurementWidget(parent)
, m_pRTMSA_New(pRTMSA_New)
, m_bMeasurement(false)
, m_bPosition(true)
, m_bFrozen(false)
, m_bScaling(false)
, m_bToolInUse(false)
, m_dSampleWidth(1.0)
, m_dPosX(0.0)
, m_dPosY(0)
, m_bStartFlag(true)
, m_ucToolIndex(0)
, m_pTimerToolDisplay(0)
, m_pTimerUpdate(0)
, m_pTime(pTime)
, m_pTimeCurrentDisplay(0)
{
    ui.setupUi(this);
    ui.m_qLabel_Tool->hide();

    // Add tool names to vector
    m_vecTool.push_back("Freeze");
    m_vecTool.push_back("Annotation");

    // Start timer
    m_pTimerUpdate = new QTimer(this);
    connect(m_pTimerUpdate, SIGNAL(timeout()), this, SLOT(update()));

    m_pTimerUpdate->start(25);

    //connect(ui.m_qSpinBox_Max, SIGNAL(valueChanged(int)), this, SLOT(maxValueChanged(int)));
    //connect(ui.m_qSpinBox_Min, SIGNAL(valueChanged(int)), this, SLOT(minValueChanged(int)));

    setMouseTracking(true);
}


//*************************************************************************************************************

RealTimeMultiSampleArrayNewWidget::~RealTimeMultiSampleArrayNewWidget()
{
    delete m_pTimerToolDisplay;
    delete m_pTimerUpdate;
    delete m_pTimeCurrentDisplay;

    // Clear sampling rate vector
    RealTimeMultiSampleArrayNewWidget::s_listSamplingRates.clear();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::actualize()
{
    m_dPosY = ui.m_qFrame->pos().y()+0.5*ui.m_qFrame->height();


    // Compute scaling factor
    m_fScaleFactor = ui.m_qFrame->height()/static_cast<float>(m_pRTMSA_New->chInfo()[0].getMaxValue()-m_pRTMSA_New->chInfo()[0].getMinValue());

    // Compute the middle of RTSA values
    m_dMiddle = 0.5*(m_pRTMSA_New->chInfo()[0].getMinValue()+m_pRTMSA_New->chInfo()[0].getMaxValue())*m_fScaleFactor;

    //*********************************************************************************************************
    //=========================================================================================================
    // Compute new sample width in order to synchronize all RTSA
    //=========================================================================================================

    if((m_pRTMSA_New->getSamplingRate() == 0) || (DisplayManager::getRTSAWidgets().size() == 0))
        return;

    // Add current sampling rate to s_listSamplingRates
    RealTimeMultiSampleArrayNewWidget::s_listSamplingRates << m_pRTMSA_New->getSamplingRate();

    // Find maximal sampling rate in s_listSamplingRates
    double dMax = 0;
    foreach (double value, s_listSamplingRates)
        dMax = value > dMax ? value : dMax;

    // Set new sample widths
    foreach(RealTimeMultiSampleArrayNewWidget* pRTMSAW, DisplayManager::getRTMSANewWidgets().values())
        pRTMSAW->m_dSampleWidth = dMax/pRTMSAW->m_pRTMSA_New->getSamplingRate();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::stopAnnotation()
{
    m_bToolInUse = !m_bToolInUse;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::maxValueChanged(double maxValue)
{
//    m_pRTMSA_New->setMaxValue(maxValue);
    for(quint32 i = 0; i < m_pRTMSA_New->getNumChannels(); ++i)
        m_pRTMSA_New->chInfo()[i].setMaxValue(maxValue);

//    ui.m_qLabel_MaxValue->setText(QString::number(maxValue));
    actualize();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::minValueChanged(double minValue)
{
//    m_pRTMSA_New->setMinValue(minValue);
    for(quint32 i = 0; i < m_pRTMSA_New->getNumChannels(); ++i)
        m_pRTMSA_New->chInfo()[i].setMaxValue(minValue);

//    ui.m_qLabel_MinValue->setText(QString::number(minValue));
    actualize();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::update(Subject*)
{
    VectorXd vecValue = VectorXd::Zero(m_uiNumChannels);
    double dPositionDifference = 0.0;
    QVector< VectorXd > matSamples = m_pRTMSA_New->getMultiSampleArray();


    for(unsigned char i = 0; i < m_pRTMSA_New->getMultiArraySize(); ++i)//ToDo maybe downsampling here increase step size
    {
//        for(unsigned int k = 0; k < m_uiNumChannels; ++k)
//            vecValue[k] = matSamples[i][k]*m_fScaleFactor - m_dMiddle;

        vecValue = (matSamples[i].array()*m_fScaleFactor).array() - m_dMiddle;

        dPositionDifference = m_dPosition - (m_dPosX+ui.m_qFrame->width());

        if((dPositionDifference >= 0) || m_bStartFlag)
        {
            if(m_bStartFlag)
                dPositionDifference = 0;

            m_qMutex.lock();
//                    m_qPainterPath = QPainterPath();
//                    m_qPainterPathTest = QPainterPath();

                m_dPosition = m_dPosX + dPositionDifference;

//                    m_qPainterPath.moveTo(m_dPosition, m_dPosY-dValue);
//                    m_qPainterPathTest.moveTo(m_dPosition, m_dPosY-dValue-10);

                for(unsigned int k = 0; k < m_uiNumChannels; ++k)
                {
                    m_qVecPainterPath[k] = QPainterPath();
                    m_qVecPainterPath[k].moveTo(m_dPosition, m_dPosY-vecValue[k]-k*10); // ToDo offset over PosY has to be relative
                }
            m_qMutex.unlock();
            m_bStartFlag = false;

            if(!m_bFrozen)
                m_pTimeCurrentDisplay->setHMS(m_pTime->hour(),m_pTime->minute(),m_pTime->second(),m_pTime->msec());
        }

        else
        {
            m_qMutex.lock();
//                    m_qPainterPath.lineTo(m_dPosition, m_dPosY-dValue);
//                    m_qPainterPathTest.lineTo(m_dPosition, m_dPosY-dValue-10);
            for(unsigned int k = 0; k < m_uiNumChannels; ++k)
                m_qVecPainterPath[k].lineTo(m_dPosition, m_dPosY-vecValue[k]-k*10); // ToDo offset over PosY vec has to be relative
            m_qMutex.unlock();
        }

        m_dPosition = m_dPosition + m_dSampleWidth;
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::init()
{
    ui.m_qLabel_Caption->setText(m_pRTMSA_New->getName());
//    ui.m_qLabel_MinValue->setText(QString::number(m_pRTSM->getMinValue()));
//    ui.m_qLabel_MaxValue->setText(QString::number(m_pRTSM->getMaxValue()));

    m_uiNumChannels = 20;//m_pRTMSA_New->getNumChannels();

    m_dMinValue_init = m_pRTMSA_New->chInfo()[0].getMinValue();
    m_dMaxValue_init = m_pRTMSA_New->chInfo()[0].getMaxValue();


    // Set drawing start position in X and Y direction
    m_dPosX = ui.m_qFrame->pos().x()+1;
    m_dPosition = m_dPosX;
//    m_dPosY = ui.m_qFrame->pos().y()+0.5*ui.m_qFrame->height();// set to actualize

//    m_qPainterPath = QPainterPath();
//    m_qPainterPathTest = QPainterPath();

    m_qVecPainterPath.clear();
    for(unsigned int i = 0; i < m_uiNumChannels; ++i)
        m_qVecPainterPath.push_back(QPainterPath());


    m_bStartFlag = true;

    if(m_pTimeCurrentDisplay)
        delete m_pTimeCurrentDisplay;
    m_pTimeCurrentDisplay = new QTime(0, 0);

    actualize();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);


//    //*************************************************************************************************************
//    //=============================================================================================================
//    // Draw white background
//    //=============================================================================================================
//
//    painter.setBrush(Qt::white);
//    painter.drawRect(0, 0, width(), height());


    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));

    //*************************************************************************************************************
    //=============================================================================================================
    // Draw grid in X direction (each 100ms)
    //=============================================================================================================

    double dNumPixelsX = m_pRTMSA_New->getSamplingRate()/10.0f;
    double dMinMaxDifference = static_cast<double>(m_pRTMSA_New->chInfo()[0].getMaxValue()-m_pRTMSA_New->chInfo()[0].getMinValue());
    double dActualPosX = 0.0;
    unsigned short usNumOfGridsX = (unsigned short)(ui.m_qFrame->width()/dNumPixelsX);
    unsigned short usPosY = ui.m_qFrame->pos().y()+1;
    unsigned short usPosX = ui.m_qFrame->pos().x()+1;
    unsigned short usHeight = ui.m_qFrame->height()-2;
    unsigned short usWidth = ui.m_qFrame->width()-2;

    for(unsigned short i = 1; i <= usNumOfGridsX; ++i)
    {
        dActualPosX = m_dPosX+i*dNumPixelsX;
        painter.drawLine((int)dActualPosX, usPosY, (int)dActualPosX, usPosY+usHeight);
    }


    //*************************************************************************************************************
    //=============================================================================================================
    // Draw grid in Y direction
    //=============================================================================================================

    double exponent = (int)floor(log10(dMinMaxDifference))-1;//math.h
    double dim = pow(10.0, exponent);//respectively at 0.001; 0.01, 0.1, 1, 10, 100

    int NumOfLines = (int)floor(dMinMaxDifference/(dim*5));

    double dDifferenceToFirstLine = (m_pRTMSA_New->chInfo()[0].getMaxValue()-floor(m_pRTMSA_New->chInfo()[0].getMaxValue()/dim)*dim);

    double dNumPixelsY = usHeight/NumOfLines;//10.0f;
    double dActualPosY = usPosY + dDifferenceToFirstLine * (usHeight/dMinMaxDifference);

    for(unsigned char i = 1; i <= NumOfLines; ++i)
    {
        painter.drawLine((int)m_dPosX, (int)dActualPosY, usWidth, (int)dActualPosY);
        dActualPosY += dNumPixelsY;
    }

    //Paint middle value
//	painter.setPen(QPen(Qt::gray, 1, Qt::SolidLine));
//	painter.drawText(usWidth-75, usHeight/2, tr("%1%2").arg(m_dMiddle, 0, 'f', 2).arg(m_pRTSM->getUnit()));
//	painter.setPen(QPen(Qt::gray, 1, Qt::DotLine));
//	painter.drawLine(m_dPosX, usHeight/2, usWidth, usHeight/2);

    painter.setPen(QPen(Qt::red, 1, Qt::SolidLine));
    painter.setRenderHint(QPainter::Antialiasing);


    //*************************************************************************************************************
    //=============================================================================================================
    // Draw real time curve respectively frozen curve
    //=============================================================================================================

    if(m_bFrozen)
    {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
//        painter.drawPath(m_qPainterPath_Freeze);
//        painter.drawPath(m_qPainterPath_FreezeTest);
        for(unsigned int k = 0; k < m_uiNumChannels; ++k)
            painter.drawPath(m_qVecPainterPath_Freeze[k]);
    }
    else
    {
        m_qMutex.lock();
//            painter.drawPath(m_qPainterPath);
//            painter.drawPath(m_qPainterPathTest);
            for(unsigned int k = 0; k < m_uiNumChannels; ++k)
                painter.drawPath(m_qVecPainterPath[k]);
        m_qMutex.unlock();
    }


    //*************************************************************************************************************
    //=============================================================================================================
    // Calculates zoom with the help of new minimum/maximum factors.
    //=============================================================================================================

    if(m_bScaling)
    {
        int iStartX = m_qPointMouseStartPosition.x();

        int iEndY   = m_qPointMouseEndPosition.y();
        int iStartY = m_qPointMouseStartPosition.y();

        // Compute pixel difference
        int iPixelDifferenceY = abs(iStartY - iEndY);

        double scale = (m_dMaxValue_init-m_dMinValue_init)/usHeight;

        if(iStartY>iEndY)
        {
            double changeValue = scale * iPixelDifferenceY;

            if(changeValue*2 < m_dMaxValue_init - m_dMinValue_init)
            {
                minValueChanged(m_dMinValue_init + changeValue);
                maxValueChanged(m_dMaxValue_init - changeValue);
            }
            else
            {
                double maxChange = (m_dMaxValue_init - m_dMinValue_init)*0.499999;
                minValueChanged(m_dMinValue_init + maxChange);
                maxValueChanged(m_dMaxValue_init - maxChange);
            }
        }
        else
        {
            double changeValue = scale * iPixelDifferenceY*10;

            minValueChanged(m_dMinValue_init - changeValue);
            maxValueChanged(m_dMaxValue_init + changeValue);
        }

        double factor = (m_dMaxValue_init-m_dMinValue_init)/(m_pRTMSA_New->chInfo()[0].getMaxValue()-m_pRTMSA_New->chInfo()[0].getMinValue());
        // Draw text
        painter.setPen(QPen(Qt::darkCyan, 1, Qt::SolidLine));
        painter.drawText(iStartX+8, iEndY, tr("Zoom %1x").arg(factor, 0, 'f', 2));

    }

    //*************************************************************************************************************
    //=============================================================================================================
    // Draw coordinates at mouse position
    //=============================================================================================================

    if(m_bPosition && m_pRTMSA_New->getSamplingRate())
    {
        int iPosX = mapFromGlobal(QCursor::pos()).x();

        int iPosY = mapFromGlobal(QCursor::pos()).y();

        if(iPosX > usPosX && iPosX  < (usPosX + usWidth) && iPosY > usPosY && iPosY < usPosY + usHeight )
        {
            //Vertical Measuring
            painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));

            QPoint start(usPosX, iPosY);//iStartY-5);//paint measure line vertical direction
            QPoint end(usPosX + usWidth, iPosY);//iStartY+5);

            painter.drawLine(start, end);

            start.setX(iPosX); start.setY(usPosY);//iStartY - 5);
            end.setX(iPosX); end.setY(usPosY + usHeight);//iStartY + 5);
            painter.drawLine(start, end);

            // Compute time between MouseStartPosition and MouseEndPosition
            QTime t = m_pTimeCurrentDisplay->addMSecs((int)(1000*(iPosX-usPosX)/(float)m_pRTMSA_New->getSamplingRate()));

            // Draw text
            painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));

            painter.drawText(iPosX+8, iPosY-8, tr("%1").arg(t.toString("hh:mm:ss.zzz")));// ToDo Precision should be part of preferences
        }
    }



    //*************************************************************************************************************
    //=============================================================================================================
    // Draw the measurement tools of the curve
    //=============================================================================================================

    if(m_bMeasurement && m_pRTMSA_New->getSamplingRate())
    {
        int iEndX   = m_qPointMouseEndPosition.x();
        int iStartX = m_qPointMouseStartPosition.x();

        int iEndY   = m_qPointMouseEndPosition.y();
        int iStartY = m_qPointMouseStartPosition.y();


        // Compute pixel difference
        double iPixelDifferenceX = abs(iStartX - iEndX);
        double iPixelDifferenceY = abs(iStartY - iEndY);

        if(iPixelDifferenceX < 5 && iPixelDifferenceY < 5)
            return;

        //Vertical Measuring
        painter.setPen(QPen(Qt::darkCyan, 1, Qt::DashLine));
        if(iPixelDifferenceX > iPixelDifferenceY)
        {
            // Draw measuring line
//          QPoint endPosY(iEndX, iStartY);
//          painter.drawLine(m_qPointMouseStartPosition, endPosY);

            QPoint start(iStartX, usPosY);//iStartY-5);//paint measure line vertical direction
            QPoint end(iStartX, usPosY+usHeight);//iStartY+5);
            painter.drawLine(start, end);

            start.setX(iEndX); start.setY(usPosY);//iStartY - 5);
            end.setX(iEndX); end.setY(usPosY+usHeight);//iStartY + 5);
            painter.drawLine(start, end);

            // Compute text position
            if(iEndX > iStartX)
                iEndX = iEndX + 9;
            else
                iEndX = iEndX - 67;

            // Compute time between MouseStartPosition and MouseEndPosition
            float iTime = 1000.0f*(float)iPixelDifferenceX/(float)m_pRTMSA_New->getSamplingRate();
            float iHz = 1000.0f/(float)iTime;

            // Draw text
            painter.setPen(QPen(Qt::darkCyan, 1, Qt::SolidLine));

            painter.drawText(iEndX, iEndY-18, tr("%1ms").arg(iTime, 0, 'f', 2));// ToDo Precision should be part of preferences
            painter.drawText(iEndX, iEndY-4, tr("%1Hz").arg(iHz, 0, 'f', 2));
        }
        else
        {
            // Draw measuring line
//          QPoint endPosX(iStartX, iEndY);
//          painter.drawLine(endPosX, m_qPointMouseStartPosition);

            QPoint start(usPosX, iStartY);//iStartY-5);//paint measure line vertical direction
            QPoint end(usPosX+usWidth, iStartY);//iStartY+5);
            painter.drawLine(start, end);

            start.setX(usPosX); start.setY(iEndY);//iStartY - 5);
            end.setX(usPosX+usWidth); end.setY(iEndY);//iStartY + 5);
            painter.drawLine(start, end);


            // Compute text position
            if(iEndY > iStartY)
                iEndY = iEndY + 1;
            else
                iEndY = iEndY + 23 ;

            // Compute time between MouseStartPosition and MouseEndPosition
            float fMagnitude = (float)iPixelDifferenceY * (dMinMaxDifference/usHeight) ;

            // Draw text
            painter.setPen(QPen(Qt::darkCyan, 1, Qt::SolidLine));
            painter.drawText(iEndX+14, iEndY-8, tr("%1%2").arg(fMagnitude, 0, 'e', 3).arg(m_pRTMSA_New->chInfo()[0].getUnit()));// ToDo Precision should be part of preferences
        }
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::resizeEvent(QResizeEvent*)
{
    m_bStartFlag = true; //start new painting
    actualize();
}

//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::mousePressEvent(QMouseEvent* mouseEvent)
{
    m_qPointMouseStartPosition = m_qPointMouseEndPosition = mouseEvent->pos();
    if(mouseEvent->button() == Qt::LeftButton)
    {
        m_bMeasurement = true;
        m_bPosition = false;
    }
    else if(mouseEvent->button() == Qt::RightButton)
    {
        m_bScaling = true;
        m_bPosition = false;
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::mouseMoveEvent(QMouseEvent* mouseEvent)
{
    if(m_bMeasurement || m_bScaling)
        m_qPointMouseEndPosition = mouseEvent->pos();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::mouseReleaseEvent(QMouseEvent*)
{
    m_bMeasurement = false;
    m_bPosition = true;
    m_bScaling = false;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::mouseDoubleClickEvent(QMouseEvent*)
{
    switch((Tool)m_ucToolIndex)
    {
        case Freeze:
            m_bFrozen = !m_bFrozen;
            if(m_bFrozen)
            {
//                m_qPainterPath_Freeze = m_qPainterPath;
//                m_qPainterPath_FreezeTest = m_qPainterPathTest;

                m_qVecPainterPath_Freeze = m_qVecPainterPath;
            }
            else
            {
                m_pTimeCurrentDisplay->setHMS(m_pTime->hour(),m_pTime->minute(),m_pTime->second(),m_pTime->msec());
            }
            break;

        case Annotation:
            break;
    }

    m_bToolInUse = !m_bToolInUse;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayNewWidget::wheelEvent(QWheelEvent* wheelEvent)
{
    if(m_bToolInUse)
        return;

    if(wheelEvent->delta() < 0)
    {
        if(m_ucToolIndex == 0)
            m_ucToolIndex = m_vecTool.size()-1;
        else
            --m_ucToolIndex;
    }
    else
    {
        if(m_ucToolIndex == m_vecTool.size()-1)
            m_ucToolIndex = 0;
        else
            ++m_ucToolIndex;
    }

    QString text = QString("%1/%2 Tool: %3").arg(m_ucToolIndex+1).arg(m_vecTool.size()).arg(m_vecTool[m_ucToolIndex]);
    ui.m_qLabel_Tool->setText(text);
    ui.m_qLabel_Tool->show();

    if(m_pTimerToolDisplay)
        delete m_pTimerToolDisplay;

    m_pTimerToolDisplay = new QTimer(this);

    connect( m_pTimerToolDisplay, SIGNAL(timeout()), ui.m_qLabel_Tool, SLOT(hide()));
    m_pTimerToolDisplay->start(2000);

}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QList<double>       RealTimeMultiSampleArrayNewWidget::s_listSamplingRates;
