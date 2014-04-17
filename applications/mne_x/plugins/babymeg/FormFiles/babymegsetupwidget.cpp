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

#include "babymegsetupbabymegwidget.h"
//#include "babymegsquidcontroldgl.h"

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

using namespace BabyMEGPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSetupWidget::BabyMEGSetupWidget(BabyMEG* p_pBabyMEG, QWidget* parent)
: QWidget(parent)
, m_pBabyMEG(p_pBabyMEG)
, m_bIsInit(false)
, m_pBabyMEGSetupBabyMegWidget(new BabyMEGSetupBabyMegWidget(p_pBabyMEG))
{
    ui.setupUi(this);

    //Record data checkbox
    connect(ui.m_qCheckBox_RecordData, &QCheckBox::stateChanged, this, &BabyMEGSetupWidget::checkedRecordDataChanged);

    //Fiff record file
    connect(ui.m_qPushButton_FiffRecordFile, &QPushButton::released, this, &BabyMEGSetupWidget::pressedFiffRecordFile);

    //Select connector
    connect(ui.m_qComboBox_Connector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BabyMEGSetupWidget::connectorIdxChanged);

    //Configure connector
    connect(ui.m_qPushButton_Configure, &QCheckBox::released, this, &BabyMEGSetupWidget::pressedConfigure);

    //rt server connection
    this->ui.m_qLineEdit_Ip->setText(m_pBabyMEG->m_sBabyMEGIP);

    connect(ui.m_qPushButton_Connect, &QPushButton::released, this, &BabyMEGSetupWidget::pressedConnect);

    connect(m_pBabyMEG, &BabyMEG::cmdConnectionChanged, this, &BabyMEGSetupWidget::cmdConnectionChanged);

    //rt server fiffInfo received
    connect(m_pBabyMEG, &BabyMEG::fiffInfoAvailable, this, &BabyMEGSetupWidget::fiffInfoReceived);

    //Buffer
    connect(ui.m_qLineEdit_BufferSize, &QLineEdit::editingFinished, this, &BabyMEGSetupWidget::bufferSizeEdited);

    //CLI
    connect(ui.m_qPushButton_SendCLI, &QPushButton::released, this, &BabyMEGSetupWidget::pressedSendCLI);

    //About
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &BabyMEGSetupWidget::showAboutDialog);

//    //SQUID Control
//    connect(ui.m_qPushButton_SQUIDControl, &QPushButton::released, this, &BabyMEGSetupWidget::SQUIDControlDialog);

    this->init();
}


//*************************************************************************************************************

BabyMEGSetupWidget::~BabyMEGSetupWidget()
{

}


//*************************************************************************************************************

void BabyMEGSetupWidget::init()
{
    checkedRecordDataChanged();
    cmdConnectionChanged(m_pBabyMEG->m_bCmdClientIsConnected);
}


//*************************************************************************************************************

void BabyMEGSetupWidget::bufferSizeEdited()
{
    bool t_bSuccess = false;
    qint32 t_iBufferSize = ui.m_qLineEdit_BufferSize->text().toInt(&t_bSuccess);

    if(t_bSuccess && t_iBufferSize > 0)
        m_pBabyMEG->m_iBufferSize = t_iBufferSize;
    else
        ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pBabyMEG->m_iBufferSize));
}


//*************************************************************************************************************

void BabyMEGSetupWidget::checkedRecordDataChanged()
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

void BabyMEGSetupWidget::connectorIdxChanged(int idx)
{
    if(ui.m_qComboBox_Connector->itemData(idx).toInt() != m_pBabyMEG->m_iActiveConnectorId && m_bIsInit)
        m_pBabyMEG->changeConnector(ui.m_qComboBox_Connector->itemData(idx).toInt());
}


//*************************************************************************************************************

void BabyMEGSetupWidget::pressedFiffRecordFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Fiff Record File"), "", tr("Fiff Record File (*.fif)"));

    ui.m_qLineEdit_FiffRecordFile->setText(fileName);
}


//*************************************************************************************************************

void BabyMEGSetupWidget::pressedConnect()
{
    if(m_pBabyMEG->m_bCmdClientIsConnected)
        m_pBabyMEG->disconnectCmdClient();
    else
    {
        m_pBabyMEG->m_sBabyMEGIP = this->ui.m_qLineEdit_Ip->text();
        m_pBabyMEG->connectCmdClient();
    }
}


//*************************************************************************************************************

void BabyMEGSetupWidget::pressedSendCLI()
{
    if(m_pBabyMEG->m_bCmdClientIsConnected)
    {
        this->printToLog(this->ui.m_qLineEdit_SendCLI->text());
        QString t_sReply = m_pBabyMEG->m_pRtCmdClient->sendCLICommand(this->ui.m_qLineEdit_SendCLI->text());
        this->printToLog(t_sReply);
    }
}


//*************************************************************************************************************

void BabyMEGSetupWidget::pressedConfigure()
{
    QString t_sConnector = ui.m_qComboBox_Connector->currentText();
    m_pBabyMEGSetupBabyMegWidget->show();
}

//*************************************************************************************************************

void BabyMEGSetupWidget::printToLog(QString logMsg)
{
    ui.m_qTextBrowser_ServerMessage->insertPlainText(logMsg+"\n");
    //scroll down to the newest entry
    QTextCursor c = ui.m_qTextBrowser_ServerMessage->textCursor();
    c.movePosition(QTextCursor::End);
    ui.m_qTextBrowser_ServerMessage->setTextCursor(c);
}


//*************************************************************************************************************

void BabyMEGSetupWidget::cmdConnectionChanged(bool p_bConnectionStatus)
{
    m_bIsInit = false;

    if(p_bConnectionStatus)
    {
        //
        // set frequency txt
        //
        if(m_pBabyMEG->m_pFiffInfo)
            this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMEG->m_pFiffInfo->sfreq));

        //
        // set buffer size txt
        //
        this->ui.m_qLineEdit_BufferSize->setText(QString("%1").arg(m_pBabyMEG->m_iBufferSize));

        //
        // set connectors
        //
        QMap<qint32, QString>::ConstIterator it = m_pBabyMEG->m_qMapConnectors.begin();
        qint32 idx = 0;

        for(; it != m_pBabyMEG->m_qMapConnectors.end(); ++it)
        {
            if(this->ui.m_qComboBox_Connector->findData(it.key()) == -1)
            {
                this->ui.m_qComboBox_Connector->insertItem(idx, it.value(), it.key());
                ++idx;
            }
            else
                idx = this->ui.m_qComboBox_Connector->findData(it.key()) + 1;
        }
        this->ui.m_qComboBox_Connector->setCurrentIndex(this->ui.m_qComboBox_Connector->findData(m_pBabyMEG->m_iActiveConnectorId));

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
        m_pBabyMEG->m_qMapConnectors.clear();
        this->ui.m_qComboBox_Connector->clear();
        m_pBabyMEG->m_iBufferSize = -1;

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

void BabyMEGSetupWidget::fiffInfoReceived()
{
    if(m_pBabyMEG->m_pFiffInfo)
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMEG->m_pFiffInfo->sfreq));
}


//*************************************************************************************************************

void BabyMEGSetupWidget::showAboutDialog()
{
    BabyMEGAboutWidget aboutDialog(this);
    aboutDialog.exec();
}


////*************************************************************************************************************

//void BabyMEGSetupWidget::SQUIDControlDialog()
//{
//    BabyMEGSQUIDControlDgl SQUIDCtrlDlg(m_pBabyMEG,this);
//    SQUIDCtrlDlg.exec();
//}
