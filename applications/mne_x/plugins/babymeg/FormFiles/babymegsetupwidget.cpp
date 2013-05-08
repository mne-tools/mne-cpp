//=============================================================================================================
/**
* @file     babymegsetupwidget.cpp
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

#include "babymegsetupwidget.h"
#include "babymegaboutwidget.h"
#include "../babymeg.h"


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

using namespace BabyMegPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMegSetupWidget::BabyMegSetupWidget(BabyMeg* p_pBabyMeg, QWidget* parent)
: QWidget(parent)
, m_pBabyMeg(p_pBabyMeg)
, m_bIsInit(false)
{
    ui.setupUi(this);


    connect(ui.m_qCheckBox_RecordData, &QCheckBox::stateChanged, this, &BabyMegSetupWidget::checkedRecordDataChanged);

    //Fiff Record File
    connect(ui.m_qPushButton_FiffRecordFile, &QPushButton::released, this, &BabyMegSetupWidget::pressedFiffRecordFile);

    //Select Connector
    connect(ui.m_qComboBox_Connector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BabyMegSetupWidget::connectorIdxChanged);

    //rt server connection
    this->ui.m_qLineEdit_Ip->setText(m_pBabyMeg->m_sBabyMegIP);

    connect(ui.m_qPushButton_Connect, &QPushButton::released, this, &BabyMegSetupWidget::pressedConnect);

    connect(m_pBabyMeg, &BabyMeg::cmdConnectionChanged, this, &BabyMegSetupWidget::cmdConnectionChanged);

    //rt server fiffInfo received
    connect(m_pBabyMeg, &BabyMeg::fiffInfoAvailable, this, &BabyMegSetupWidget::fiffInfoReceived);

    //Buffer
    connect(ui.m_qLineEdit_BufferSize, &QLineEdit::editingFinished, this, &BabyMegSetupWidget::bufferSizeEdited);

    //CLI
    connect(ui.m_qPushButton_SendCLI, &QPushButton::released, this, &BabyMegSetupWidget::pressedSendCLI);

    //About
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &BabyMegSetupWidget::showAboutDialog);

    this->init();
}


//*************************************************************************************************************

BabyMegSetupWidget::~BabyMegSetupWidget()
{

}


//*************************************************************************************************************

void BabyMegSetupWidget::init()
{
    checkedRecordDataChanged();
    cmdConnectionChanged(m_pBabyMeg->m_bCmdClientIsConnected);
}


//*************************************************************************************************************

void BabyMegSetupWidget::bufferSizeEdited()
{
    bool t_bSuccess = false;
    qint32 t_iBufferSize = ui.m_qLineEdit_BufferSize->text().toInt(&t_bSuccess);

    if(t_bSuccess && t_iBufferSize > 0)
        m_pBabyMeg->m_iBufferSize = t_iBufferSize;
    else
        ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pBabyMeg->m_iBufferSize));
}


//*************************************************************************************************************

void BabyMegSetupWidget::checkedRecordDataChanged()
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

void BabyMegSetupWidget::connectorIdxChanged(int idx)
{
    if(ui.m_qComboBox_Connector->itemData(idx).toInt() != m_pBabyMeg->m_iActiveConnectorId && m_bIsInit)
        m_pBabyMeg->changeConnector(ui.m_qComboBox_Connector->itemData(idx).toInt());
}


//*************************************************************************************************************

void BabyMegSetupWidget::pressedFiffRecordFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Fiff Record File"), "", tr("Fiff Record File (*.fif)"));

    ui.m_qLineEdit_FiffRecordFile->setText(fileName);
}


//*************************************************************************************************************

void BabyMegSetupWidget::pressedConnect()
{
    if(m_pBabyMeg->m_bCmdClientIsConnected)
        m_pBabyMeg->disconnectCmdClient();
    else
    {
        m_pBabyMeg->m_sBabyMegIP = this->ui.m_qLineEdit_Ip->text();
        m_pBabyMeg->connectCmdClient();
    }
}


//*************************************************************************************************************

void BabyMegSetupWidget::pressedSendCLI()
{
    if(m_pBabyMeg->m_bCmdClientIsConnected)
    {
        this->printToLog(this->ui.m_qLineEdit_SendCLI->text());
        QString t_sReply = m_pBabyMeg->m_pRtCmdClient->sendCLICommand(this->ui.m_qLineEdit_SendCLI->text());
        this->printToLog(t_sReply);
    }
}


//*************************************************************************************************************

void BabyMegSetupWidget::printToLog(QString logMsg)
{
    ui.m_qTextBrowser_ServerMessage->insertPlainText(logMsg+"\n");
    //scroll down to the newest entry
    QTextCursor c = ui.m_qTextBrowser_ServerMessage->textCursor();
    c.movePosition(QTextCursor::End);
    ui.m_qTextBrowser_ServerMessage->setTextCursor(c);
}


//*************************************************************************************************************

void BabyMegSetupWidget::cmdConnectionChanged(bool p_bConnectionStatus)
{
    m_bIsInit = false;

    if(p_bConnectionStatus)
    {
        //
        // set frequency txt
        //
        if(m_pBabyMeg->m_pFiffInfo)
            this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMeg->m_pFiffInfo->sfreq));

        //
        // set buffer size txt
        //
        this->ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pBabyMeg->m_iBufferSize));

        //
        // set connectors
        //
        QMap<qint32, QString>::ConstIterator it = m_pBabyMeg->m_qMapConnectors.begin();
        qint32 idx = 0;

        for(; it != m_pBabyMeg->m_qMapConnectors.end(); ++it)
        {
            if(this->ui.m_qComboBox_Connector->findData(it.key()) == -1)
            {
                this->ui.m_qComboBox_Connector->insertItem(idx, it.value(), it.key());
                ++idx;
            }
            else
                idx = this->ui.m_qComboBox_Connector->findData(it.key()) + 1;
        }
        this->ui.m_qComboBox_Connector->setCurrentIndex(this->ui.m_qComboBox_Connector->findData(m_pBabyMeg->m_iActiveConnectorId));

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
        m_pBabyMeg->m_qMapConnectors.clear();
        this->ui.m_qComboBox_Connector->clear();
        m_pBabyMeg->m_iBufferSize = -1;

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

void BabyMegSetupWidget::fiffInfoReceived()
{
    if(m_pBabyMeg->m_pFiffInfo)
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMeg->m_pFiffInfo->sfreq));
}


//*************************************************************************************************************

void BabyMegSetupWidget::showAboutDialog()
{
    BabyMegAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
