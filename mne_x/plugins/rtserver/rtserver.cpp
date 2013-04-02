//=============================================================================================================
/**
* @file     rtserver.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the RTServer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver.h"

#include "FormFiles/rtserversetupwidget.h"
#include "FormFiles/rtserverrunwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTServerPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RTServer::RTServer()
{
    m_MDL_ID = MDL_ID::RTSERVER;

    m_pRtClient = new RtClient("127.0.0.1", "mne_x", this);


    qDebug() << "m_pRtClient->start()";

    // Start RtClient - ToDo just perform a rtserver check
    m_pRtClient->start();

}


//*************************************************************************************************************

RTServer::~RTServer()
{

}


//*************************************************************************************************************

bool RTServer::start()
{
    // Initialize real time measurements
    init();

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RTServer::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    //Clear Buffers


    return true;
}


//*************************************************************************************************************

Type RTServer::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

const char* RTServer::getName() const
{
    return "RT Server";
}


//*************************************************************************************************************

QWidget* RTServer::setupWidget()
{
    RTServerSetupWidget* widget = new RTServerSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog

    return widget;
}


//*************************************************************************************************************

QWidget* RTServer::runWidget()
{
    RTServerRunWidget* widget = new RTServerRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return widget;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void RTServer::init()
{
    qDebug() << "RTServer::init()";
}


//*************************************************************************************************************

void RTServer::run()
{
    while(true)
    {
    }
}
