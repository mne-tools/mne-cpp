//=============================================================================================================
/**
* @file     ftbuffersetupwidget.cpp
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2020
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
* @brief    Definition of the FtBufferSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffersetupwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBufferSetupWidget::FtBufferSetupWidget(FtBuffer* toolbox, QWidget *parent)
: QWidget(parent)
, m_pFtBuffer(toolbox)
{
    ui.setupUi(this);

    this->ui.m_lineEditIP->setText(toolbox->m_pFtBuffProducer->m_pFtConnector->getAddr());


    //Always connect GUI elemts after ui.setpUi has been called
    connect(ui.m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog())); // About page
    connect(ui.m_qPushButton_Connect, SIGNAL(released()), this, SLOT(pressedConnect())); // Connect/Disconnect button

    connect(this, &FtBufferSetupWidget::connectAtAddr, m_pFtBuffer->m_pFtBuffProducer.data(), &FtBuffProducer::connectToBuffer);
    connect(m_pFtBuffer->m_pFtBuffProducer.data(), &FtBuffProducer::connecStatus, this, &FtBufferSetupWidget::isConnected);
}

//*************************************************************************************************************

FtBufferSetupWidget::~FtBufferSetupWidget()
{

}

//*************************************************************************************************************

void FtBufferSetupWidget::showAboutDialog()
{
    FtBufferAboutWidget aboutDialog(this);
    aboutDialog.exec();
}

//*************************************************************************************************************

void FtBufferSetupWidget::pressedConnect()
{
    emit connectAtAddr(ui.m_lineEditIP->text(),ui.m_spinBoxPort->value());
}

//*************************************************************************************************************

void FtBufferSetupWidget::isConnected(bool stat) {
    if (stat) {
        ui.m_qPushButton_Connect->setText("Set");
    }
}
