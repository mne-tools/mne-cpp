//=============================================================================================================
/**
 * @file     ssvepbciflickeringitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     May 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Definition of the SsvepBciFlickeringItem class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbciflickeringitem.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SSVEPBCIPLUGIN;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SsvepBciFlickeringItem::SsvepBciFlickeringItem()
: m_dPosX(0)
, m_dPosY(0)
, m_dWidth(0.4)
, m_dHeight(0.4)
, m_bFlickerState(true)
, m_bSignFlag(false)
, m_bIter(m_bRenderOrder)
, m_iFreqKey(0)
{
    m_bRenderOrder << 0 << 0 << 1 << 1; //default
}


//*************************************************************************************************************

SsvepBciFlickeringItem::~SsvepBciFlickeringItem()
{
}


//*************************************************************************************************************

void SsvepBciFlickeringItem::setPos(double x, double y)
{
        m_dPosX = x;
        m_dPosY = y;
}


//*************************************************************************************************************

void SsvepBciFlickeringItem::setDim(double w, double h)
{
    m_dWidth    = w;
    m_dHeight   = h;
}


//*************************************************************************************************************

void SsvepBciFlickeringItem::setRenderOrder(QList<bool> renderOrder, int freqKey)
{
    //clear the old rendering order list
    m_bRenderOrder.clear();

    //setup the iterator and assign it to the new list
    m_bRenderOrder  = renderOrder;
    m_bIter         = m_bRenderOrder;
    m_iFreqKey      = freqKey;
}


//*************************************************************************************************************

void SsvepBciFlickeringItem::paint(QPaintDevice *paintDevice)
{
    //setting the nex flicker state (moving iterater to front if necessary)
    if(!m_bIter.hasNext()){
        m_bIter.toFront();
    }

    if( m_bIter.peekNext() != m_bFlickerState){
        //painting the itme's shape
        QPainter p(paintDevice);

        if(m_bSignFlag){
            QFont f = p.font();
            f.setBold(true);
            f.setPointSize(30);
            p.setFont(f);
        }

        QRect rectangle(m_dPosX*paintDevice->width(),m_dPosY*paintDevice->height(),m_dWidth*paintDevice->width(),m_dHeight*paintDevice->height());

        if(m_bFlickerState){ //
            p.fillRect(rectangle,Qt::white);
            p.drawText(rectangle, Qt::AlignCenter, m_sSign);
        }
        else{
            p.fillRect(m_dPosX*paintDevice->width(),m_dPosY*paintDevice->height(),m_dWidth*paintDevice->width(),m_dHeight*paintDevice->height(),Qt::black);
        }
    }
    m_bFlickerState = m_bIter.next();
}


//*************************************************************************************************************

int SsvepBciFlickeringItem::getFreqKey()
{
    return m_iFreqKey;
}


//*************************************************************************************************************

void SsvepBciFlickeringItem::addSign(QString sign)
{
    m_bSignFlag = true;
    m_sSign = sign;
}
