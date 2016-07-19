//=============================================================================================================
/**
* @file     ssvepbciscreen.cpp
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
* @brief    Contains the implementation of the ssvepBCIScreen class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbciscreen.h"
#include <QDebug>
#include <QPainter>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ssvepBCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


ssvepBCIScreen::ssvepBCIScreen(QSharedPointer<ssvepBCI> pSSVEPBCI, QOpenGLWidget *parent)
: m_pSSVEPBCI(pSSVEPBCI)
, m_dXPosCross(0.5)
, m_dYPosCross(0.5)
, m_dStep(0.01)
, m_bUseScreenKeyboard(false)
, m_qPainter(this)
, m_qCrossColor(Qt::red)
//, m_qSoundPath(m_pSSVEPBCI->getSSVEPBCIResourcePath() + "beep.mp3")
{
//      // implementing the sound file
//    qDebug() << "copying successfull:" << QFile::copy(":/sounds/beep.mp3", m_qSoundPath);
//    m_qBeep.setMedia(QUrl(m_qSoundPath));
//    m_qBeep.setVolume(100);

    // register Meta Type
    qRegisterMetaType<MyQList>("MyQList");

    //set format of the QOpenGLWidget (enable vsync and setup buffers)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapInterval(1);
    setFormat(format);

    // connect classResult and frequency list signal of SSVEPBCI class to setClassResult slot
    connect(m_pSSVEPBCI.data(), &ssvepBCI::classificationResult, this, &ssvepBCIScreen::setClassResults);
    connect(m_pSSVEPBCI.data(), &ssvepBCI::getFrequencyLabels, this, &ssvepBCIScreen::updateFrequencyList);

    // initialize freqList
    m_lFreqList << 6.66 << 7.5 << 8.57 << 10 << 12;

}


//*************************************************************************************************************

ssvepBCIScreen::~ssvepBCIScreen(){

}

//*************************************************************************************************************

void ssvepBCIScreen::resizeGL(int w, int h) {
    Q_UNUSED(w)
    Q_UNUSED(h)
}


//*************************************************************************************************************

void ssvepBCIScreen::initializeGL(){
}


//*************************************************************************************************************

void ssvepBCIScreen::paintGL() {

    //paint all items to the screen
    for(int i = 0; i < m_Items.size(); i++)
        m_Items[i].paint(this);

    m_qPainter.begin(this);
    //painting red cross as a point of reference for the subject
//    m_qPainter.fillRect((m_dXPosCross-0.01/2)*this->width(),(m_dYPosCross-0.05/2)*this->height(),0.01*this->width(),0.05*this->height(), m_qCrossColor);
//    m_qPainter.fillRect(m_dXPosCross*this->width()-0.05*this->height()/2,m_dYPosCross*this->height()-0.01*this->width()/2,0.05*this->height(),0.01*this->width(),m_qCrossColor);

    if(m_bUseScreenKeyboard)
        m_ScreenKeyboard->paint(this);

    m_qPainter.end();
    update(); //schedules next update directly, without going through signal dispatching
}


//*************************************************************************************************************

void ssvepBCIScreen::setClassResults(double classResult){

    m_qCrossColor = Qt::red;

    if(classResult != 0){
        int index = m_lFreqList.indexOf(classResult);
        // assign classifaiction result to an action
        switch(index){
        case 0:
            m_dYPosCross -= m_dStep; break;
        case 1:
            m_dXPosCross += m_dStep; break;
        case 2:
            m_dYPosCross += m_dStep; break;
        case 3:
            m_dXPosCross -= m_dStep; break;
        case 4:
            m_qCrossColor = Qt::blue; break;
        default:
            qDebug() << "WARNING: no classifiaction could be made!"; break;
        }

//        //generate beep sound
//        m_qBeep.play();  // doesn't work on Windows 10
    }
}


//*************************************************************************************************************

void ssvepBCIScreen::updateFrequencyList(MyQList freqList){
    m_lFreqList.clear();
    m_lFreqList = freqList;
    qDebug() <<"update freqList:" <<freqList;
}


//*************************************************************************************************************

void ssvepBCIScreen::useScreenKeyboard(bool useKeyboard){

    if(m_ScreenKeyboard == NULL)
        m_ScreenKeyboard = QSharedPointer<ScreenKeyboard>(new ScreenKeyboard(m_pSSVEPBCI, QSharedPointer<ssvepBCIScreen>(this)));

    m_bUseScreenKeyboard = useKeyboard;
}


//*************************************************************************************************************

