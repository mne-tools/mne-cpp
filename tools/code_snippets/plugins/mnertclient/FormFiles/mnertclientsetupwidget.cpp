//=============================================================================================================
/**
* @file     mnertclientsetupwidget.cpp
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
* @brief    Contains the implementation of the RTServerSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnertclientsetupwidget.h"
#include "mnertclientaboutwidget.h"

#include "mnertclientsetupbabymegwidget.h"
//#include "mnertclientsquidcontroldgl.h"

#include "../mnertclient.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>
#include <QComboBox>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MneRtClientPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneRtClientSetupWidget::MneRtClientSetupWidget(MneRtClient* p_pMneRtClient, QWidget* parent)
: QWidget(parent)
, m_pMneRtClient(p_pMneRtClient)
, m_bIsInit(false)
, m_pMneRtClientSetupFiffFileSimulatorWidget(new MneRtClientSetupFiffFileSimulatorWidget)
, m_pMneRtClientSetupNeuromagWidget(new MneRtClientSetupNeuromagWidget)
, m_pMneRtClientSetupBabyMegWidget(new MneRtClientSetupBabyMegWidget(p_pMneRtClient))
{
    ui.setupUi(this);

    //Record data checkbox
    connect(ui.m_qCheckBox_RecordData, &QCheckBox::stateChanged, this, &MneRtClientSetupWidget::checkedRecordDataChanged);

    //Fiff record file
    connect(ui.m_qPushButton_FiffRecordFile, &QPushButton::released, this, &MneRtClientSetupWidget::pressedFiffRecordFile);

    //Select connector
    connect(ui.m_qComboBox_Connector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MneRtClientSetupWidget::connectorIdxChanged);

    //Configure connector
    connect(ui.m_qPushButton_Configure, &QCheckBox::released, this, &MneRtClientSetupWidget::pressedConfigure);

    //rt server connection
    this->ui.m_qLineEdit_Ip->setText(m_pMneRtClient->m_sMneRtClientIP);

    connect(ui.m_qPushButton_Connect, &QPushButton::released, this, &MneRtClientSetupWidget::pressedConnect);

    connect(m_pMneRtClient, &MneRtClient::cmdConnectionChanged, this, &MneRtClientSetupWidget::cmdConnectionChanged);

    //rt server fiffInfo received
    connect(m_pMneRtClient, &MneRtClient::fiffInfoAvailable, this, &MneRtClientSetupWidget::fiffInfoReceived);

    //Buffer
    connect(ui.m_qLineEdit_BufferSize, &QLineEdit::editingFinished, this, &MneRtClientSetupWidget::bufferSizeEdited);

    //CLI
    connect(ui.m_qPushButton_SendCLI, &QPushButton::released, this, &MneRtClientSetupWidget::pressedSendCLI);

    //About
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &MneRtClientSetupWidget::showAboutDialog);

//    //SQUID Control
//    connect(ui.m_qPushButton_SQUIDControl, &QPushButton::released, this, &MneRtClientSetupWidget::SQUIDControlDialog);

    this->init();
}


//*************************************************************************************************************

MneRtClientSetupWidget::~MneRtClientSetupWidget()
{

}


//*************************************************************************************************************

void MneRtClientSetupWidget::init()
{
    checkedRecordDataChanged();
    cmdConnectionChanged(m_pMneRtClient->m_bCmdClientIsConnected);
}


//*************************************************************************************************************

void MneRtClientSetupWidget::bufferSizeEdited()
{
    bool t_bSuccess = false;
    qint32 t_iBufferSize = ui.m_qLineEdit_BufferSize->text().toInt(&t_bSuccess);

    if(t_bSuccess && t_iBufferSize > 0)
        m_pMneRtClient->m_iBufferSize = t_iBufferSize;
    else
        ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pMneRtClient->m_iBufferSize));
}


//*************************************************************************************************************

void MneRtClientSetupWidget::checkedRecordDataChanged()
{
    if(ui.m_qCheckBox_RecordData->checkState() == Qt::Checked)
    {
        ui.m_qComboBox_SubjectSelection->setEnabled(true);
        ui.m_qPushButton_NewSubject->setEnabled(true);
        ui.m_qLineEdit_FiffRecordFile->setEnabled(true);
        ui.m_qPushButton_FiffRecordFile->setEnabled(true);
    }
    else
    {
        ui.m_qComboBox_SubjectSelection->setEnabled(false);
        ui.m_qPushButton_NewSubject->setEnabled(false);
        ui.m_qLineEdit_FiffRecordFile->setEnabled(false);
        ui.m_qPushButton_FiffRecordFile->setEnabled(false);
    }
}


//*************************************************************************************************************

void MneRtClientSetupWidget::connectorIdxChanged(int idx)
{
    if(ui.m_qComboBox_Connector->itemData(idx).toInt() != m_pMneRtClient->m_iActiveConnectorId && m_bIsInit)
        m_pMneRtClient->changeConnector(ui.m_qComboBox_Connector->itemData(idx).toInt());
}


//*************************************************************************************************************

void MneRtClientSetupWidget::pressedFiffRecordFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Fiff Record File"), "", tr("Fiff Record File (*.fif)"));

    ui.m_qLineEdit_FiffRecordFile->setText(fileName);
}


//*************************************************************************************************************

void MneRtClientSetupWidget::pressedConnect()
{
    if(m_pMneRtClient->m_bCmdClientIsConnected)
        m_pMneRtClient->disconnectCmdClient();
    else
    {
        m_pMneRtClient->m_sMneRtClientIP = this->ui.m_qLineEdit_Ip->text();
        m_pMneRtClient->connectCmdClient();
    }
}


//*************************************************************************************************************

void MneRtClientSetupWidget::pressedSendCLI()
{
    if(m_pMneRtClient->m_bCmdClientIsConnected)
    {
        this->printToLog(this->ui.m_qLineEdit_SendCLI->text());
        QString t_sReply = m_pMneRtClient->m_pRtCmdClient->sendCLICommand(this->ui.m_qLineEdit_SendCLI->text());
        this->printToLog(t_sReply);
    }
}


//*************************************************************************************************************

void MneRtClientSetupWidget::pressedConfigure()
{
    QString t_sConnector = ui.m_qComboBox_Connector->currentText();
    if(t_sConnector ==  QString("Fiff File Simulator"))
        m_pMneRtClientSetupFiffFileSimulatorWidget->show();
    else if(t_sConnector == QString("Neuromag Connector"))
        m_pMneRtClientSetupNeuromagWidget->show();
    else if(t_sConnector == QString("BabyMEG"))
        m_pMneRtClientSetupBabyMegWidget->show();
}

//*************************************************************************************************************

void MneRtClientSetupWidget::printToLog(QString logMsg)
{
    ui.m_qTextBrowser_ServerMessage->insertPlainText(logMsg+"\n");
    //scroll down to the newest entry
    QTextCursor c = ui.m_qTextBrowser_ServerMessage->textCursor();
    c.movePosition(QTextCursor::End);
    ui.m_qTextBrowser_ServerMessage->setTextCursor(c);
}


//*************************************************************************************************************

void MneRtClientSetupWidget::cmdConnectionChanged(bool p_bConnectionStatus)
{
    m_bIsInit = false;

    if(p_bConnectionStatus)
    {
        //
        // set frequency txt
        //
        if(m_pMneRtClient->m_pFiffInfo)
            this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pMneRtClient->m_pFiffInfo->sfreq));

        //
        // set buffer size txt
        //
        this->ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pMneRtClient->m_iBufferSize));

        //
        // set connectors
        //
        QMap<qint32, QString>::ConstIterator it = m_pMneRtClient->m_qMapConnectors.begin();
        qint32 idx = 0;

        for(; it != m_pMneRtClient->m_qMapConnectors.end(); ++it)
        {
            if(this->ui.m_qComboBox_Connector->findData(it.key()) == -1)
            {
                this->ui.m_qComboBox_Connector->insertItem(idx, it.value(), it.key());
                ++idx;
            }
            else
                idx = this->ui.m_qComboBox_Connector->findData(it.key()) + 1;
        }
        this->ui.m_qComboBox_Connector->setCurrentIndex(this->ui.m_qComboBox_Connector->findData(m_pMneRtClient->m_iActiveConnectorId));

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
        m_pMneRtClient->m_qMapConnectors.clear();
        this->ui.m_qComboBox_Connector->clear();
        m_pMneRtClient->m_iBufferSize = -1;

        //UI enables/disables
        this->ui.m_qLabel_ConnectionStatus->setText(QString("Not connected"));
        this->ui.m_qLineEdit_Ip->setEnabled(true);
        this->ui.m_qPushButton_Connect->setText(QString("Connect"));
        this->ui.m_qLineEdit_SendCLI->setEnabled(false);
        this->ui.m_qPushButton_SendCLI->setEnabled(false);

        this->ui.m_qLineEdit_BufferSize->setText(QString(""));

    }
}


//*************************************************************************************************************

void MneRtClientSetupWidget::fiffInfoReceived()
{
    if(m_pMneRtClient->m_pFiffInfo)
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pMneRtClient->m_pFiffInfo->sfreq));
}


//*************************************************************************************************************

void MneRtClientSetupWidget::showAboutDialog()
{
    MneRtClientAboutWidget aboutDialog(this);
    aboutDialog.exec();
}


////*************************************************************************************************************

//void MneRtClientSetupWidget::SQUIDControlDialog()
//{
//    mnertclientSQUIDControlDgl SQUIDCtrlDlg(m_pMneRtClient,this);
//    SQUIDCtrlDlg.exec();
//}
