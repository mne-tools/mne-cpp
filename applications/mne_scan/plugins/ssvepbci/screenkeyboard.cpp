//=============================================================================================================
/**
* @file     screenkeyboard.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*			Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the ScreenKeaboard class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "screenkeyboard.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ssvepBCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================



ScreenKeyboard::ScreenKeyboard(QSharedPointer<ssvepBCI> pSSVEPBCI, QSharedPointer<ssvepBCIScreen> pSSVEPBCIScreen)
: m_pSSVEPBCI(pSSVEPBCI)
, m_pSSVEPBCIScreen(pSSVEPBCIScreen)
, m_qPainter(m_pSSVEPBCIScreen.data())
, m_qCursorCoord(QPair<int, int> (0,0))
{
    // initialize map for keyboard values and their relative coordinates
    m_mapKeys[QPair<int, int>( 0, 0)] = "E";
    m_mapKeys[QPair<int, int>( 0, 1)] = "I";
    m_mapKeys[QPair<int, int>( 0, 2)] = "T";
    m_mapKeys[QPair<int, int>( 0, 3)] = "L";
    m_mapKeys[QPair<int, int>( 0,-1)] = "R";
    m_mapKeys[QPair<int, int>( 0,-2)] = "H";
    m_mapKeys[QPair<int, int>( 1, 0)] = "S";
    m_mapKeys[QPair<int, int>( 1, 1)] = "O";
    m_mapKeys[QPair<int, int>( 1, 2)] = "J";
    m_mapKeys[QPair<int, int>( 1, 3)] = "Del";
    m_mapKeys[QPair<int, int>( 1,-1)] = "B";
    m_mapKeys[QPair<int, int>( 1,-2)] = "Z";
    m_mapKeys[QPair<int, int>(-1, 0)] = "N";
    m_mapKeys[QPair<int, int>(-1, 1)] = "M";
    m_mapKeys[QPair<int, int>(-1, 2)] = "F";
    m_mapKeys[QPair<int, int>(-1, 3)] = "Clr";
    m_mapKeys[QPair<int, int>(-1,-1)] = "W";
    m_mapKeys[QPair<int, int>(-1,-2)] = "X";
    m_mapKeys[QPair<int, int>( 2, 0)] = "D";
    m_mapKeys[QPair<int, int>( 2, 1)] = "K";
    m_mapKeys[QPair<int, int>( 2,-1)] = "Y";
    m_mapKeys[QPair<int, int>( 3, 0)] = "C";
    m_mapKeys[QPair<int, int>( 3, 1)] = ".";
    m_mapKeys[QPair<int, int>( 3,-1)] = "-";
    m_mapKeys[QPair<int, int>( 4, 0)] = " ";
    m_mapKeys[QPair<int, int>(-2, 0)] = "A";
    m_mapKeys[QPair<int, int>(-2, 1)] = "V";
    m_mapKeys[QPair<int, int>(-2,-1)] = "P";
    m_mapKeys[QPair<int, int>(-3, 0)] = "U";
    m_mapKeys[QPair<int, int>(-3, 1)] = "Q";
    m_mapKeys[QPair<int, int>(-3,-1)] = "G";
    m_mapKeys[QPair<int, int>(-4, 0)] = ",";

    m_lClassList << 6.66 << 7.5 << 8.57 << 10 << 12;

    m_pSSVEPBCIScreen->update();

    // connect ssvepBCI signals to the screen keyboard
    connect(m_pSSVEPBCI.data(), &ssvepBCI::getFrequencyLabels, this, &ScreenKeyboard::updateClassList);
    connect(m_pSSVEPBCI.data(), &ssvepBCI::classificationResult, this, &ScreenKeyboard::updateCommand);

}


//*************************************************************************************************************

ScreenKeyboard::~ScreenKeyboard(){

}


//*************************************************************************************************************

void ScreenKeyboard::paint(QPaintDevice *device){

    QPainter painter(device);

    // declare center of screen and width of letter boxes
    int width = int(0.08*m_pSSVEPBCIScreen.data()->height());
    int x = int(0.5*m_pSSVEPBCIScreen.data()->width() - 0.5*width);
    int y = int(0.5*m_pSSVEPBCIScreen.data()->height() - 0.5*width);

    // scaling the letter size to the biggest sign "DEL"
    float factor = width / painter.fontMetrics().width("DEL");
    if ((factor < 1) || (factor > 1.25))
    {
        QFont f = painter.font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF()*factor);
        painter.setFont(f);
    }

     // setting the painter
     painter.setBrush(Qt::white);

     foreach(QString sign, m_mapKeys){

         QPair<int, int> coord = m_mapKeys.key(sign);
         const QRect rectangle = QRect(x + coord.first*width,y - coord.second*width,width,width);

         // painting rectangles and text
         painter.setPen(Qt::red);
         painter.drawRect(rectangle);
         painter.setPen(Qt::black);
         painter.drawText(rectangle, Qt::AlignCenter, sign);

    }

    // paint cursor
    const QRect rectangle = QRect(x + m_qCursorCoord.first*width,y - m_qCursorCoord.second*width,width,width);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::red);
    painter.drawRect(rectangle);
    painter.setPen(Qt::black);
    painter.drawText(rectangle, Qt::AlignCenter, m_mapKeys.value(m_qCursorCoord));

}


//*************************************************************************************************************

void ScreenKeyboard::updateClassList(MyQList classList){
    m_lClassList = classList;
}


//*************************************************************************************************************

void ScreenKeyboard::updateCommand(double value){

    int index = m_lClassList.indexOf(value);
    int deltaX = 0;
    int deltaY = 0;

    switch(index){
    case 0:
        deltaY +=1; break;
    case 1:
        deltaX +=1; break;
    case 2:
        deltaY -=1; break;
    case 3:
        deltaX -=1; break;
    case 4:
        emit getLetter(m_mapKeys.value(m_qCursorCoord));
        m_qCursorCoord = QPair<int, int> (0,0);
        break;
    default:
        break;
    }

    // check if cursor keys exist -> then update
    if(m_mapKeys.contains(QPair<int, int> (m_qCursorCoord.first + deltaX, m_qCursorCoord.second + deltaY))){
        m_qCursorCoord.first    += deltaX;
        m_qCursorCoord.second   += deltaY;
    }

}
