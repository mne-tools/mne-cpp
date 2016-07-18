//=============================================================================================================
/**
* @file     flashobject.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenauz.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April 2016
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
* @brief    Contains the implementation of the FlashObject class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "flashobject.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FlashObject::FlashObject(QGraphicsItem *parent) :
    QGraphicsObject(parent),
  m_iT(100),
  m_bFlashState(true),
  m_iPosX(0),
  m_iPosY(0),
  m_iDimX(200),
  m_iDimY(200),
  m_dFreq(10)
{
    //adjust and start timer
    m_pTimer = new QTimer();
    connect(m_pTimer,SIGNAL(timeout()),this,SLOT(flash()));
    m_pTimer->setTimerType(Qt::PreciseTimer);
    m_pTimer->start(m_iT);
    m_t0 = QTime::currentTime();

    this->setFlag(ItemIsMovable, true);
    this->setFlag(ItemIsSelectable, true);
}

//*************************************************************************************************************

void FlashObject::flash()
{
    //calculating FPS
    m_t1 = QTime::currentTime();
    int curDelta = m_t0.msecsTo(m_t1);
    m_t0 = m_t1;
    if(curDelta != m_iDelta)
    {
        m_iDelta = curDelta;
        emit currDelta(2*m_iDelta);
        emit adjDelta(2*m_iT);
    }

    //process blinking procedure
    if(m_bFlashState)
        this->show();
    else
        this->hide();
    m_bFlashState = ! m_bFlashState;


    emit trigger(m_bFlashState);
}

//*************************************************************************************************************

void FlashObject::setPos(int x, int y)
{
    m_iPosX = qreal(x);
    m_iPosY = qreal(y);

    this->update();
}

//*************************************************************************************************************

void FlashObject::setDim(int x, int y)
{
    m_iDimX = x;
    m_iDimY = y;

    this->update();
}

//*************************************************************************************************************

void FlashObject::setFreq(double freq)
{
    //setting the new timer interval
    m_iT = int(1000/double(freq)/2);
    m_pTimer->setInterval(m_iT);

    //refresh adjusted freq value
    m_dFreq = freq;
}

//*************************************************************************************************************

double FlashObject::getFreq()
{
    return m_dFreq;
}

//*************************************************************************************************************

QRectF FlashObject::boundingRect() const
{
    return QRectF(m_iPosX, m_iPosY, m_iDimX, m_iDimY);
}

//*************************************************************************************************************

void FlashObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //Plot shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::white);
    painter->drawRect(m_iPosX, m_iPosY, this->boundingRect().width(), this->boundingRect().height());
}
