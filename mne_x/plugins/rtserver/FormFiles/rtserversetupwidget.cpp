//=============================================================================================================
/**
* @file     rtserversetupwidget.cpp
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
* @brief    Contains the implementation of the RTServerSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserversetupwidget.h"
#include "rtserveraboutwidget.h"
#include "../rtserver.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
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

RTServerSetupWidget::RTServerSetupWidget(RTServer* simulator, QWidget* parent)
: QWidget(parent)
, m_pRTServer(simulator)
{
    ui.setupUi(this);

    this->ui.m_qLineEdit_Ip->setText("127.0.0.1");

    connect(ui.m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog()));

    this->setConnectionStatus(m_pRTServer->m_pRtClient->getConnectionStatus());

//    if(m_pRTServer->m_pRtClient->getConnectionStatus())
//        ui.m_qTextBrowser_Information->setText(QString("true"));
//    else
//        ui.m_qTextBrowser_Information->setText(QString("false"));

    connect(m_pRTServer->m_pRtClient, &RtClient::connectionChanged, this, &RTServerSetupWidget::setConnectionStatus);
}


//*************************************************************************************************************

RTServerSetupWidget::~RTServerSetupWidget()
{

}


//*************************************************************************************************************

void RTServerSetupWidget::printToLog(QString logMsg)
{
    ui.m_qTextBrowser_ServerMessage->insertPlainText(logMsg+"\n");
    //scroll down to the newest entry
    QTextCursor c = ui.m_qTextBrowser_ServerMessage->textCursor();
    c.movePosition(QTextCursor::End);
    ui.m_qTextBrowser_ServerMessage->setTextCursor(c);
}


//*************************************************************************************************************

void RTServerSetupWidget::setConnectionStatus(bool p_bConnectionStatus)
{
    if(p_bConnectionStatus)
    {
        this->ui.m_qLabel_ConnectionStatus->setText(QString("Connected"));
        this->ui.m_qLineEdit_Ip->setEnabled(false);
        this->ui.m_qPushButton_SetIp->setEnabled(false);
    }
    else
    {
        this->ui.m_qLabel_ConnectionStatus->setText(QString("Not connected"));
        this->ui.m_qLineEdit_Ip->setEnabled(true);
        this->ui.m_qPushButton_SetIp->setEnabled(true);
    }
}


//*************************************************************************************************************

void RTServerSetupWidget::showAboutDialog()
{
    RTServerAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
