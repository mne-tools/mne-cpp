#include <QMessageBox>

#include "brainflowsetupwidget.h"
#include "ui_brainflowsetupwidget.h"

#include "board_shim.h"

BrainFlowSetupWidget::BrainFlowSetupWidget(BrainFlowBoard *board, QWidget *parent) :
    QWidget(parent),
    brainFlowBoard(board),
    ui(new Ui::BrainFlowSetupWidget)
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
    ui->boardId->addItem("Streaming Board");
    ui->boardId->addItem("Synthetic Board");
    ui->boardId->addItem("Cyton");
    ui->boardId->addItem("Ganglion");
    ui->boardId->addItem("Cyton Daisy");
    ui->boardId->addItem("Ganglion Wifi");
    ui->boardId->addItem("Cyton Wifi");
    ui->boardId->addItem("Cyton Daisy Wifi");
    ui->boardId->setCurrentIndex(1); // Synthetic board is default
    // vert scale
    ui->vertScale->addItem("50");
    ui->vertScale->addItem("100");
    ui->vertScale->addItem("200");
    ui->vertScale->addItem("400");
    ui->vertScale->addItem("1500");
    ui->vertScale->addItem("10000");
    ui->vertScale->setCurrentIndex(4); // 1500 is default, synthetic board amplitude is ~1000

    connect(ui->prepareSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::prepareSession);
    connect(ui->releaseSession, &QPushButton::clicked, this, &BrainFlowSetupWidget::releaseSession);
}

BrainFlowSetupWidget::~BrainFlowSetupWidget()
{
    delete ui;
}

void BrainFlowSetupWidget::releaseSession()
{
    brainFlowBoard->releaseSession();
}

void BrainFlowSetupWidget::prepareSession()
{
    BrainFlowInputParams params;
    params.ip_address = ui->ipAddress->text().toStdString();
    params.ip_port = ui->ipPort->text().toInt();
    params.mac_address = ui->macAddress->text().toStdString();
    params.other_info = ui->otherInfo->text().toStdString();
    params.serial_port = ui->serialPort->text().toStdString();

    std::string streamerParams = ui->streamerParams->text().toStdString();

    int dataType = ui->dataType->currentIndex();
    int vertScale = ui->vertScale->currentText().toInt();

    int boardIndex = ui->boardId->currentIndex();
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

    brainFlowBoard->prepareSession(params, streamerParams, boardId, dataType, vertScale);
}
