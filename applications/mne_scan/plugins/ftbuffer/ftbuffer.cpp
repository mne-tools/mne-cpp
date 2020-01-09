/**
* @file     ftbuffer.cpp
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the FtBuffer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace FTBUFFERPLUGIN;

//*************************************************************************************************************
FtBuffer::FtBuffer() :
m_bIsRunning(false)
{
    qDebug() << "Constructing FtBuffer Object";
    m_pActionShowYourWidget = new QAction(QIcon(":/images/options.png"), tr("FieldTrip Buffer Widget"),this);
    m_pActionShowYourWidget->setShortcut(tr("F12"));
    m_pActionShowYourWidget->setStatusTip(tr("FieldTrip Buffer Widget"));
    connect(m_pActionShowYourWidget, &QAction::triggered, this, &FtBuffer::showYourWidget);
    addPluginAction(m_pActionShowYourWidget);
}

//*************************************************************************************************************
FtBuffer::~FtBuffer() {
    if(this->isRunning()){
        stop();
    }
}


QSharedPointer<IPlugin> FtBuffer::clone() const {
    QSharedPointer<FtBuffer> pFtBufferClone(new FtBuffer);
    return pFtBufferClone;
}

//*************************************************************************************************************
void FtBuffer::init() {qDebug() << "Running init()";}

void FtBuffer::unload() {}

bool FtBuffer::start() {

    qDebug() << "Running start()";
    m_bIsRunning = true;
    QThread::start();

    return true;
}

//*************************************************************************************************************
bool FtBuffer::stop() {

    m_bIsRunning = false;
    return true;
}

//*************************************************************************************************************
IPlugin::PluginType FtBuffer::getType() const {
    return _ISensor;
}

//*************************************************************************************************************
QString FtBuffer::getName() const {
    return "FtBuffer";
}

//*************************************************************************************************************
QWidget* FtBuffer::setupWidget() {
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this);
    return setupWidget;
}

//*************************************************************************************************************
void FtBuffer::run() {}

//*************************************************************************************************************
void FtBuffer::showYourWidget() {
    m_pYourWidget = FtBufferYourWidget::SPtr(new FtBufferYourWidget());
    m_pYourWidget->show();
}

//*************************************************************************************************************
bool FtBuffer::connectToBuffer(QString addr){
    //this->m_FtBuffClient.setAddress(addr);
    return this->m_FtBuffClient.startConnection();
}

//*************************************************************************************************************
bool FtBuffer::disconnectFromBuffer(){
    return this->m_FtBuffClient.stopConnection();
}
