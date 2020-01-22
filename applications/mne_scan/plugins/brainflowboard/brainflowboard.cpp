#include <QMessageBox>

#include "brainflowboard.h"
#include "FormFiles/brainflowsetupwidget.h"

BrainFlowBoard::BrainFlowBoard() :
    boardShim(NULL),
    channels(NULL),
    boardId((int)BoardIds::SYNTHETIC_BOARD),
    isRunning(false),
    output(NULL),
    samplingRate(0),
    streamerParams(""),
    numChannels(0)
{

}

BrainFlowBoard::~BrainFlowBoard()
{
    if (boardShim)
    {
        try {
            boardShim->release_session();
        } catch (...) {
        }
    }
    delete boardShim;
    delete[] channels;
}

QSharedPointer<IPlugin> BrainFlowBoard::clone() const
{
    QSharedPointer<BrainFlowBoard> pClone(new BrainFlowBoard());
    return pClone;
}

void BrainFlowBoard::init()
{
    BoardShim::set_log_file((char *)"brainflow_log.txt");
    BoardShim::enable_dev_board_logger();
}

void BrainFlowBoard::unload()
{
}

bool BrainFlowBoard::start()
{
    isRunning = true;
    QThread::start();
    return true;
}

bool BrainFlowBoard::stop()
{
    this->isRunning = false;
    return true;
}

IPlugin::PluginType BrainFlowBoard::getType() const
{
    return _ISensor;
}

QString BrainFlowBoard::getName() const
{
    return "BrainFlow Board Plugin";
}

QWidget* BrainFlowBoard::setupWidget()
{
    BrainFlowSetupWidget* widget = new BrainFlowSetupWidget(this);
    return widget;
}

void BrainFlowBoard::run()
{
    if (!boardShim)
    {
        QMessageBox msgBox;
        msgBox.setText("Configure streaming session before run!");
        msgBox.exec();
        return;
    }
    int samplingPeriod = 1000000.0 / samplingRate;
    int numRows = 0;
    try {
        numRows = BoardShim::get_num_rows(boardId);
        boardShim->start_stream(samplingRate * 1000, (char*)streamerParams.c_str());
    } catch (const BrainFlowException &err) {
        BoardShim::log_message((int)LogLevels::LEVEL_ERROR, err.what());
        QMessageBox msgBox;
        msgBox.setText("Failed to start streaming.");
        msgBox.exec();
        return;
    }

    double **data = NULL;
    int dataCount = 0;
    while(isRunning)
    {
        usleep(samplingPeriod);
        data = boardShim->get_board_data (&dataCount);

        for (int i = 0; i < dataCount; i++)
        {
            for (int j = 0; j < numChannels; j++)
            {
                output[j]->data()->setValue(data[channels[j]][i]);
            }
        }

        for (int i = 0; i < numRows; i++)
        {
            delete[] data[i];
        }
        delete[] data;
    }
}
