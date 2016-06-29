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


ssvepBCIScreen::ssvepBCIScreen(QOpenGLWidget *parent)
{
    //set format of the QOpenGLWidget (enable vsync and setup buffers)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapInterval(1);
    setFormat(format);
}

//*************************************************************************************************************

void ssvepBCIScreen::resizeGL(int w, int h) {
}

//*************************************************************************************************************

void ssvepBCIScreen::initializeGL(){
}

//*************************************************************************************************************

void ssvepBCIScreen::paintGL() {
    //paint all items to the screen
    for(int i = 0; i < m_Items.size(); i++)
        m_Items[i].paint(this);

    //painting red cross as a point of reference for the subject
    QPainter p(this);
    p.fillRect((0.5-0.01/2)*this->width(),(0.5-0.05/2)*this->height(),0.01*this->width(),0.05*this->height(),Qt::red);
    p.fillRect(0.5*this->width()-0.05*this->height()/2,0.5*this->height()-0.01*this->width()/2,0.05*this->height(),0.01*this->width(),Qt::red);

    update(); //schedules next update directly, without going through signal dispatching
}


