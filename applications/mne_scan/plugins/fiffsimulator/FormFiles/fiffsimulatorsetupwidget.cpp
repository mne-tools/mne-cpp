//=============================================================================================================
/**
 * @file     fiffsimulatorsetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RTServerSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulatorsetupwidget.h"

#include "../fiffsimulator.h"

#include <fiff/fiff_info.h>
#include <communication/rtClient/rtcmdclient.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>
#include <QComboBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulatorSetupWidget::FiffSimulatorSetupWidget(FiffSimulator* p_pFiffSimulator, QWidget* parent)
: QWidget(parent)
, m_pFiffSimulator(p_pFiffSimulator)
, m_bIsInit(false)
{
    ui.setupUi(this);

    //rt server connection
    this->ui.m_qLineEdit_Ip->setText(m_pFiffSimulator->m_sFiffSimulatorIP);

    connect(ui.m_qPushButton_Connect, &QPushButton::released,
            this, &FiffSimulatorSetupWidget::pressedConnect);

    connect(m_pFiffSimulator, &FiffSimulator::cmdConnectionChanged,
            this, &FiffSimulatorSetupWidget::cmdConnectionChanged);

    //rt server fiffInfo received
    connect(m_pFiffSimulator, &FiffSimulator::fiffInfoAvailable,
            this, &FiffSimulatorSetupWidget::fiffInfoReceived);

    //Buffer
    connect(ui.m_qLineEdit_BufferSize, &QLineEdit::textChanged,
            this, &FiffSimulatorSetupWidget::bufferSizeEdited);

    //CLI
    connect(ui.m_qPushButton_SendCLI, &QPushButton::released,
            this, &FiffSimulatorSetupWidget::pressedSendCLI);

    this->init();
}

//=============================================================================================================

FiffSimulatorSetupWidget::~FiffSimulatorSetupWidget()
{
}

//=============================================================================================================

void FiffSimulatorSetupWidget::init()
{
    cmdConnectionChanged(m_pFiffSimulator->m_bCmdClientIsConnected);
}

//=============================================================================================================

void FiffSimulatorSetupWidget::bufferSizeEdited()
{
    bool t_bSuccess = false;
    qint32 t_iBufferSize = ui.m_qLineEdit_BufferSize->text().toInt(&t_bSuccess);

    if(t_bSuccess && t_iBufferSize > 0)
        m_pFiffSimulator->m_iBufferSize = t_iBufferSize;
    else
        ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pFiffSimulator->m_iBufferSize));
}

//=============================================================================================================

void FiffSimulatorSetupWidget::pressedConnect()
{
    if(m_pFiffSimulator->m_bCmdClientIsConnected)
        m_pFiffSimulator->disconnectCmdClient();
    else
    {
        m_pFiffSimulator->m_sFiffSimulatorIP = this->ui.m_qLineEdit_Ip->text();
        m_pFiffSimulator->connectCmdClient();
    }
}

//=============================================================================================================

void FiffSimulatorSetupWidget::pressedSendCLI()
{
    if(m_pFiffSimulator->m_bCmdClientIsConnected)
    {
        this->printToLog(this->ui.m_qLineEdit_SendCLI->text());
        QString t_sReply = m_pFiffSimulator->m_pRtCmdClient->sendCLICommand(this->ui.m_qLineEdit_SendCLI->text());
        this->printToLog(t_sReply);
    }
}

//=============================================================================================================

void FiffSimulatorSetupWidget::printToLog(QString logMsg)
{
    ui.m_qTextBrowser_ServerMessage->insertPlainText(logMsg+"\n");
    //scroll down to the newest entry
    QTextCursor c = ui.m_qTextBrowser_ServerMessage->textCursor();
    c.movePosition(QTextCursor::End);
    ui.m_qTextBrowser_ServerMessage->setTextCursor(c);
}

//=============================================================================================================

void FiffSimulatorSetupWidget::cmdConnectionChanged(bool p_bConnectionStatus)
{
    m_bIsInit = false;

    if(p_bConnectionStatus)
    {
        //
        // set frequency txt
        //
        if(m_pFiffSimulator->m_pFiffInfo)
            this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pFiffSimulator->m_pFiffInfo->sfreq));

        //
        // set buffer size txt
        //
        this->ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pFiffSimulator->m_iBufferSize));

        //
        // set connectors
        //
//        QMap<qint32, QString>::ConstIterator it = m_pFiffSimulator->m_qMapConnectors.begin();
//        qint32 idx = 0;

//        for(; it != m_pFiffSimulator->m_qMapConnectors.end(); ++it)
//        {
//            if(this->ui.m_qComboBox_Connector->findData(it.key()) == -1)
//            {
//                this->ui.m_qComboBox_Connector->insertItem(idx, it.value(), it.key());
//                ++idx;
//            }
//            else
//                idx = this->ui.m_qComboBox_Connector->findData(it.key()) + 1;
//        }
//        this->ui.m_qComboBox_Connector->setCurrentIndex(this->ui.m_qComboBox_Connector->findData(m_pFiffSimulator->m_iActiveConnectorId));

        //UI enables/disables
        this->ui.m_qLabel_ConnectionStatus->setText(QString("Connected"));
        this->ui.m_qLineEdit_Ip->setEnabled(false);
        this->ui.m_qPushButton_Connect->setText(QString("Disconnect"));
        this->ui.m_qLineEdit_SendCLI->setEnabled(true);
        this->ui.m_qPushButton_SendCLI->setEnabled(true);

        m_bIsInit = true;
    }
    else
    {
        //clear connectors --> ToDO create a clear function
        m_pFiffSimulator->m_qMapConnectors.clear();
//        this->ui.m_qComboBox_Connector->clear();
        m_pFiffSimulator->m_iBufferSize = -1;

        //UI enables/disables
        this->ui.m_qLabel_ConnectionStatus->setText(QString("Not connected"));
        this->ui.m_qLineEdit_Ip->setEnabled(true);
        this->ui.m_qPushButton_Connect->setText(QString("Connect"));
        this->ui.m_qLineEdit_SendCLI->setEnabled(false);
        this->ui.m_qPushButton_SendCLI->setEnabled(false);

        this->ui.m_qLineEdit_BufferSize->setText(QString(""));

    }
}

//=============================================================================================================

void FiffSimulatorSetupWidget::fiffInfoReceived()
{
    if(m_pFiffSimulator->m_pFiffInfo)
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pFiffSimulator->m_pFiffInfo->sfreq));
}
