//=============================================================================================================
/**
 * @file     brainflowsetupwidget.cpp
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke, Lorenz Esch. All rights reserved.
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
 * @brief Contains the definition of the BrainFlowSetupWidget class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowsetupwidget.h"
#include "ui_brainflowsetupwidget.h"
#include "board_shim.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowSetupWidget::BrainFlowSetupWidget(BrainFlowBoard *p_pBoard, QWidget *parent) :
    QWidget(parent),
    m_pBrainFlowBoard(p_pBoard),
    ui(new Ui::BrainFlowSetupWidget)
{
    ui->setupUi(this);
    // data type to display
    ui->m_qDataType->addItem("EEG");
    ui->m_qDataType->addItem("EMG");
    ui->m_qDataType->addItem("ECG");
    ui->m_qDataType->addItem("EOG");
    ui->m_qDataType->addItem("EDA");
    ui->m_qDataType->setCurrentIndex(0);
    // board Id
    ui->m_qBoardId->addItem("Streaming Board");
    ui->m_qBoardId->addItem("Synthetic Board");
    ui->m_qBoardId->addItem("Cyton");
    ui->m_qBoardId->addItem("Ganglion");
    ui->m_qBoardId->addItem("Cyton Daisy");
    ui->m_qBoardId->addItem("Ganglion Wifi");
    ui->m_qBoardId->addItem("Cyton Wifi");
    ui->m_qBoardId->addItem("Cyton Daisy Wifi");
    ui->m_qBoardId->setCurrentIndex(1); // Synthetic board is default

    connect(ui->m_qbnPrepareSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::prepareSession);
    connect(ui->m_qbnReleaseSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::releaseSession);
}

//*************************************************************************************************************

BrainFlowSetupWidget::~BrainFlowSetupWidget()
{
    delete ui;
}

//*************************************************************************************************************

void BrainFlowSetupWidget::releaseSession()
{
    m_pBrainFlowBoard->releaseSession();
}

//*************************************************************************************************************

void BrainFlowSetupWidget::prepareSession()
{
    BrainFlowInputParams params;
    params.ip_address = ui->m_qIpAddress->text().toStdString();
    params.ip_port = ui->m_qIpPort->text().toInt();
    params.mac_address = ui->m_qMacAddress->text().toStdString();
    params.other_info = ui->m_qOtherInfo->text().toStdString();
    params.serial_port = ui->m_qSerialPort->text().toStdString();

    std::string streamerParams = ui->m_qStreamerParams->text().toStdString();

    int dataType = ui->m_qDataType->currentIndex();

    int boardIndex = ui->m_qBoardId->currentIndex();
    int boardId = (int)BoardIds::SYNTHETIC_BOARD;;
    switch (boardIndex)
    {
        case 0:
            boardId = (int)BoardIds::STREAMING_BOARD;
            break;
        case 1:
            boardId = (int)BoardIds::SYNTHETIC_BOARD;
            break;
        case 2:
            boardId = (int)BoardIds::CYTON_BOARD;
            break;
        case 3:
            boardId = (int)BoardIds::GANGLION_BOARD;
            break;
        case 4:
            boardId = (int)BoardIds::CYTON_DAISY_BOARD;
            break;
        case 5:
            boardId = (int)BoardIds::GANGLION_WIFI_BOARD;
            break;
        case 6:
            boardId = (int)BoardIds::CYTON_WIFI_BOARD;
            break;
        case 7:
            boardId = (int)BoardIds::CYTON_DAISY_WIFI_BOARD;
            break;
    }

    m_pBrainFlowBoard->prepareSession(params, streamerParams, boardId, dataType);
}
