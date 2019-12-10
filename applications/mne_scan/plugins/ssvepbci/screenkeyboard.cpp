//=============================================================================================================
/**
* @file     screenkeyboard.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the ScreenKeyboard class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "screenkeyboard.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SSVEPBCIPLUGIN;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScreenKeyboard::ScreenKeyboard(SsvepBci* pSsvepBci,
                               SsvepBciSetupStimulusWidget* pSsvepBciSetupStimulusWidget,
                               SsvepBciScreen* pSsvepBciScreen)
: m_pSsvepBci(pSsvepBci)
, m_pSsvepBciSetupStimulusWidget(pSsvepBciSetupStimulusWidget)
, m_pSsvepBciScreen(pSsvepBciScreen)
, m_qPainter(m_pSsvepBciScreen)
, m_qCurCursorCoord(QPair<int, int> (0,0))
, m_qOldCursorCoord(QPair<int, int> (1,0))
, m_bDisplaySpeller(false)
, m_bInitializeKeyboard(true)
, m_bUpdatePhraseDisplay(true)
, m_bUseSpellAccuracy(false)
, m_qSpellIterator(0)
{
    // initialize map for keyboard values and their relative coordinates to each other
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

    m_pSsvepBciScreen->update();

    // connect ssvepBCI signals to the screen keyboard
    connect(m_pSsvepBci, &SsvepBci::getFrequencyLabels, this, &ScreenKeyboard::updateClassList);
    connect(m_pSsvepBci, &SsvepBci::classificationResult, this, &ScreenKeyboard::updateCommand);

    // connect SSVEPBCI speller
    connect(m_pSsvepBciSetupStimulusWidget, &SsvepBciSetupStimulusWidget::settledPhrase, this, &ScreenKeyboard::setPhrase);
}


//*************************************************************************************************************

ScreenKeyboard::~ScreenKeyboard()
{
}


//*************************************************************************************************************

void ScreenKeyboard::paint(QPaintDevice *device)
{
    QPainter painter(device);

    // declare center of screen and width of letter boxes
    int width = int(0.08*m_pSsvepBciScreen->height());
    int x = int(0.5*m_pSsvepBciScreen->width() - 0.5*width);
    int y = int(0.5*m_pSsvepBciScreen->height() - 0.5*width);

    // draw screen keyboard
    if(m_bInitializeKeyboard){

        // setting the painter
        painter.setBrush(Qt::white);

        // drawing each item in the stored map
        foreach(QString sign, m_mapKeys){

            QPair<int, int> coord = m_mapKeys.key(sign);
            const QRect rectangle = QRect(x + coord.first*width,y - coord.second*width,width,width);

            // scaling the letter size to the biggest sign "DEL"
            float factor = float(width) / float(painter.fontMetrics().width("DEL"));
            if ((factor < 1) || (factor > 1.25))
            {
                QFont f = painter.font();
                f.setBold(true);
                f.setPointSizeF(f.pointSizeF()*factor);
                painter.setFont(f);
            }

            // painting rectangles and and drawing text into them
            painter.setPen(Qt::red);
            painter.drawRect(rectangle);
            painter.setPen(Qt::black);
            painter.drawText(rectangle, Qt::AlignCenter, sign);
        }

        m_bInitializeKeyboard = false;
        //qDebug() << "update BCI Screen";
    }

    // update cursor draw
    if( (m_qCurCursorCoord != m_qOldCursorCoord) ){

        // scaling the letter size to the biggest sign "DEL"
        float factor = ((float)(width)) / painter.fontMetrics().width("DEL");
        if ((factor < 1) || (factor > 1.25))
        {
            QFont f = painter.font();
            f.setBold(true);
            f.setPointSizeF(f.pointSizeF()*factor);
            painter.setFont(f);
        }

        // refresh old cursor position
        const QRect oldRect = QRect(x + m_qOldCursorCoord.first*width,y - m_qOldCursorCoord.second*width,width,width);
        painter.setBrush(Qt::white);
        painter.setPen(Qt::red);
        painter.drawRect(oldRect);

        // paint current cursor position
        const QRect curRect = QRect(x + m_qCurCursorCoord.first*width,y - m_qCurCursorCoord.second*width,width,width);
        painter.setBrush(Qt::red);
        painter.drawRect(curRect);

        // draw Text
        painter.setPen(Qt::black);
        painter.drawText(curRect, Qt::AlignCenter, m_mapKeys.value(m_qCurCursorCoord));
        painter.drawText(oldRect, Qt::AlignCenter, m_mapKeys.value(m_qOldCursorCoord));

        // update old cursor position
        m_qOldCursorCoord = m_qCurCursorCoord;
        //qDebug() << "update Cursor paint!";
    }

    // update speller box
    if(m_bDisplaySpeller & m_bUpdatePhraseDisplay){

        // scaling the letter size to the biggest sign - in this case: "DEL"
        float factor = (float)(width) / (float)(painter.fontMetrics().width("DEL"));
        if ((factor < 1) || (factor > 1.25))
        {
            QFont f = painter.font();
            f.setBold(true);
            f.setPointSizeF(f.pointSizeF()*factor);
            painter.setFont(f);
        }

        // setting speller box parameter
        int heightSpBox = 0.1* m_pSsvepBciScreen->height() ;
        int widthSpBox = 0.3* m_pSsvepBciScreen->width() ;
        QRect rectSpBox = QRect(0,m_pSsvepBciScreen->height() - heightSpBox, widthSpBox, heightSpBox);

        // drawing speller box
        painter.setBrush(Qt::white);
        painter.drawRect(rectSpBox);

        // draw text
        painter.setPen(Qt::red);
        painter.drawText(rectSpBox,Qt::AlignLeft & Qt::AlignTop, m_sSettledPhrase);
        painter.setPen(Qt::black);
        painter.drawText(rectSpBox,Qt::AlignBottom, m_sSpelledPhrase);

        //qDebug() << "Update Phrase display";

        m_bUpdatePhraseDisplay = false;
    }
}


//*************************************************************************************************************

void ScreenKeyboard::updateClassList(MyQList classList)
{
    m_lClassList = classList;
}


//*************************************************************************************************************

void ScreenKeyboard::updateCommand(double value){

    // initialize
    int index = m_lClassList.indexOf(value);
    int deltaX = 0;
    int deltaY = 0;

    // set new cursor position
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
        spellLetter(m_mapKeys.value(m_qCurCursorCoord));
        qDebug() << "chosen sign:"  << m_mapKeys.value(m_qCurCursorCoord);
        m_bUpdatePhraseDisplay = true;
        break;
    default:
        break;
    }

    // accuracy feature of the screen keyboard
    if(m_bUseSpellAccuracy && index >= 0){

        // giving audio feedback for every command
        //m_qSound->play();
        //cout << "\a";

        // determine distance between selected and supposed coordinate
        int difXold = m_qNextCoord.first - m_qCurCursorCoord.first;
        int difYold = m_qNextCoord.second - m_qCurCursorCoord.second;
        int difXnew = m_qNextCoord.first - ( m_qCurCursorCoord.first + deltaX);
        int difYnew = m_qNextCoord.second - ( m_qCurCursorCoord.second + deltaY);

        // evaluate classifiaction result
        if((m_mapKeys.contains(QPair<int, int> (m_qCurCursorCoord.first + deltaX, m_qCurCursorCoord.second + deltaY)))){

            if(qFabs( difXnew ) < qFabs(difXold) || qFabs( difYnew ) < qFabs(difYold)){

                emit isCorrectCommand(true);
                qDebug() << "correct movement";
            }
            else if((m_qCurCursorCoord == m_qNextCoord) && (index == 4)){

                emit isCorrectCommand(true);
                qDebug() << "correct select";

                if(m_qNextCoord == m_mapKeys.key("Del")){
                    m_qNextCoord = m_mapKeys.key(*m_qSpellIterator);
                    qDebug() << "correct delete! next coordinate:" << m_qNextCoord;
                } 
                else if(m_qSpellIterator != m_sSettledPhrase.end()){

                    ++m_qSpellIterator;
                    m_qNextCoord = m_mapKeys.key(*m_qSpellIterator);
                    qDebug() << "next sign:" << *m_qSpellIterator;
                    qDebug() << "next coordinate:" << m_qNextCoord;
                }

                if(m_qSpellIterator == m_sSettledPhrase.end()){
                    stopSpellAccuracyFeature();
                }
            }
            else if(m_qCurCursorCoord == m_mapKeys.key("Clr") && (index == 4)){

                m_qSpellIterator = m_sSettledPhrase.begin();
                m_qNextCoord = m_mapKeys.key(*m_qSpellIterator);
                qDebug() << "correct clear! next coordinate:" << m_qNextCoord;
            }
            else if((m_qCurCursorCoord != m_qNextCoord) && (index == 4)){

                m_qNextCoord = m_mapKeys.key("Del");
                emit isCorrectCommand(false);
                qDebug() << "false select! next coordinate is:" << m_qNextCoord << "Del";
            }
            else{
                emit isCorrectCommand(false);
                qDebug() << "wrong command";
            }

        }
        else{
            emit isCorrectCommand(false);
            qDebug() << "wrong command";
        }
    }

    // check existence of new cursor position -> then update
    if(m_mapKeys.contains(QPair<int, int> (m_qCurCursorCoord.first + deltaX, m_qCurCursorCoord.second + deltaY))){
        m_qCurCursorCoord.first    += deltaX;
        m_qCurCursorCoord.second   += deltaY;
    }

    if(index == 4){
        m_qCurCursorCoord = QPair<int, int> (0,0);
    }
}


//*************************************************************************************************************

void ScreenKeyboard::setPhrase(QString phrase)
{
    // capitalizing the phrase and assign it
    m_sSettledPhrase = phrase.toUpper();

    // set required flags
    m_bDisplaySpeller = true;
    m_bUpdatePhraseDisplay = true;

    if(m_bUseSpellAccuracy){
        initSpellAccuracyFeature();
    }
}


//*************************************************************************************************************

void ScreenKeyboard::spellLetter(QString letter)
{
    // classify the subject's selection
    if(letter == "Clr"){
        m_sSpelledPhrase.clear();
    }
    else if(letter == "Del"){
        m_sSpelledPhrase.resize(m_sSpelledPhrase.size() - 1);
    }
    else{
        m_sSpelledPhrase.append(letter);
        m_bUpdatePhraseDisplay = true;
        emit getLetter(letter);
    }
}


//*************************************************************************************************************

void ScreenKeyboard::initScreenKeyboard()
{
    m_bInitializeKeyboard = true;
    m_qOldCursorCoord = QPair<int, int>(100,100);
}


//*************************************************************************************************************

void ScreenKeyboard::initSpellAccuracyFeature()
{

    m_sSpelledPhrase.clear();

    // initialize spell iterator to settled phrase
    m_qSpellIterator = m_sSettledPhrase.begin();
    m_qNextCoord = m_mapKeys.key(*m_qSpellIterator);
    qDebug() << "next coordinate:" << m_qNextCoord;

    m_qCurCursorCoord = QPair<int, int> (0,0);

    m_bUseSpellAccuracy = true;
    m_bUpdatePhraseDisplay = true;
}


//*************************************************************************************************************

void ScreenKeyboard::stopSpellAccuracyFeature()
{
    emit spellingFinished();
    m_bUseSpellAccuracy = false;
}
