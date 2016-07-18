//=============================================================================================================
/**
* @file     ssvepBCIFlickeringItem.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenauz.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May 2016
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
* @brief    Contains the implementation of the ssvepBCIFlickeringItem class.
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

using namespace ssvepBCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ssvepBCIFlickeringItem::ssvepBCIFlickeringItem()
    : m_dPosX(0)
    , m_dPosY(0)
    , m_dWidth(0.4)
    , m_dHeight(0.4)
    , m_bFlickerState(true)
    , m_bSignFlag(false)
    , m_bIter(m_bRenderOrder)

{
    m_bRenderOrder << 0 << 0 << 1 << 1; //default
}

//*************************************************************************************************************

ssvepBCIFlickeringItem::~ssvepBCIFlickeringItem(){
}

//*************************************************************************************************************

void ssvepBCIFlickeringItem::setPos(double x, double y){
        m_dPosX = x;
        m_dPosY = y;
}

//*************************************************************************************************************

void ssvepBCIFlickeringItem::setDim(double w, double h){
    m_dWidth    = w;
    m_dHeight   = h;
}

//*************************************************************************************************************

void ssvepBCIFlickeringItem::setRenderOrder(QList<bool> renderOrder, int freqKey){

    //clear the old rendering order list
    m_bRenderOrder.clear();

    //setup the iterator and assign it to the new list
    m_bRenderOrder  = renderOrder;
    m_bIter         = m_bRenderOrder;
    m_iFreqKey      = freqKey;
}

//*************************************************************************************************************

void ssvepBCIFlickeringItem::paint(QPaintDevice *paintDevice)
{
    //setting the nex flicker state (moving iterater to front if necessary)
    if(!m_bIter.hasNext())
        m_bIter.toFront();
    m_bFlickerState = m_bIter.next();

    //painting the itme's shape
    QPainter p(paintDevice);


    if(m_bSignFlag){
//        // scaling the letter size to the biggest sign "DEL"
//        float factor =0.2* m_dWidth*paintDevice->width() / p.fontMetrics().width(m_sSign);
//        if ((factor < 1) || (factor > 1.25))
//        {
//            QFont f = p.font();
//            f.setBold(true);
//            f.setPointSizeF(f.pointSizeF()*factor);
//            p.setFont(f);
//        }

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(30);
        p.setFont(f);
    }

    QRect rectangle(m_dPosX*paintDevice->width(),m_dPosY*paintDevice->height(),m_dWidth*paintDevice->width(),m_dHeight*paintDevice->height());

    if(true){
        p.fillRect(rectangle,Qt::white);
        p.drawText(rectangle, Qt::AlignCenter, m_sSign);
    }
//    else
//        p.fillRect(m_dPosX*paintDevice->width(),m_dPosY*paintDevice->height(),m_dWidth*paintDevice->width(),m_dHeight*paintDevice->height(),Qt::black);


}

//*************************************************************************************************************

int ssvepBCIFlickeringItem::getFreqKey()
{
    return m_iFreqKey;
}


void ssvepBCIFlickeringItem::addSign(QString sign){

    m_bSignFlag = true;
    m_sSign = sign;
}
