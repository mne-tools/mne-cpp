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
    ui->dataType->addItem("EEG");
    ui->dataType->addItem("EMG");
    ui->dataType->addItem("ECG");
    ui->dataType->addItem("EOG");
    ui->dataType->addItem("PPG");
    ui->dataType->addItem("EDA");
    ui->dataType->setCurrentIndex(0);
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
    int boardId = ui->boardId->text().toInt();
    int dataType = ui->dataType->currentIndex();

    brainFlowBoard->prepareSession(params, streamerParams, boardId, dataType);
}
