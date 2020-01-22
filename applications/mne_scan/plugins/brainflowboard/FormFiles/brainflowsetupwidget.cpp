#include <QMessageBox>

#include "brainflowsetupwidget.h"
#include "ui_brainflowsetupwidget.h"

#include "board_shim.h"

BrainFlowSetupWidget::BrainFlowSetupWidget(BrainFlowBoard *board, QWidget *parent) :
    QWidget(parent),
    brainFlowBoard(board),
    initialized(false),
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
    connect(ui->submitInfo, &QPushButton::clicked, this, &BrainFlowSetupWidget::prepareParams);
}

BrainFlowSetupWidget::~BrainFlowSetupWidget()
{
    delete ui;
}

void BrainFlowSetupWidget::prepareParams()
{
    if (initialized)
    {
        QMessageBox msgBox;
        msgBox.setText("Streaming session already prepared.");
        msgBox.exec();
        return;
    }

    QMessageBox msgBox;

    BrainFlowInputParams params;
    params.ip_address = ui->ipAddress->text().toStdString();
    params.ip_port = ui->ipPort->text().toInt();
    params.mac_address = ui->macAddress->text().toStdString();
    params.other_info = ui->otherInfo->text().toStdString();
    params.serial_port = ui->serialPort->text().toStdString();
    std::string streamerParams = ui->streamerParams->text().toStdString();
    int boardId = ui->boardId->text().toInt();
    int dataType = ui->dataType->currentIndex();

    brainFlowBoard->params = params;
    brainFlowBoard->boardId = boardId;
    brainFlowBoard->streamerParams = streamerParams;

    try {
        switch (dataType)
        {
            case 0:
                brainFlowBoard->channels = BoardShim::get_eeg_channels(boardId, &brainFlowBoard->numChannels);
                break;
            case 1:
                brainFlowBoard->channels = BoardShim::get_emg_channels(boardId, &brainFlowBoard->numChannels);
                break;
            case 2:
                brainFlowBoard->channels = BoardShim::get_ecg_channels(boardId, &brainFlowBoard->numChannels);
                break;
            case 3:
                brainFlowBoard->channels = BoardShim::get_eog_channels(boardId, &brainFlowBoard->numChannels);
                break;
            case 4:
                brainFlowBoard->channels = BoardShim::get_ppg_channels(boardId, &brainFlowBoard->numChannels);
                break;
            case 5:
                brainFlowBoard->channels = BoardShim::get_eda_channels(boardId, &brainFlowBoard->numChannels);
                break;
            default:
                throw BrainFlowException ("unsupported data type", UNSUPPORTED_BOARD_ERROR);
                break;
        }
        brainFlowBoard->samplingRate = BoardShim::get_sampling_rate(boardId);
        brainFlowBoard->boardShim = new BoardShim(boardId, params);
        brainFlowBoard->boardShim->prepare_session();

        brainFlowBoard->output = new PluginOutputData<RealTimeSampleArray>::SPtr[brainFlowBoard->numChannels];
        for (int i = 0; i < brainFlowBoard->numChannels; i++)
        {
            brainFlowBoard->output[i] = PluginOutputData<RealTimeSampleArray>::create(brainFlowBoard, "Channel", "Channel output data");
            brainFlowBoard->m_outputConnectors.append(brainFlowBoard->output[i]);
            brainFlowBoard->output[i]->data()->setSamplingRate(brainFlowBoard->samplingRate);
            brainFlowBoard->output[i]->data()->setVisibility(true);
            brainFlowBoard->output[i]->data()->setArraySize(1);
        }

        msgBox.setText("Streaming session is ready");
        initialized = true;
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        msgBox.setText("Invalid parameters. Check logs for details.");

        delete[] brainFlowBoard->channels;
        delete brainFlowBoard->boardShim;
        brainFlowBoard->channels = NULL;
        brainFlowBoard->boardShim = NULL;
        brainFlowBoard->numChannels = 0;
        delete[] brainFlowBoard->output;
    }

    msgBox.exec();
}
