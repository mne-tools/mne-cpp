//=============================================================================================================
/**
 * @file     ssvepbciscreen.cpp
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
 * @brief    Definition of the ssvepBCIScreen class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbciscreen.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QPainter>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SSVEPBCIPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SsvepBciScreen::SsvepBciScreen(SsvepBci* pSsvepBci,
                               SsvepBciSetupStimulusWidget* pSsvepBciSetupStimulusWidget,
                               QOpenGLWidget *parent)
: QOpenGLWidget(parent)
, m_pSsvepBci(pSsvepBci)
, m_pSsvepBciSetupStimulusWidget(pSsvepBciSetupStimulusWidget)
, m_pScreenKeyboard(new ScreenKeyboard(m_pSsvepBci, m_pSsvepBciSetupStimulusWidget, this))
, m_dXPosCross(0.5)
, m_dYPosCross(0.5)
, m_dStep(0.01)
, m_bUseScreenKeyboard(false)
, m_qPainter(this)
, m_qCrossColor(Qt::red)
, m_bClearScreen(true)
{
    Q_UNUSED(parent);

    // register Meta Type
    qRegisterMetaType<MyQList>("MyQList");

    //set format of the QOpenGLWidget (enable vsync and setup buffers)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapInterval(1);
    setFormat(format);

    // set update behaviour to preserved-swap-buffer
    setUpdateBehavior(UpdateBehavior::PartialUpdate);

    // connect classResult and frequency list signal of SsvepBci class to setClassResult slot
    connect(m_pSsvepBci, &SsvepBci::classificationResult, this, &SsvepBciScreen::setClassResults);
    connect(m_pSsvepBci, &SsvepBci::getFrequencyLabels, this, &SsvepBciScreen::updateFrequencyList);

    // initialize freqList
    m_lFreqList << 6.66 << 7.5 << 8.57 << 10 << 12;
}


//*************************************************************************************************************

SsvepBciScreen::~SsvepBciScreen()
{
}


//*************************************************************************************************************

void SsvepBciScreen::resizeGL(int w, int h)
{
    Q_UNUSED(w)
    Q_UNUSED(h)

    m_pScreenKeyboard->initScreenKeyboard();
}


//*************************************************************************************************************

void SsvepBciScreen::initializeGL()
{
}


//*************************************************************************************************************

void SsvepBciScreen::paintGL()
{
    // clear Screen by drawing a black filled rectangle
    if(m_bClearScreen){
        QPainter p(this);
        p.fillRect(0,0,width(),height(), Qt::black);
        m_bClearScreen = false;
    }


    //paint all items to the screen
    for(int i = 0; i < m_Items.size(); i++){
        m_Items[i].paint(this);
    }

    if(m_bUseScreenKeyboard){
        m_pScreenKeyboard->paint(this);
    }

    update(); //schedules next update directly, without going through signal dispatching
}


//*************************************************************************************************************

void SsvepBciScreen::setClassResults(double classResult)
{
    m_qCrossColor = Qt::red;

    if(classResult != 0){
        int index = m_lFreqList.indexOf(classResult);
        // assign classifaiction result to an action
        switch(index){
        case 0:
            m_dYPosCross -= m_dStep;
            break;
        case 1:
            m_dXPosCross += m_dStep;
            break;
        case 2:
            m_dYPosCross += m_dStep;
            break;
        case 3:
            m_dXPosCross -= m_dStep;
            break;
        case 4:
            m_qCrossColor = Qt::blue;
            break;
        default:
            qDebug() << "WARNING: no classifiaction could be made!"; break;
        }
//        //generate beep sound
    }
}


//*************************************************************************************************************

void SsvepBciScreen::updateFrequencyList(MyQList freqList)
{
    m_lFreqList.clear();
    m_lFreqList = freqList;
    qDebug() <<"update freqList:" <<freqList;
}


//*************************************************************************************************************

void SsvepBciScreen::useScreenKeyboard(bool useKeyboard)
{
    if(m_pScreenKeyboard == NULL){
        m_pScreenKeyboard = QSharedPointer<ScreenKeyboard>(new ScreenKeyboard(m_pSsvepBci, m_pSsvepBciSetupStimulusWidget, this));
    }

    m_bUseScreenKeyboard = useKeyboard;
    m_pScreenKeyboard->initScreenKeyboard();
}


//*************************************************************************************************************

void SsvepBciScreen::clearScreen()
{
    m_bClearScreen = true;
    m_bUseScreenKeyboard = false;
}
