//=============================================================================================================
/**
* @file     mnertclientsquidcontroldgl.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the mnertclientSQUIDControlDgl class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mnertclient.h"
#include "mnertclientsquidcontroldgl.h"
#include "ui_mnertclientsquidcontroldgl.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MneRtClientPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

mnertclientSQUIDControlDgl::mnertclientSQUIDControlDgl(MneRtClient* p_pMneRtClient,QWidget *parent) :
    QDialog(parent),
    m_pMneRtClient(p_pMneRtClient),
    ui(new Ui::mnertclientSQUIDControlDgl)
{
    ui->setupUi(this);

    // retune
    connect(ui->m_Qbn_retune, &QPushButton::released, this, &mnertclientSQUIDControlDgl::SendRetune);

}

mnertclientSQUIDControlDgl::~mnertclientSQUIDControlDgl()
{
    delete ui;
}

void mnertclientSQUIDControlDgl::SendRetune()
{
    // Send FLL command to cmdclient
    if(m_pMneRtClient->m_bCmdClientIsConnected)
    {
        this->ui->m_tx_info->setText(QString("Send Retune Command"));
        QString t_sReply = m_pMneRtClient->m_pRtCmdClient->sendCLICommand(QString("RETUNE"));
        this->ui->m_tx_info->setText(QString("Reply:")+t_sReply);
    }
}
