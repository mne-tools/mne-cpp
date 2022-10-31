//=============================================================================================================
/**
 * @file     brainflowsetupwidget.cpp
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Andrey Parfenov. All rights reserved.
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
 * @brief    Contains the definition of the BrainFlowSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMessageBox>

#include "brainflowsetupwidget.h"
#include "ui_brainflowsetupwidget.h"

#include "board_shim.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINFLOWBOARDPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowSetupWidget::BrainFlowSetupWidget(BrainFlowBoard *board, QWidget *parent)
: QWidget(parent)
, m_pBrainFlowBoard(board)
, ui(new Ui::BrainFlowSetupWidget)
{
    ui->setupUi(this);
    // data type to display
    ui->dataType->addItem("EEG");
    ui->dataType->addItem("EMG");
    ui->dataType->addItem("ECG");
    ui->dataType->addItem("EOG");
    ui->dataType->addItem("EDA");
    ui->dataType->setCurrentIndex(0);
    // board Id
    ui->boardId->addItem("PlayBack Board");
    ui->boardId->addItem("Streaming Board");
    ui->boardId->addItem("Synthetic Board");
    ui->boardId->addItem("Cyton");
    ui->boardId->addItem("Ganglion");
    ui->boardId->addItem("Cyton Daisy");
    ui->boardId->addItem("Ganglion Wifi");
    ui->boardId->addItem("Cyton Wifi");
    ui->boardId->addItem("Cyton Daisy Wifi");
    ui->boardId->addItem("BrainBit");
    ui->boardId->addItem("Unicorn");
    ui->boardId->addItem("Callibri EEG");
    ui->boardId->addItem("Callibri EMG");
    ui->boardId->addItem("Callibri ECG");
    ui->boardId->addItem("Notion 1");
    ui->boardId->addItem("Notion 2");
    ui->boardId->addItem("IronBCI");
    ui->boardId->addItem("FreeEEG32");
    ui->boardId->setCurrentIndex(2); // Synthetic board is default

    connect(ui->prepareSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::prepareSession);
    connect(ui->releaseSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::releaseSession);
}

//=============================================================================================================

BrainFlowSetupWidget::~BrainFlowSetupWidget()
{
    delete ui;
}

//=============================================================================================================

void BrainFlowSetupWidget::releaseSession()
{
    m_pBrainFlowBoard->releaseSession();
}

//=============================================================================================================

void BrainFlowSetupWidget::prepareSession()
{
    BrainFlowInputParams params;
    params.ip_address = ui->ipAddress->text().toStdString();
    params.ip_port = ui->ipPort->text().toInt();
    params.mac_address = ui->macAddress->text().toStdString();
    params.other_info = ui->otherInfo->text().toStdString();
    params.serial_port = ui->serialPort->text().toStdString();
    params.serial_number = ui->serialNumber->text().toStdString();
    params.file = ui->file->text().toStdString();

    std::string streamerParams = ui->streamerParams->text().toStdString();

    int dataType = ui->dataType->currentIndex();

    int boardIndex = ui->boardId->currentIndex();
    int boardId = (int)BoardIds::SYNTHETIC_BOARD;
    switch (boardIndex)
    {
        case 0:
            boardId = (int)BoardIds::PLAYBACK_FILE_BOARD;
            break;
        case 1:
            boardId = (int)BoardIds::STREAMING_BOARD;
            break;
        case 2:
            boardId = (int)BoardIds::SYNTHETIC_BOARD;
            break;
        case 3:
            boardId = (int)BoardIds::CYTON_BOARD;
            break;
        case 4:
            boardId = (int)BoardIds::GANGLION_BOARD;
            break;
        case 5:
            boardId = (int)BoardIds::CYTON_DAISY_BOARD;
            break;
        case 6:
            boardId = (int)BoardIds::GANGLION_WIFI_BOARD;
            break;
        case 7:
            boardId = (int)BoardIds::CYTON_WIFI_BOARD;
            break;
        case 8:
            boardId = (int)BoardIds::CYTON_DAISY_WIFI_BOARD;
            break;
        case 9:
            boardId = (int)BoardIds::BRAINBIT_BOARD;
            break;
        case 10:
            boardId = (int)BoardIds::UNICORN_BOARD;
            break;
        case 11:
            boardId = (int)BoardIds::CALLIBRI_EEG_BOARD;
            break;
        case 12:
            boardId = (int)BoardIds::CALLIBRI_EMG_BOARD;
            break;
        case 13:
            boardId = (int)BoardIds::CALLIBRI_ECG_BOARD;
            break;
        case 14:
            boardId = (int)BoardIds::NOTION_1_BOARD;
            break;
        case 15:
            boardId = (int)BoardIds::NOTION_2_BOARD;
            break;
        case 16:
            boardId = (int)BoardIds::IRONBCI_BOARD;
            break;
        case 17:
            boardId = (int)BoardIds::FREEEEG32_BOARD;
            break;
    }

    m_pBrainFlowBoard->prepareSession(params, streamerParams, boardId, dataType);
}
